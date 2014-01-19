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

###########################################################################
#
# Configuration
#
# Use cmake "-DOCLTOYS_CUSTOM_CONFIG=YouFileCName" To define your personal settings
# where YouFileCName.cname must exist in one of the cmake include directories
# best to use cmake/SpecializedConfig/
# 
# To not load defaults before loading custom values define
# -DOCLTOYS_NO_DEFAULT_CONFIG=true
#
# WARNING: These variables will be cached like any other
#
###########################################################################

IF (NOT OCLTOYS_NO_DEFAULT_CONFIG)

  # Disable Boost automatic linking
  ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)

  IF (WIN32)

    MESSAGE(STATUS "Using default WIN32 Configuration settings")

    IF(MSVC)

      STRING(REGEX MATCH "(Win64)" _carch_x64 ${CMAKE_GENERATOR})
      IF(_carch_x64)
        SET(WINDOWS_ARCH "x64")
      ELSE(_carch_x64)
        SET(WINDOWS_ARCH "x86")
      ENDIF(_carch_x64)
      MESSAGE(STATUS "Building for target ${WINDOWS_ARCH}")

    ENDIF(MSVC)

	SET(BOOST_SEARCH_PATH         "${OclToys_SOURCE_DIR}/../boost")
	SET(OPENCL_SEARCH_PATH        "${OclToys_SOURCE_DIR}/../opencl")
	SET(GLUT_SEARCH_PATH          "${OclToys_SOURCE_DIR}/../freeglut")

  ENDIF(WIN32)

ELSE(NOT OCLTOYS_NO_DEFAULT_CONFIG)
	
	MESSAGE(STATUS "OCLTOYS_NO_DEFAULT_CONFIG defined - not using default configuration values.")

ENDIF(NOT OCLTOYS_NO_DEFAULT_CONFIG)

#
# Overwrite defaults with Custom Settings
#
IF (OCLTOYS_CUSTOM_CONFIG)
	MESSAGE(STATUS "Using custom build config: ${OCLTOYS_CUSTOM_CONFIG}")
	INCLUDE(${OCLTOYS_CUSTOM_CONFIG})
ENDIF (OCLTOYS_CUSTOM_CONFIG)
