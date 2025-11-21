#include "../include/core/Application.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <cstring>

// Global log file stream (for Application to use)
std::ofstream* g_logFile = nullptr;

// Forward declaration for Application to use
void appLog(const std::string& message, bool isError = false);

// Get path to log file
std::string getLogPath(const std::string& xrayId = "") {
    if (!xrayId.empty()) {
        // Xray mode: use /tmp with reference ID
        return "/tmp/blackhole_sim_xray_" + xrayId + ".log";
    }
    
    // Default: use user's Library/Logs
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Logs/BlackHoleSim.log";
    }
    return "/tmp/BlackHoleSim.log";
}

// Logging helper that writes to both console and log file
void logMessage(const std::string& message, bool isError = false) {
    // Get timestamp using strftime (safer than std::put_time on macOS)
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
    
    std::string logLine = "[" + std::string(timestamp) + "] " + message;
    
    // Write to console
    if (isError) {
        std::cerr << logLine << std::endl;
    } else {
        std::cout << logLine << std::endl;
    }
    
    // Write to log file if open
    if (g_logFile && g_logFile->is_open()) {
        *g_logFile << logLine << std::endl;
        g_logFile->flush();
    }
}

// Logging function for Application to use
void appLog(const std::string& message, bool isError) {
    logMessage(message, isError);
}

int main(int argc, char* argv[]) {
  std::string xrayId;
  bool xrayMode = false;
  
  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--xray" && i + 1 < argc) {
      xrayId = argv[++i];
      xrayMode = true;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Black Hole Simulation\n";
      std::cout << "Usage: " << argv[0] << " [--xray REFERENCE_ID]\n";
      std::cout << "\nOptions:\n";
      std::cout << "  --xray REFERENCE_ID    Enable detailed logging to /tmp/blackhole_sim_xray_REFERENCE_ID.log\n";
      std::cout << "  --help, -h             Show this help message\n";
      return 0;
    }
  }
  
  // Determine log path
  std::string logPath = getLogPath(xrayId);
  
  // Open log file (append mode for default, overwrite for xray)
  std::ofstream logFile;
  if (xrayMode) {
    logFile.open(logPath, std::ios::out | std::ios::trunc);
  } else {
    logFile.open(logPath, std::ios::app);
  }
  
  g_logFile = &logFile;
  
  // Write startup message
  std::ostringstream startupMsg;
  startupMsg << "\n=== BlackHoleSim Startup ===";
  if (xrayMode) {
    startupMsg << " [XRAY MODE: " << xrayId << "]";
  }
  startupMsg << "\nLog file: " << logPath;
  
  logMessage(startupMsg.str());
  
  if (!logFile.is_open()) {
    std::cerr << "[WARNING] Could not open log file: " << logPath << std::endl;
  }
  
  Application app;
  
  if (!app.initialize()) {
    logMessage("[FATAL] Failed to initialize application!", true);
    if (logFile.is_open()) {
      logFile.close();
    }
    return 1;
  }
  
  logMessage("[SUCCESS] Application initialized, entering main loop...");
  
  app.run();
  
  logMessage("[INFO] Application shutting down...");
  
  if (logFile.is_open()) {
    logFile.close();
  }
  g_logFile = nullptr;
  
  return 0;
}
