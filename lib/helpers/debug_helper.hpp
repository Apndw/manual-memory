#pragma once

#ifndef DEBUG_HELPER_HPP
#define DEBUG_HELPER_HPP

#include <iostream>

#include "../structs/app_config_struct.hpp"

namespace debug {
  static void print(const std::string &message) {
    std::cout << message << std::endl;
  }

  static void quit(const std::string &message) {
    print("ERROR: " + message);

    if (AppConfig::environment == AppEnvironment::DEVELOPMENT) {
      print("Exiting...");
      exit(1);
    }
  }
}

#endif // DEBUG_HELPER_HPP