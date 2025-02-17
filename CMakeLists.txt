cmake_minimum_required (VERSION 3.11)

include(FetchContent)


set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
        GIT_TAG        v5.0.0
        GIT_PROGRESS   TRUE
        GIT_SHALLOW    TRUE
        EXCLUDE_FROM_ALL
        )
FetchContent_MakeAvailable(ftxui)

#FetchContent_GetProperties(ftxui)
#if(NOT ftxui_POPULATED)
#  FetchContent_Populate(ftxui)
#  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
#endif()

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.1
)

FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
        clip
        GIT_REPOSITORY https://github.com/dacap/clip.git
        GIT_TAG v1.9
)

FetchContent_GetProperties(clip)
if(NOT clip_POPULATED)
  FetchContent_Populate(clip)
  add_subdirectory(${clip_SOURCE_DIR} ${clip_BINARY_DIR})
endif()

#FetchContent_MakeAvailable(clip)

project(dmon
  LANGUAGES CXX
  VERSION 1.0.0
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g2 -ggdb -gdwarf -gsplit-dwarf")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -O0 -no-pie")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -fPIC -Ofast -no-pie")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -fPIC -Ofast -no-pie")

# diffusion
link_directories(${PROJECT_SOURCE_DIR}/diffusion/lib)
link_directories(${PROJECT_SOURCE_DIR}/diffusion/lib)
include_directories(${PROJECT_SOURCE_DIR}/diffusion/include)

#clip some hack
include_directories(${clip_SOURCE_DIR})

# openssl
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL 1.0.0 REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Boost COMPONENTS system REQUIRED)

add_executable(dmon
        src/main.cpp
  src/ui/log_displayer.cpp
  src/ui/log_displayer.hpp
  src/ui/main_component.cpp
  src/ui/main_component.hpp
  src/data/session.h
  src/data/session.cpp
)


#-------------------------------------------------------------------------------
# Environment variable available in C++ source
#-------------------------------------------------------------------------------

execute_process(
  COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE git_position
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE git_hash
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TIMESTAMP today "%Y-%m-%d ")

target_include_directories(dmon
  PRIVATE src
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen
)

target_link_libraries(dmon
  PRIVATE libdiffusion_exp.a
  PRIVATE pcre
  PRIVATE backtrace
  PRIVATE OpenSSL::SSL
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE spdlog::spdlog
  PRIVATE clip
)

set_target_properties(dmon PROPERTIES CXX_STANDARD 17)

# Add as many warning as possible:
if (MSVC)
  target_compile_options(dmon PRIVATE "/wd4244")
  target_compile_options(dmon PRIVATE "/wd4267")
  target_compile_options(dmon PRIVATE "/wd4996")
  target_compile_options(dmon PRIVATE "/wd4305")
else()
  target_compile_options(dmon PRIVATE "-Wall")
  target_compile_options(dmon PRIVATE "-Werror")
  target_compile_options(dmon PRIVATE "-Wno-sign-compare")
endif()


install(TARGETS dmon RUNTIME DESTINATION "bin")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Arthur Sonzogni")
include(CPack)
