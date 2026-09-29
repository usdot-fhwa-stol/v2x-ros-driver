cmake_minimum_required(VERSION 3.5)
project(v2x_ros_driver)

# find dependencies
find_package(carma_cmake_common REQUIRED)
carma_check_ros_version(2)
carma_package()

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

# ── Find mosquitto library ──
find_library(MOSQUITTO_LIB mosquitto REQUIRED)
if(NOT MOSQUITTO_LIB)
  message(FATAL_ERROR "libmosquitto not found. Install with: apt install libmosquitto-dev")
endif()
message(STATUS "Found mosquitto: ${MOSQUITTO_LIB}")

# Name build targets
set(node_exec v2x_ros_driver_node_exec)
set(node_lib v2x_ros_driver_node)

# Includes
include_directories(
  include
)

ament_export_include_directories(include)


# Build
ament_auto_add_library(${node_lib} SHARED
        src/base_radio_client.cpp
        src/udp_radio_client.cpp
        src/mqtt_radio_client.cpp
        src/v2x_ros_driver_node.cpp
)

# Link mosquitto to the shared library
target_link_libraries(${node_lib}
        ${MOSQUITTO_LIB}
)

ament_auto_add_executable(${node_exec}
        src/main.cpp
)

# Register component
rclcpp_components_register_nodes(${node_lib} "V2XDriverApplication::Node")

# All locally created targets will need to be manually linked
# ament auto will handle linking of external dependencies
target_link_libraries(${node_exec}
        ${node_lib}
)


# Testing
if(BUILD_TESTING)

  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies() # This populates the ${${PROJECT_NAME}_FOUND_TEST_DEPENDS} variable

  ament_add_gtest(test_v2x_ros_driver test/node_test.cpp)

  ament_target_dependencies(test_v2x_ros_driver ${${PROJECT_NAME}_FOUND_TEST_DEPENDS})

  target_link_libraries(test_v2x_ros_driver ${node_lib})

endif()

# Install
ament_auto_package(
        INSTALL_TO_SHARE config etc launch
)