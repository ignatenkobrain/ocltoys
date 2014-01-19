/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of OCLToys.                                         *
 *                                                                         *
 *   OCLToys is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   OCLToys is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   OCLToys website: http://code.google.com/p/ocltoys                     *
 ***************************************************************************/

#ifndef UTILS_H
#define	UTILS_H

#include "opencl.h"

#include <string>
#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#include <stddef.h>
#include <sys/time.h>
#elif defined (WIN32)
#include <windows.h>
#else
#error "Unsupported Platform !!!"
#endif

#ifdef __MACOSX
#include <GLut/glut.h>
#else
#include <GL/glut.h>
#endif

extern std::string OCLErrorString(cl_int error);
extern std::string OCLLocalMemoryTypeString(cl_device_local_mem_type type);
extern std::string OCLDeviceTypeString(cl_device_type type);

extern void PrintString(void *font, const char *string);
extern void PrintHelpString(const unsigned int x, const unsigned int y,
		const char *key, const char *msg);
extern std::string ReadSources(const std::string &fileName);

inline double WallClockTime() {
#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__OpenBSD__) || defined(__FreeBSD__)
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (WIN32)
	LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
	LARGE_INTEGER ts;
	QueryPerformanceCounter(&ts);
	return ts.QuadPart / (double)freq.QuadPart;

	//return GetTickCount() / 1000.0;
#else
#error "Unsupported Platform !!!"
#endif
}

template <class T> inline T RoundUp(const T a, const T b) {
        const T r = a % b;
        if (r == 0)
                return a;
        else
                return a + b - r;
}

template <class T> inline T RoundUpPow2(T v) {
        v--;

        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;

        return v+1;
}

#endif	/* UTILS_H */

