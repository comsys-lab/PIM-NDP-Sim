add_library(ramulator-dram OBJECT)

target_sources(
  ramulator-dram PRIVATE
  # dram.h node.h spec.h  lambdas.h  
  
  # lambdas/preq.h  lambdas/rowhit.h  lambdas/rowopen.h lambdas/action.h lambdas/power.h

  # impl/DDR3.cpp
  # impl/DDR4.cpp
  # impl/DDR4-VRR.cpp
  # impl/DDR4-RVRR.cpp
  # impl/DDR5.cpp
  # impl/DDR5-VRR.cpp
  # impl/DDR5-RVRR.cpp
  # impl/LPDDR5.cpp
  # impl/HBM.cpp
  # impl/HBM2.cpp
  # impl/HBM3.cpp

  AiM_dram.h AiM_node.h spec.h AiM_lambdas.h
  lambdas/AiM_preq.h lambdas/AiM_rowhit.h lambdas/AiM_rowopen.h lambdas/AiM_action.h # lambdas/power.h
  impl/AiM_GDDR6.cpp
  impl/AiM_LPDDR5.cpp
)

target_link_libraries(
  ramulator
  PRIVATE
  ramulator-dram
)
