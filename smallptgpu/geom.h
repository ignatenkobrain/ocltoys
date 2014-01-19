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

#ifndef _GEOM_H
#define	_GEOM_H

#include "vec.h"

#define EPSILON 0.01f
#define FLOAT_PI 3.14159265358979323846f

typedef struct {
	Vec o, d;
} Ray;

#define rinit(r, a, b) { vassign((r).o, a); vassign((r).d, b); }
#define rassign(a, b) { vassign((a).o, (b).o); vassign((a).d, (b).d); }

typedef enum {
	MATTE, MIRROR, GLASS, MATTETRANSLUCENT, GLOSSY, GLOSSYTRANSLUCENT
} MaterialType; /* material types, used in radiance() */

typedef struct {
	float rad; /* radius */
	Vec p; // Position, emission
	Vec e; // Emission
	MaterialType matType;
	union {
		struct {
			Vec c;
		} matte;
		struct {
			Vec c;
		} mirror;
		struct {
			Vec c;
			float ior; // Index of refraction
			float sigmaS, sigmaA; // Volume rendering
		} glass;
		struct {
			Vec c;
			float transparency;
			float sigmaS, sigmaA; // Volume rendering
		} mattertranslucent;
		struct {
			Vec c;
			float exponent;
		} glossy;
		struct {
			Vec c;
			float exponent;
			float transparency;
			float sigmaS, sigmaA; // Volume rendering
		} glossytranslucent;
	};
} Sphere;

#endif	/* _GEOM_H */

