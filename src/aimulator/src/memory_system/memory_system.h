#ifndef     RAMULATOR_MEMORYSYSTEM_MEMORY_H
#define     RAMULATOR_MEMORYSYSTEM_MEMORY_H

#include <map>
#include <vector>
#include <string>
#include <functional>

#include "base/base.h"
#include "frontend/frontend.h"

#include <fstream>

namespace Ramulator {

class IMemorySystem : public TopLevel<IMemorySystem> {
  RAMULATOR_REGISTER_INTERFACE(IMemorySystem, "MemorySystem", "Memory system interface (e.g., communicates between processor and memory controller).")

  friend class Factory;

  protected:
    IFrontEnd* m_frontend;
    uint m_clock_ratio = 1;

  public:
    virtual void connect_frontend(IFrontEnd* frontend) { 
      m_frontend = frontend; 
      m_impl->setup(frontend, this);
      for (auto component : m_components) {
        component->setup(frontend, this);
      }
    };

    virtual void finalize() { 
      for (auto component : m_components) {
        component->finalize();
      }

      YAML::Emitter emitter;
      emitter << YAML::BeginMap;
      m_impl->print_stats(emitter);
      emitter << YAML::EndMap;
      std::cout << emitter.c_str() << std::endl;
    };

    virtual void finalize_wrapper(const char* stats_dir, const char* timestamp) {
      std::string file_path(stats_dir);
      file_path.append("/aimulator_");
      file_path.append(timestamp);
      file_path.append("system.yaml");

      for (auto component: m_components) {
        component->finalize();
      }

      YAML::Emitter emitter;
      emitter << YAML::BeginMap;
      m_impl->print_stats(emitter);
      emitter << YAML::EndMap;
      std::cout << emitter.c_str() << std::endl;

      std::ofstream output_file(file_path);
      if (output_file.is_open()) {
        output_file << emitter.c_str() << std::endl;
        output_file.close();
        std::cout << "[Ramulator] Statistics saved to: " << file_path << std::endl;
      } else {
        std::cerr << "[Ramulator] Error: Unable to open file for writing: " << file_path << std::endl;
      }
    }

    /**
     * @brief         Tries to send the request to the memory system
     * 
     * @param    req      The request
     * @return   true     Request is accepted by the memory system.
     * @return   false    Request is rejected by the memory system, maybe the memory controller is full?
     */
    virtual bool send(Request req) = 0;
    virtual bool send(std::vector<Request> req_vec) = 0;

    /**
     * @brief         Ticks the memory system
     * 
     */
    virtual void tick() = 0;

    /**
     * @brief    Returns 
     * 
     * @return   int 
     */
    int get_clock_ratio() { return m_clock_ratio; };

    // /**
    //  * @brief    Get the integer id of the request type from the memory spec
    //  * 
    //  */
    // virtual const SpecDef& get_supported_requests() = 0;

    virtual float get_tCK() { return -1.0f; };
};

}        // namespace Ramulator


#endif   // RAMULATOR_MEMORYSYSTEM_MEMORY_H