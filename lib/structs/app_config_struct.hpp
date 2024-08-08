#pragma once

#ifndef APP_CONFIG_STRUCT_HPP
#define APP_CONFIG_STRUCT_HPP

#include "../enums/app_env_enum.hpp"

struct AppConfig {
  static const AppEnvironment environment = AppEnvironment::PRODUCTION;
};

#endif // APP_CONFIG_STRUCT_HPP