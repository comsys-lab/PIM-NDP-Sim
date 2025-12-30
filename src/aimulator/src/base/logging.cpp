#include "base/logging.h"


namespace Ramulator {

Logger_t Logging::create_logger(std::string name, std::string pattern) {
  auto logger = spdlog::stdout_color_st("Ramulator::" + name);

  if (!logger) {
    throw InitializationError("Error creating logger {}!", name);
  }

  logger->set_pattern(pattern);
  logger->set_level(spdlog::level::debug);
  return logger;
}

Logger_t Logging::get(std::string name) {
  auto logger = spdlog::get("Ramulator::" + name);
  if (logger) {
    return logger;
  } else {
    throw std::runtime_error(
      fmt::format(
        "Logger {} does not exist!",
        name
      )
    );
  }
}

void Logging::destroy_logger(std::string name) {
  spdlog::drop("Ramulator::" + name);
}

void Logging::destroy_all_loggers() {
  spdlog::drop_all();
}

bool Logging::_create_base_logger() {
  auto logger = create_logger("Base");
  if (logger) {
    return true;
  } else {
    throw InitializationError("Error creating the base logger!");
  }
  return false;
}

}        // namespace Ramulator
