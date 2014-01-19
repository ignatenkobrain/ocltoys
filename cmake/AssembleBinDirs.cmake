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
	set(CMAKE_INSTALL_PREFIX .)

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

###########################################################################
#
# LuxMark binary directory
#
###########################################################################

IF (WIN32)

ELSE (WIN32)

	set(LUXMARK_LINUX64_BIN_DIR "luxmark-linux64-v2.1beta2")
	set(LUXMARK_WIN32_BIN_DIR "luxmark-win32-v2.1beta2")

	# Win32
	add_custom_command(
	    OUTPUT "${LUXMARK_WIN32_BIN_DIR}"
	    COMMAND rm -rf ${LUXMARK_WIN32_BIN_DIR}
	    COMMAND mkdir ${LUXMARK_WIN32_BIN_DIR}
	    COMMAND cp -r samples/luxmark/scenes ${LUXMARK_WIN32_BIN_DIR}
		COMMAND rm -rf ${LUXMARK_WIN32_BIN_DIR}/scenes/room
	    COMMAND cp samples/luxmark/exe-32bit/*.* ${LUXMARK_WIN32_BIN_DIR}
		COMMAND mv ${LUXMARK_WIN32_BIN_DIR}/LuxMark.exe ${LUXMARK_WIN32_BIN_DIR}/LuxMark-win32.exe
	    COMMAND cp AUTHORS.txt COPYING.txt README.txt ${LUXMARK_WIN32_BIN_DIR}
	    COMMENT "Building ${LUXMARK_WIN32_BIN_DIR}")
	
	add_custom_command(
	    OUTPUT "${LUXMARK_WIN32_BIN_DIR}.zip"
	    COMMAND zip -r ${LUXMARK_WIN32_BIN_DIR}.zip ${LUXMARK_WIN32_BIN_DIR}
	    COMMAND rm -rf ${LUXMARK_WIN32_BIN_DIR}
	    DEPENDS ${LUXMARK_WIN32_BIN_DIR}
	    COMMENT "Building ${LUXMARK_WIN32_BIN_DIR}.zip")
	
	# Linux64
	add_custom_command(
	    OUTPUT "${LUXMARK_LINUX64_BIN_DIR}"
	    COMMAND rm -rf ${LUXMARK_LINUX64_BIN_DIR}
	    COMMAND mkdir ${LUXMARK_LINUX64_BIN_DIR}
	    COMMAND cp -r samples/luxmark/scenes ${LUXMARK_LINUX64_BIN_DIR}
	    COMMAND cp bin/luxmark ${LUXMARK_LINUX64_BIN_DIR}/luxmark-linux64
	    COMMAND cp AUTHORS.txt COPYING.txt README.txt ${LUXMARK_LINUX64_BIN_DIR}
		COMMAND cp samples/luxmark/linux-64bit/* ${LUXMARK_LINUX64_BIN_DIR}
	    COMMENT "Building ${LUXMARK_LINUX64_BIN_DIR}")

	add_custom_command(
	    OUTPUT "${LUXMARK_LINUX64_BIN_DIR}.zip"
	    COMMAND zip -r ${LUXMARK_LINUX64_BIN_DIR}.zip ${LUXMARK_LINUX64_BIN_DIR}
	    DEPENDS ${LUXMARK_LINUX64_BIN_DIR}
	    COMMENT "Building ${LUXMARK_LINUX64_BIN_DIR}.zip")
	
	add_custom_target(luxmark_all_zip DEPENDS "${LUXMARK_LINUX64_BIN_DIR}.zip" "${LUXMARK_WIN32_BIN_DIR}.zip")
ENDIF(WIN32)
