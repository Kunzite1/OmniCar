cmake_minimum_required(VERSION 3.8)
project(car_control LANGUAGES C CXX)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/CanFrame.msg"
)

add_library(can_protocol STATIC
  src/can_protocol.c
)
target_include_directories(can_protocol
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(can_protocol PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(can_communication SHARED
  src/can_communication_node.cpp
)
target_include_directories(can_communication
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(can_communication rclcpp)
target_link_libraries(can_communication can_protocol)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)
target_link_libraries(can_communication "${cpp_typesupport_target}")

add_executable(can_communication_node
  src/can_communication_main.cpp
)
ament_target_dependencies(can_communication_node rclcpp)
target_link_libraries(can_communication_node can_communication)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(can_protocol PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(can_communication PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(can_communication_node PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
  add_executable(test_can_protocol test/test_can_protocol.c)
  target_link_libraries(test_can_protocol can_protocol)
  add_test(NAME test_can_protocol COMMAND test_can_protocol)

  add_executable(test_can_tx_queue test/test_can_tx_queue.cpp)
  target_include_directories(test_can_tx_queue PRIVATE include)
  add_test(NAME test_can_tx_queue COMMAND test_can_tx_queue)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_can_protocol PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_can_tx_queue PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

install(
  TARGETS can_protocol can_communication can_communication_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
install(FILES README.md DESTINATION share/${PROJECT_NAME})

ament_export_include_directories(include)
ament_export_libraries(can_protocol can_communication)
ament_export_dependencies(rclcpp rosidl_default_runtime)

ament_package()
