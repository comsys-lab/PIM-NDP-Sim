#include "aimulator/src/base/base.h"
#include "aimulator/src/base/AiM_request.h"
#include "aimulator/src/base/config.h"
#include "aimulator/src/frontend/frontend.h"
#include "aimulator/src/memory_system/memory_system.h"

#ifndef     RAMULATOR_BASE_BASE_H
#define     RAMULATOR_BASE_BASE_H

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "base/type.h"
#include "base/factory.h"
#include "base/clocked.h"
#include "base/param.h"
#include "base/exception.h"
#include "base/logging.h"
#include "base/debug.h"
#include "base/AiM_request.h"
#include "base/utils.h"
#include "base/stats.h"


#ifndef uint
#define uint unsigned int
#endif

int main (int argc, char* argv[]) {
  // Parse command line arguments
  argparse::ArgumentParser program("Ramulator", "2.0");
  program.add_argument("-c", "--config").metavar("\"dumped YAML configuration\"").help("String dump of the yaml configuration.");
  program.add_argument("-f", "--config_file").metavar("path-to-configuration-file").help("Path to a YAML configuration file.");
  program.add_argument("-p", "--param").metavar("KEY=VALUE").append().help("Specify parameter to override in the configuration file. Repeat this option to change multiple parameters.");

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::runtime_error& err) {
    spdlog::error(err.what());
    std::cerr << program;
    std::exit(1);
  }

  // Are we accepting the configuration YAML through commandline dump?
  bool use_dumped_yaml = false;
  std::string dumped_config;
  if (auto arg = program.present<std::string>("-c")) {
    use_dumped_yaml = true;
    dumped_config = *arg;
  }

  // Are we gettign a path to a YAML document?
  bool use_yaml_file = false;
  std::string config_file_path;
  if (auto arg = program.present<std::string>("-f")) {
    use_yaml_file = true;
    config_file_path = *arg;
  }

  // Are we overriding some parameters in a YAML document from the comand line?
  bool has_param_override = false;
  std::vector<std::string> params;
  if (auto arg = program.present<std::vector<std::string>>("-p")) {
    has_param_override = true;
    params = *arg;
  }

  // Some sanity check of the inputs
  if (use_dumped_yaml && use_yaml_file) {
    spdlog::error("Dumped config and loaded config cannot be used together!");
    std::cerr << program;
    std::exit(1);
  } else if (!(use_dumped_yaml || use_yaml_file)) {
    spdlog::error("No configuration specified!");
    std::cerr << program;
    std::exit(1);
  }

  if (use_dumped_yaml && has_param_override) {
    spdlog::warn("Using dumped configuration. Parameter overrides with -p/--param will be ignored!");
  }
  
  // Parse the configurations
  YAML::Node config;
  if (use_dumped_yaml) {
    std::string dumped_config = program.get<std::string>("-c");
    config = YAML::Load(dumped_config);
  } else if (use_yaml_file) {
    config = Ramulator::Config::parse_config_file(config_file_path, params);
  }

  Ramulator::IFrontEnd* frontend = Ramulator::Factory::create_frontend(config);
  Ramulator::IMemorySystem* memory_system = Ramulator::Factory::create_memory_system(config);
}