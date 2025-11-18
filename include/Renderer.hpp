#pragma once
#include "BlackHole.hpp"
#include "Vector3.hpp"
#include <SDL2/SDL.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct Camera {
  Vector3 position;
  Vector3 forward;
  Vector3 up;
  Vector3 right;
  double fov;

  Camera(Vector3 pos, Vector3 lookAt, double fovDeg)
      : position(pos), fov(fovDeg) {
    forward = (lookAt - pos).normalized();
    Vector3 worldUp(0, 1, 0);
    right = forward.cross(worldUp).normalized();
    up = right.cross(forward).normalized();
  }
};

class ThreadPool {
public:
  ThreadPool(size_t numThreads);
  ~ThreadPool();

  void enqueue(std::function<void()> task);
  void waitFinished();

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  std::mutex queueMutex;
  std::condition_variable condition;
  std::condition_variable finishedCondition;

  bool stop;
  std::atomic<int> activeTasks;
};

class Renderer {
public:
  Renderer(int width, int height, SDL_Renderer *sdlRenderer);
  ~Renderer();

  void render(const BlackHole &bh, const Camera &cam);
  void updateTexture();

private:
  int width, height;
  SDL_Renderer *sdlRenderer;
  SDL_Texture *texture;
  std::vector<uint32_t> pixels;
  ThreadPool pool;

  void renderRow(int y, const BlackHole &bh, const Camera &cam);
  uint32_t tracePixel(const Ray &ray, const BlackHole &bh);
};
