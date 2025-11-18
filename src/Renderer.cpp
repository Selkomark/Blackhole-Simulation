#include "../include/Renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>

// --- ThreadPool Implementation ---

ThreadPool::ThreadPool(size_t numThreads) : stop(false), activeTasks(0) {
  for (size_t i = 0; i < numThreads; ++i)
    workers.emplace_back([this] {
      for (;;) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(this->queueMutex);
          this->condition.wait(
              lock, [this] { return this->stop || !this->tasks.empty(); });
          if (this->stop && this->tasks.empty())
            return;
          task = std::move(this->tasks.front());
          this->tasks.pop();
          this->activeTasks++;
        }
        task();
        activeTasks--;
        finishedCondition.notify_one();
      }
    });
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    stop = true;
  }
  condition.notify_all();
  for (std::thread &worker : workers)
    worker.join();
}

void ThreadPool::enqueue(std::function<void()> task) {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    tasks.emplace(std::move(task));
  }
  condition.notify_one();
}

void ThreadPool::waitFinished() {
  std::unique_lock<std::mutex> lock(queueMutex);
  finishedCondition.wait(
      lock, [this]() { return tasks.empty() && activeTasks == 0; });
}

// --- Renderer Implementation ---

Renderer::Renderer(int width, int height, SDL_Renderer *sdlRenderer)
    : width(width), height(height), sdlRenderer(sdlRenderer),
      pool(std::thread::hardware_concurrency()) { // Use all available cores
  texture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  pixels.resize(width * height);
}

Renderer::~Renderer() { SDL_DestroyTexture(texture); }

void Renderer::updateTexture() {
  SDL_UpdateTexture(texture, nullptr, pixels.data(), width * sizeof(uint32_t));
  SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
}

uint32_t Renderer::tracePixel(const Ray &ray, const BlackHole &bh) {
  // Use the new volumetric trace
  Vector3 color = bh.trace(ray);

  // Tone mapping (simple reinhard)
  color = color / (color + Vector3(1, 1, 1));

  // Gamma correction
  color.x = std::pow(color.x, 1.0 / 2.2);
  color.y = std::pow(color.y, 1.0 / 2.2);
  color.z = std::pow(color.z, 1.0 / 2.2);

  int r = std::min(255, (int)(color.x * 255.0));
  int g = std::min(255, (int)(color.y * 255.0));
  int b = std::min(255, (int)(color.z * 255.0));

  return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void Renderer::renderRow(int y, const BlackHole &bh, const Camera &cam) {
  double aspectRatio = (double)width / height;
  double scale = std::tan((cam.fov * 0.5) * (std::numbers::pi / 180.0));

  for (int x = 0; x < width; ++x) {
    double px = (2.0 * (x + 0.5) / width - 1.0) * aspectRatio * scale;
    double py = (1.0 - 2.0 * (y + 0.5) / height) * scale;

    Vector3 dir = (cam.forward + cam.right * px + cam.up * py).normalized();
    Ray ray(cam.position, dir);

    pixels[y * width + x] = tracePixel(ray, bh);
  }
}

void Renderer::render(const BlackHole &bh, const Camera &cam) {
  // Enqueue tasks for each row (or chunks of rows for less overhead)
  // Chunking is better for performance to reduce lock contention
  int rowsPerChunk = 8;
  for (int y = 0; y < height; y += rowsPerChunk) {
    pool.enqueue([this, y, rowsPerChunk, &bh, &cam]() {
      int endY = std::min(y + rowsPerChunk, height);
      for (int r = y; r < endY; ++r) {
        renderRow(r, bh, cam);
      }
    });
  }
  pool.waitFinished();
}
