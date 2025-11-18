#include "../include/core/Application.hpp"
#include <iostream>

int main(int, char **) {
  Application app;
  
  if (!app.initialize()) {
    std::cerr << "Failed to initialize application!" << std::endl;
    return 1;
  }
  
  app.run();
  
  return 0;
}
