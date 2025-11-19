#include "../include/core/Application.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

// Get path to log file in user's home directory
std::string getLogPath() {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Logs/BlackHoleSim.log";
    }
    return "/tmp/BlackHoleSim.log";
}

int main(int, char **) {
  std::string logPath = getLogPath();
  std::ofstream logFile(logPath, std::ios::app);
  
  // Write startup message to both stderr and log file
  std::cerr << "\n=== BlackHoleSim Startup ===" << std::endl;
  std::cerr << "Log file: " << logPath << std::endl;
  if (logFile.is_open()) {
    logFile << "\n=== BlackHoleSim Startup ===" << std::endl;
    logFile << "Log file: " << logPath << std::endl;
    logFile.flush();
  }
  
  Application app;
  
  if (!app.initialize()) {
    std::cerr << "\n[FATAL] Failed to initialize application!" << std::endl;
    if (logFile.is_open()) {
      logFile << "[FATAL] Failed to initialize application!" << std::endl;
      logFile.close();
    }
    return 1;
  }
  
  std::cerr << "\n[SUCCESS] Application initialized, entering main loop..." << std::endl;
  if (logFile.is_open()) {
    logFile << "[SUCCESS] Application initialized, entering main loop..." << std::endl;
    logFile.close();
  }
  
  app.run();
  
  return 0;
}
