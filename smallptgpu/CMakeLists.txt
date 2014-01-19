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

include_directories(../common)

PreprocessOCLKernel(rendering_kernel.cl)

set(SMALLPTGPU_SRCS
	smallptgpu.cpp
	)

add_executable(smallptgpu ${SMALLPTGPU_SRCS} preprocessed_rendering_kernel.cl)

TARGET_LINK_LIBRARIES(smallptgpu ocltoys_common ${GLUT_LIBRARY} ${OPENGL_LIBRARY} ${OPENCL_LIBRARIES} ${Boost_LIBRARIES})

# This instructs FREEGLUT to emit a pragma for the static version
SET_TARGET_PROPERTIES(smallptgpu PROPERTIES COMPILE_DEFINITIONS FREEGLUT_STATIC)
