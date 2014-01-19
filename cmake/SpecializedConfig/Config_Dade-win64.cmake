###########################################################################
#
# Configuration
#
###########################################################################

# OCLTOYS_CUSTOM_CONFIG
# E:/projects/ocltoys-64bit/ocltoys/cmake/SpecializedConfig/Config_Dade-win64.cmake
# 
# Boost:
#  CALL "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64
#  bootstrap.bat
#  bjam --toolset=msvc address-model=64 --stagedir=stage stage -j 12
#
# To disable OpenMP support (VCOMP.lib error) in Visula Studio 2010 go
# properties => C/C++ => languages and disable OpenMP

MESSAGE(STATUS "Using Dade's Win64 Configuration settings")

set(CMAKE_BUILD_TYPE "Release")

set(BOOST_SEARCH_PATH         "E:/projects/ocltoys-64bit/boost_1_48_0")
set(BOOST_LIBRARYDIR          "${BOOST_SEARCH_PATH}/stage/boost/lib")

set(OPENCL_SEARCH_PATH        "C:/Program Files (x86)/AMD APP")
set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib/x86")

# For GLUT with sources
set(GLUT_SEARCH_PATH          "E:/projects/ocltoys-64bit/freeglut-2.8.0")
set(GLUT_LIBRARYDIR           "${GLUT_SEARCH_PATH}/lib/x86")
ADD_DEFINITIONS(-DFREEGLUT_STATIC)
# For GLUT with binary-only
#set(GLUT_SEARCH_PATH          "E:/projects/ocltoys-64bit/freeglut-bin")
#set(GLUT_LIBRARYDIR           "${GLUT_SEARCH_PATH}/lib")
