###########################################################################
#
# Configuration
#
###########################################################################

#cmake -DOCLTOYS_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Dade-parallella.cmake .

MESSAGE(STATUS "Using Dade's Parallella Configuration settings")

#set(BOOST_SEARCH_PATH         "/home/david/projects/luxrender-dev/boost_1_43_0")

set(OPENCL_SEARCH_PATH        "/home/linaro/opt/browndeer")
set(OPENCL_INCLUDEPATH        "${OPENCL_SEARCH_PATH}/include")
set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib")

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")

ADD_DEFINITIONS(-DFREEGLUT_STATIC)
