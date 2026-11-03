# GP2040-CE for Linux USB gadget boards (Raspberry Pi 5 / CM5 / Pi 4 / CM4).
#
# This is a separate CMake project from the RP2040/RP2350 firmware in the
# repository root. It is never pulled into the Pico SDK build.
#
#   cmake -S linux -B build-linux && cmake --build build-linux
#   ctest --test-dir build-linux
cmake_minimum_required(VERSION 3.13)
project(gp2040-linux LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "gp2040-linux only builds for Linux targets")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(gp2040_linux_core STATIC
  src/config.cpp
  src/debounce.cpp
  src/gamepad_logic.cpp
  src/report_builder.cpp
  src/report_sender.cpp
)
# Reuse the firmware's HID descriptors and report structs.
target_include_directories(gp2040_linux_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../headers
)
target_compile_options(gp2040_linux_core PUBLIC -Wall -Wextra)

add_executable(gp2040-linux src/main.cpp src/gpio_input.cpp)
target_link_libraries(gp2040-linux PRIVATE gp2040_linux_core)

include(CTest)
if(BUILD_TESTING)
  add_executable(gp2040-linux-tests tests/test_core.cpp)
  target_link_libraries(gp2040-linux-tests PRIVATE gp2040_linux_core)
  add_test(NAME gp2040-linux-tests COMMAND gp2040-linux-tests)
endif()

install(TARGETS gp2040-linux DESTINATION bin)
install(PROGRAMS scripts/gp2040-gadget.sh DESTINATION bin RENAME gp2040-gadget)
install(FILES systemd/gp2040-gadget.service systemd/gp2040-linux.service
        DESTINATION lib/systemd/system)
