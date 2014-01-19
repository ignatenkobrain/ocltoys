###########################################################################
#   Copyright (C) 1998-2013 by authors (see AUTHORS.txt )                 #
#                                                                         #
#   This file is part of OCLToys.                                         #
#                                                                         #
#   OCLToys is free software; you can redistribute it and/or modify       #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 3 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   OCLToys is distributed in the hope that it will be useful,            #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
#                                                                         #
#   OCLToys website: http://code.google.com/p/ocltoys                     #
###########################################################################

include(FindPkgMacros)
getenv_path(OCLToys_DEPENDENCIES_DIR)

#######################################################################
# Core dependencies
#######################################################################

# Find threading library
FIND_PACKAGE(Threads REQUIRED)

# Find Boost
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(BOOST_ROOT                  "${BOOST_SEARCH_PATH}")
#set(Boost_DEBUG                 ON)
set(Boost_MINIMUM_VERSION       "1.44.0")

set(Boost_ADDITIONAL_VERSIONS "1.47.0" "1.46.1" "1.46" "1.46.0" "1.45" "1.45.0" "1.44" "1.44.0")

set(OCLTOYS_BOOST_COMPONENTS thread filesystem system program_options regex)
find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${OCLTOYS_BOOST_COMPONENTS})
if (NOT Boost_FOUND)
        # Try again with the other type of libs
        if(Boost_USE_STATIC_LIBS)
                set(Boost_USE_STATIC_LIBS)
        else()
                set(Boost_USE_STATIC_LIBS OFF)
        endif()
        find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${OCLTOYS_BOOST_COMPONENTS})
endif()

if (Boost_FOUND)
	include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
	link_directories(${Boost_LIBRARY_DIRS})
	# Don't use old boost versions interfaces
	ADD_DEFINITIONS(-DBOOST_FILESYSTEM_NO_DEPRECATED)
endif ()

#######################################################################

# Find OpenGL
find_package(OpenGL)
if (OPENGL_FOUND)
	include_directories(SYSTEM ${OPENGL_INCLUDE_PATH})
endif()

#######################################################################

set(GLUT_ROOT                  "${GLUT_SEARCH_PATH}")
find_package(GLUT)

# GLUT
if (GLUT_FOUND)
	include_directories(SYSTEM ${GLUT_INCLUDE_PATH})
endif ()

#######################################################################

set(OPENCL_ROOT                  "${OPENCL_SEARCH_PATH}")
find_package(OpenCL)
# OpenCL
if (OPENCL_FOUND)
	include_directories(SYSTEM ${OPENCL_INCLUDE_DIR} ${OPENCL_C_INCLUDE_DIR})
endif ()
