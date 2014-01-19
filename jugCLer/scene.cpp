/*****************************************************************************
 *                                 jugCLer                                   *
 *              realtime raytracing experiment: camera methods               *
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

#include "scene.h"

void camInit(Camera& c) { // default initialization
	vecInit(c.eye, 0.0f, 0.0f, 3.0f); // 3 units in front of screen
	vecInit(c.sky, 0.0f, 1.0f, 0.0f); // sky, used to align viewUp
	vecInit(c.viewCenter, 0.0f, 0.0f, 0.0f); // screen center at origin
	camUpdate(c);
}

// translate camera position
// does not maintain point of interest, may need to lookAt() afterwards

void camMove(Camera& c, const cl_float3& translation) {
	vecAdd(c.eye, translation);
	vecAdd(c.viewCenter, translation);
}

// aim camera at point of interest

void camLookAt(Camera& c, const cl_float3& poi) {
	// first derive some invariant scalar measures of the current camera
	cl_float3 forward = c.viewCenter;
	vecSub(forward, c.eye);
	float distance = vecLength(forward);

	// now rebuild the camera
	forward = poi;
	vecSub(forward, c.eye);
	vecNormalize(forward);
	vecScale(forward, distance);
	c.viewCenter = c.eye;
	vecAdd(c.viewCenter, forward); // from eye towards point of interest

	camUpdate(c);
}

void camUpdate(Camera& c) {
	cl_float3 forward = c.viewCenter;
	vecSub(forward, c.eye);

	vecCross(c.viewRight, forward, c.sky); // horizon is perpendicular to sky
	vecNormalize(c.viewRight);

	vecCross(c.viewUp, c.viewRight, forward); // perpendicular 2 horizon & forward
	vecNormalize(c.viewUp);
}