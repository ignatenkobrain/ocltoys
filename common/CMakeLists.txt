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

set(OCLTOYS_VERSION_MAJOR 1)
set(OCLTOYS_VERSION_MINOR 0)

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  "version.h.in"
  "version.h"
  )

set(COMMONLIB_SRCS
	ocltoy.cpp
	utils.cpp
	)

add_library(ocltoys_common STATIC ${COMMONLIB_SRCS})

link_directories (${OCLToys_LIB_DIR}/common)
