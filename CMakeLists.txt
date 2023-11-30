cmake_minimum_required(VERSION 3.25)
project(earley)
enable_testing()

set(CMAKE_CXX_STANDARD 20)

add_library(libearley
  earley.cpp
  grammar.cpp
  src/grammar_util.cpp
  src/util.cpp)

add_library(fast
  src/fast/fast.cpp
  src/fast/items.cpp
  src/fast/grammar.cpp)

target_include_directories(libearley PUBLIC include)
target_include_directories(fast PUBLIC include)

target_link_libraries(fast libearley)

add_executable(earley main.cpp numbers.cpp)

target_link_libraries(earley PRIVATE libearley fast)

add_subdirectory(test)
