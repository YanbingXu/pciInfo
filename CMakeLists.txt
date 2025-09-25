cmake_minimum_required(VERSION 4.0)
project(pciInfo)

set(CMAKE_CXX_STANDARD 20)

add_executable(pciInfo main.cpp)

# 链接 macOS frameworks
target_link_libraries(pciInfo
        "-framework IOKit"
        "-framework CoreFoundation"
)