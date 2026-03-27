# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Include a secondary path for components
set(EXTRA_COMPONENT_DIRS ../../ESP/components)

idf_build_set_property(MINIMAL_BUILD ON)

project(SpinTX)
