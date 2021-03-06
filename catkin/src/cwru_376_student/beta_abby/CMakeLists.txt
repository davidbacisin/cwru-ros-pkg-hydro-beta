cmake_minimum_required(VERSION 2.8.3)
project(beta_abby)

find_package(catkin_simple REQUIRED)
find_package(Eigen REQUIRED)
find_package(irb120_ik REQUIRED)
find_package(example_joint_space_planner REQUIRED)

catkin_simple()
find_package(Eigen REQUIRED)
include_directories(${Eigen_INCLUDE_DIRS})
add_definitions(${EIGEN_DEFINITIONS})

# example boost usage
# find_package(Boost REQUIRED COMPONENTS system thread)

# C++0x support - not quite the same as final C++11!
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Libraries
# cs_add_library(joint_space_planner src/joint_space_planner.cpp)

# Executables
cs_add_executable(beta_example_irb120_im_interface src/beta_example_irb120_im_interface.cpp)
cs_add_executable(beta_irb120_im_interface src/beta_irb120_im_interface.cpp)
cs_add_executable(beta_irb120 src/beta_irb120.cpp)
cs_add_executable(beta_example_irb120_im_interface_v2 src/beta_example_irb120_im_interface_v2.cpp)
cs_add_executable(beta_cartesian_motion src/beta_cartesian_motion.cpp)
# target_link_library(example my_lib)

cs_install()
cs_export()
