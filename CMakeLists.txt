cmake_minimum_required(VERSION 3.18)

set (CMAKE_CXX_STANDARD 17)

set(LLVM_MOS_PLATFORM nes-nrom)
find_package(llvm-mos-sdk REQUIRED)

project(UWURANDOM LANGUAGES C ASM)

set(CMAKE_EXECUTABLE_SUFFIX .NES)

add_executable(UWURANDOM main.c chr-rom.s)
target_include_directories(UWURANDOM PRIVATE .)
target_link_libraries(UWURANDOM neslib)
set_property(SOURCE chr-rom.s PROPERTY
  OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Alpha.chr)