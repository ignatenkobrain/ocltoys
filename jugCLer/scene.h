#ifndef SCENE_H
#define SCENE_H

/*****************************************************************************
 *                                 jugCLer                                   *
 *                realtime raytracing experiment: scene data                 *
 *                                                                           *
 *  Copyright (C) 2013  Holger Bettag               (hobold@vectorizer.org)  *
 *                                                                           *
 *  This program is free software; you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License along  *
 *  with this program; if not, write to the Free Software Foundation, Inc.,  *
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              *
 *****************************************************************************/

#include "ocltoy.h"

#include <math.h>
#include <algorithm>

const int NUMSPHERES = 84; // number of spheres in scene

/***********************
 *  vector arithmetic  *
 ***********************/
inline
void vecInit(cl_float3& t, const float X, const float Y, const float Z) {
	t.s[0] = X;
	t.s[1] = Y;
	t.s[2] = Z;
}

inline
void vecAdd(cl_float3& t, const cl_float3 v) { // increment
	t.s[0] += v.s[0];
	t.s[1] += v.s[1];
	t.s[2] += v.s[2];
}

inline
void vecSub(cl_float3& t, const cl_float3 v) { // decrement
	t.s[0] -= v.s[0];
	t.s[1] -= v.s[1];
	t.s[2] -= v.s[2];
}

inline
float vecDot(const cl_float3& t, const cl_float3& v) { // return dot product
	return t.s[0] * v.s[0] + t.s[1] * v.s[1] + t.s[2] * v.s[2];
}

inline
float vecLength(const cl_float3& t) { // return length
	return sqrtf(vecDot(t, t));
}

inline
void vecNormalize(cl_float3& t) { // set to unit length
	float reciLen = 1.0f / vecLength(t);
	t.s[0] *= reciLen;
	t.s[1] *= reciLen;
	t.s[2] *= reciLen;
}

// cross product

inline
void vecCross(cl_float3& t, const cl_float3 v1, const cl_float3 v2) {
	t.s[0] = v1.s[1] * v2.s[2] - v1.s[2] * v2.s[1];
	t.s[1] = v1.s[2] * v2.s[0] - v1.s[0] * v2.s[2];
	t.s[2] = v1.s[0] * v2.s[1] - v1.s[1] * v2.s[0];
}

inline
void vecScale(cl_float3& t, float s) { // scale
	t.s[0] *= s;
	t.s[1] *= s;
	t.s[2] *= s;
}

// weighted add

inline
void vecScaledAdd(cl_float3& t, const float s, const cl_float3 v) {
	t.s[0] += s * v.s[0];
	t.s[1] += s * v.s[1];
	t.s[2] += s * v.s[2];
}

/***************
 *  view rays  *
 ***************/
struct Ray {
	cl_float3 base;
	cl_float3 dir;
};

/************************
 *  perspective camera  *
 ************************/
struct Camera {
	cl_float3 eye; // eye point
	cl_float3 sky; // sky vector
	cl_float3 viewCenter; // center of view rect
	cl_float3 viewRight; // horizontal direction of view rect
	cl_float3 viewUp; // vertical direction of view rect
	cl_int imgWidth; // image width in pixels
	cl_int imgHeight; // image height in pixels
};

void camInit(Camera& c); // default initialization

// translate camera position
// does not maintain point of interest, may need to lookAt() afterwards
void camMove(Camera& c, const cl_float3& translation);

// aim camera at point of interest
void camLookAt(Camera& c, const cl_float3& poi);

void camUpdate(Camera& c);

/************
 *  sphere  *
 ************/
struct Sphere {
	// geometry of sphere
	cl_float3 center;
	cl_float radius;

	// material
	cl_float3 color; // pigment
	cl_float ambient; // ambient amount
	cl_float diffuse; // diffuse amount
	cl_float highlight; // highlight intensity
	cl_float roughness; // controls highlight size
	cl_float reflection; // weight of reflected ray
};

/*********************
 *  scene container  *
 *********************/
struct Scene {
	Camera cam; // the camera
	cl_float3 lightDir; // direction towards light
	cl_int numSpheres; // number of valid spheres
	Sphere spheres[84]; // storage space for the spheres
};

#endif   // SCENE_H
