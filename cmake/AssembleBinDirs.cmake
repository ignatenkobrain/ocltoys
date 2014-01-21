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
# Binary samples directory
#
###########################################################################

IF (WIN32)
	
  # For MSVC moving exe files gets done automatically
  # If there is someone compiling on windows and
  # not using msvc (express is free) - feel free to implement

ELSE (WIN32)
	if(NOT CMAKE_INSTALL_PREFIX)
		set(CMAKE_INSTALL_PREFIX .)
		set(PACKAGE_DATADIR ./)
	else(NOT CMAKE_INSTALL_PREFIX)
		set(PACKAGE_DATADIR ${CMAKE_INSTALL_PREFIX}/share/ocltoys/)
	endif(NOT CMAKE_INSTALL_PREFIX)


	# Windows 64bit
	set(OCLTOYS_BIN_WIN64_DIR "ocltoys-win64-v1.0")
	add_custom_command(
		OUTPUT "${OCLTOYS_BIN_WIN64_DIR}"
		COMMAND rm -rf ${OCLTOYS_BIN_WIN64_DIR}
		COMMAND mkdir ${OCLTOYS_BIN_WIN64_DIR}
		COMMAND cp AUTHORS.txt COPYING.txt README.txt ${OCLTOYS_BIN_WIN64_DIR}
		COMMAND cp scripts/win/*.BAT ${OCLTOYS_BIN_WIN64_DIR}
		# Common
		COMMAND mkdir ${OCLTOYS_BIN_WIN64_DIR}/common
		COMMAND cp common/vec.h common/camera.h ${OCLTOYS_BIN_WIN64_DIR}/common
		# MandelGPU
		COMMAND mkdir ${OCLTOYS_BIN_WIN64_DIR}/mandelgpu
		COMMAND cp mandelgpu/README.txt ${OCLTOYS_BIN_WIN64_DIR}/mandelgpu
		COMMAND cp mandelgpu/*.cl ${OCLTOYS_BIN_WIN64_DIR}/mandelgpu
		COMMAND cp /media/windata/projects/ocltoys-64bit/ocltoys-bin/mandelgpu/Release/mandelgpu.exe ${OCLTOYS_BIN_WIN64_DIR}/mandelgpu
		# JuliaGPU
		COMMAND mkdir ${OCLTOYS_BIN_WIN64_DIR}/juliagpu
		COMMAND cp juliagpu/README.txt ${OCLTOYS_BIN_WIN64_DIR}/juliagpu
		COMMAND cp juliagpu/*.cl ${OCLTOYS_BIN_WIN64_DIR}/juliagpu
		COMMAND cp /media/windata/projects/ocltoys-64bit/ocltoys-bin/juliagpu/Release/juliagpu.exe ${OCLTOYS_BIN_WIN64_DIR}/juliagpu
		# SmallPTGPU
		COMMAND mkdir ${OCLTOYS_BIN_WIN64_DIR}/smallptgpu
		COMMAND mkdir ${OCLTOYS_BIN_WIN64_DIR}/smallptgpu/scenes
		COMMAND cp smallptgpu/README.txt ${OCLTOYS_BIN_WIN64_DIR}/smallptgpu
		COMMAND cp smallptgpu/*.cl ${OCLTOYS_BIN_WIN64_DIR}/smallptgpu
		COMMAND cp smallptgpu/scenes/*.scn ${OCLTOYS_BIN_WIN64_DIR}/smallptgpu/scenes
		COMMAND cp /media/windata/projects/ocltoys-64bit/ocltoys-bin/smallptgpu/Release/smallptgpu.exe ${OCLTOYS_BIN_WIN64_DIR}/smallptgpu
		COMMENT "Building ${OCLTOYS_BIN_WIN64_DIR}")

	add_custom_command(
		OUTPUT "${OCLTOYS_BIN_WIN64_DIR}.zip"
		COMMAND zip -r ${OCLTOYS_BIN_WIN64_DIR}.zip ${OCLTOYS_BIN_WIN64_DIR}
		COMMAND rm -rf ${OCLTOYS_BIN_WIN64_DIR}
		DEPENDS ${OCLTOYS_BIN_WIN64_DIR}
		COMMENT "Building ${OCLTOYS_BIN_WIN64_DIR}.zip")

	# Linux 64bit
	set(OCLTOYS_BIN_LINUX64_DIR "ocltoys-linux64-v1.0")
	add_custom_command(
		OUTPUT "${OCLTOYS_BIN_LINUX64_DIR}"
		COMMAND rm -rf ${OCLTOYS_BIN_LINUX64_DIR}
		COMMAND mkdir ${OCLTOYS_BIN_LINUX64_DIR}
		COMMAND cp AUTHORS.txt COPYING.txt README.txt ${OCLTOYS_BIN_LINUX64_DIR}
		# Common
		COMMAND mkdir ${OCLTOYS_BIN_LINUX64_DIR}/common
		COMMAND cp common/vec.h common/camera.h ${OCLTOYS_BIN_LINUX64_DIR}/common
		# MandelGPU
		COMMAND mkdir ${OCLTOYS_BIN_LINUX64_DIR}/mandelgpu
		COMMAND cp mandelgpu/README.txt ${OCLTOYS_BIN_LINUX64_DIR}/mandelgpu
		COMMAND cp mandelgpu/*.cl ${OCLTOYS_BIN_LINUX64_DIR}/mandelgpu
		COMMAND cp mandelgpu/mandelgpu ${OCLTOYS_BIN_LINUX64_DIR}/mandelgpu
		# JuliaGPU
		COMMAND mkdir ${OCLTOYS_BIN_LINUX64_DIR}/juliagpu
		COMMAND cp juliagpu/README.txt ${OCLTOYS_BIN_LINUX64_DIR}/juliagpu
		COMMAND cp juliagpu/*.cl ${OCLTOYS_BIN_LINUX64_DIR}/juliagpu
		COMMAND cp juliagpu/juliagpu ${OCLTOYS_BIN_LINUX64_DIR}/juliagpu
		# SmallPTGPU
		COMMAND mkdir ${OCLTOYS_BIN_LINUX64_DIR}/smallptgpu
		COMMAND mkdir ${OCLTOYS_BIN_LINUX64_DIR}/smallptgpu/scenes
		COMMAND cp smallptgpu/README.txt ${OCLTOYS_BIN_LINUX64_DIR}/smallptgpu
		COMMAND cp smallptgpu/*.cl ${OCLTOYS_BIN_LINUX64_DIR}/smallptgpu
		COMMAND cp smallptgpu/scenes/*.scn ${OCLTOYS_BIN_LINUX64_DIR}/smallptgpu/scenes
		COMMAND cp smallptgpu/smallptgpu ${OCLTOYS_BIN_LINUX64_DIR}/smallptgpu
		COMMENT "Building ${OCLTOYS_BIN_LINUX64_DIR}")

	add_custom_command(
		OUTPUT "${OCLTOYS_BIN_LINUX64_DIR}.zip"
		COMMAND zip -r ${OCLTOYS_BIN_LINUX64_DIR}.zip ${OCLTOYS_BIN_LINUX64_DIR}
		COMMAND rm -rf ${OCLTOYS_BIN_LINUX64_DIR}
		DEPENDS ${OCLTOYS_BIN_LINUX64_DIR}
		COMMENT "Building ${OCLTOYS_BIN_LINUX64_DIR}.zip")

	add_custom_target(ocltoys_win64_zip DEPENDS "${OCLTOYS_BIN_WIN64_DIR}.zip")
	add_custom_target(ocltoys_linux64_zip DEPENDS "${OCLTOYS_BIN_LINUX64_DIR}.zip")
ENDIF(WIN32)
