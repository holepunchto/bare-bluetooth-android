cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(bare_bluetooth_android C CXX)

fetch_package("github:holepunchto/libjstl")
fetch_package("github:holepunchto/libjnitl#classloader-fallback")

find_package(Java)

include(UseJava)

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
    android
)

set_target_properties(
  ${bare_bluetooth_android}
  PROPERTIES
  C_STANDARD 11
  CXX_STANDARD 20
  POSITION_INDEPENDENT_CODE ON
)

if(NOT ANDROID_HOME)
  set(ANDROID_HOME $ENV{ANDROID_HOME})
endif()

if(NOT ANDROID_PLATFORM)
  file(GLOB _android_platforms LIST_DIRECTORIES true "${ANDROID_HOME}/platforms/android-*")
  list(SORT _android_platforms)
  list(POP_BACK _android_platforms ANDROID_PLATFORM)
  get_filename_component(ANDROID_PLATFORM "${ANDROID_PLATFORM}" NAME)
endif()

set(ANDROID_JAR ${ANDROID_HOME}/platforms/${ANDROID_PLATFORM}/android.jar)

set(CMAKE_JAVA_COMPILE_FLAGS --release 17)

add_jar(
  bare_bluetooth_classes
  SOURCES
    lib/ScanCallback.java
    lib/GattCallback.java
    lib/GattServerCallback.java
    lib/AdvertiseCallback.java
  INCLUDE_JARS
    ${ANDROID_JAR}
  OUTPUT_NAME classes
)

find_program(d8 d8 PATHS ${ANDROID_HOME}/build-tools/* NO_DEFAULT_PATH)

add_custom_command(
  TARGET bare_bluetooth_classes
  POST_BUILD
  COMMAND
    ${d8}
      $<IF:$<CONFIG:Debug>,--debug,--release>
      --lib ${ANDROID_JAR}
      --output ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}/classes.jar
)

bare_target(target)
bare_module_target("." _module NAME _name)

install(
  FILES
    ${CMAKE_BINARY_DIR}/classes.dex
  DESTINATION ${target}/${_name}
)
