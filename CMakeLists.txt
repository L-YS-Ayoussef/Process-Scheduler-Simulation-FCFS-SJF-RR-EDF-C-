cmake_minimum_required(VERSION 3.16)
project(ProcessScheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ProcessScheduler
  src/main.cpp
  src/core/Scheduler.cpp
  src/io/InputParser.cpp
  src/model/Process.cpp
  src/processors/Processor.cpp
  src/processors/FCFSProcessor.cpp
  src/processors/RRProcessor.cpp
  src/processors/SJFProcessor.cpp
)

target_include_directories(ProcessScheduler PRIVATE src)
