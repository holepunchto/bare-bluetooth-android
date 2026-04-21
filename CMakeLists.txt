cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-android REQUIRED PATHS node_modules/cmake-android)

project(bare_bluetooth_android C CXX)

use_java()

fetch_package("github:holepunchto/libjstl")
fetch_package("github:holepunchto/libjnitl")

add_bare_module(bare_bluetooth_android)

target_sources(
  ${bare_bluetooth_android}
  PRIVATE
    binding.cc
)

target_link_libraries(
  ${bare_bluetooth_android}
  PUBLIC
    jstl
    jnitl
)

set_target_properties(
  ${bare_bluetooth_android}
  PROPERTIES
  C_STANDARD 11
  CXX_STANDARD 20
  CXX_SCAN_FOR_MODULES OFF
  POSITION_INDEPENDENT_CODE ON
)

find_android_jar(android_jar)

set(CMAKE_JAVA_COMPILE_FLAGS --release 17)

add_dex(
  bare_bluetooth_classes
  SOURCES
    lib/ScanCallback.java
    lib/GattCallback.java
    lib/GattServerCallback.java
    lib/AdvertiseCallback.java
  INCLUDE_JARS
    ${android_jar}
)

bare_target(target)
bare_module_target("." _module NAME _name)

install(
  FILES
    $<TARGET_PROPERTY:bare_bluetooth_classes,JAR_FILE>
    $<TARGET_PROPERTY:bare_bluetooth_classes,DEX_FILE>
  DESTINATION ${target}/${_name}
)
