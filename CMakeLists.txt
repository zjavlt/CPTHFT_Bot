cmake_minimum_required(VERSION 3.10)
project(HFT_Bot)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(simdjson CONFIG REQUIRED)

add_executable(
    HFT_Bot 
    main.cpp)

target_link_libraries(HFT_Bot
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    simdjson::simdjson
)