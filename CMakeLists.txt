cmake_minimum_required(VERSION 3.0)

project(lizyWebServer)

add_compile_options(-g -w -O2 -std=c++17)

file(GLOB SOURCE_CPPS 
  "./src/pool/*.cpp" 
  "./src/http/*.cpp" 
  "./src/server/*.cpp" 
  "./src/buffer/*.cpp"
  "./src/main.cpp" 
)

add_subdirectory(lizy_log)

add_subdirectory(lizy_timewheel)

add_executable(server
  ${SOURCE_CPPS}
)

target_link_libraries(server 
  pthread
  mysqlclient
  lizyLog
  lizyTimeWheel
)
