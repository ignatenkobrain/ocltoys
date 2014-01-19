/*****************************************************************************
 *                                 jugCLer                                   *
 *            realtime raytracing experiment: animation routines             *
 *                                                                           *
 *  Copyright (C) 2013  Holger Bettag               (hobold@vectorizer.org)  *
 *  Substantial parts of this code were originally written by Michael        *
 *  Birken in 2010, when he resurrected Eric Graham's original juggler. It   *
 *  is being used here with Michael's permission.                            *
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

#include "animation.h"

/************************
 *  animation routines  *
 ************************/

double sceneTimeOffset = WallClockTime();

// move camera

void updateCamera(Scene* scene) {
	double time = WallClockTime() - sceneTimeOffset;
	double rtime = (time / 21.35634); // camera rotation time
	rtime = rtime - floor(rtime);

	double btime = (time / 59.8752); // camera bobbing time
	btime = btime - floor(btime);

	double ltime = (time / 98.7654); // light rotation time
	ltime = ltime - floor(ltime);

	double lbtime = (time / 92.764); // light bobbing time
	lbtime = lbtime - floor(lbtime);

	camInit(scene->cam);
	cl_float3 pos;
	cl_float3 center;
	vecInit(center, 14.9f, 9.7f, -14.85f);
	vecInit(pos, 33.0f * sin(rtime * M_PI * 2.0),
			8.0f * sin(btime * M_PI * 2.0 + 0.3),
			33.0f * (cos(rtime * M_PI * 2.0) - 0.11));
	vecAdd(pos, center);
	camMove(scene->cam, pos);
	camLookAt(scene->cam, center);

	vecInit(pos, 5.0f * sin(-ltime * M_PI * 2.0),
			8.0f + 6.0f * sin(lbtime * M_PI * 2.0),
			5.0f * cos(ltime * M_PI * 2.0));
	vecNormalize(pos);
	scene->lightDir = pos;
}

// place a string of spheres

void updateAppendage(Scene* scene, int sceneIndex, cl_float3 p, cl_float3 q,
		cl_float3 w, float A, float B, int countA, int countB) {
	cl_float3 U;
	cl_float3 V;
	cl_float3 W;

	cl_float3 j;
	cl_float3 d;

	V = q;
	vecSub(V, p);

	float D = vecLength(V);
	float inverseD = 1.0f / D;
	vecScale(V, inverseD);

	W = w;
	vecNormalize(W);
	vecCross(U, V, W);

	float A2 = A*A;

	float y = 0.5f * inverseD * (A2 - B * B + D * D);
	double square = A2 - y*y;
	if (square < 0.0f)
		throw std::runtime_error("Unable to construct appendage");
	float x = sqrtf(square);

	j = p;
	vecScaledAdd(j, x, U);
	vecScaledAdd(j, y, V);

	d = j;
	vecSub(d, p);
	vecScale(d, 1.0f / float(countA));
	for (int i = 0; i <= countA; i++) {
		Sphere* sphere = &(scene->spheres[sceneIndex + i]);
		sphere->center = p;
		vecScaledAdd(sphere->center, float(i), d);
		vecScale(sphere->center, 0.1f);
	}

	d = j;
	vecSub(d, q);
	vecScale(d, 1.0f / float(countB));
	for (int i = 0; i <= countA; i++) {
		Sphere* sphere = &(scene->spheres[sceneIndex + i + countA + 1]);
		sphere->center = q;
		vecScaledAdd(sphere->center, float(i), d);
		vecScale(sphere->center, 0.1f);
	}
}

// compute scene state for current time

void animatePositions(Scene* scene, const bool updateCam) {
	if (updateCam)
		updateCamera(scene);

	// update sphere positions
	const float JUGGLE_X0 = -182.0f;
	const float JUGGLE_X1 = -108.0f;
	const float JUGGLE_Y0 = 88.0f;
	const float JUGGLE_H_Y = 184.0f;

	const float JUGGLE_H_VX = (JUGGLE_X0 - JUGGLE_X1) / 60.0f;
	const float JUGGLE_L_VX = (JUGGLE_X1 - JUGGLE_X0) / 30.0f;

	const float JUGGLE_H_H = JUGGLE_H_Y - JUGGLE_Y0;
	const float JUGGLE_H_VY = 4.0f * JUGGLE_H_H / 60.0f;
	const float JUGGLE_G = JUGGLE_H_VY * JUGGLE_H_VY / (2.0f * JUGGLE_H_H);

	const float JUGGLE_L_VY = 0.5f * JUGGLE_G * 30.0f;

	const float HIPS_MAX_Y = 85.0f;
	const float HIPS_MIN_Y = 81.0f;

	const float HIPS_ANGLE_MULTIPLIER = 2.0f * M_PI / 30.0f;

	double time = WallClockTime() - sceneTimeOffset;
	time -= floor(time);

	// mirrored juggling balls
	float t = 30.0f + 30.0f * float(time);
	Sphere* sphere = &(scene->spheres[1]);
	sphere->center.s[2] = 0.1f * (JUGGLE_X1 + JUGGLE_H_VX * t);
	sphere->center.s[1] = 0.1f * (JUGGLE_Y0 + (JUGGLE_H_VY - 0.5f * JUGGLE_G * t) * t);

	t -= 30.0f;
	sphere = &(scene->spheres[2]);
	sphere->center.s[2] = 0.1f * (JUGGLE_X1 + JUGGLE_H_VX * t);
	sphere->center.s[1] = 0.1f * (JUGGLE_Y0 + (JUGGLE_H_VY - 0.5f * JUGGLE_G * t) * t);

	sphere = &(scene->spheres[0]);
	sphere->center.s[2] = 0.1f * (JUGGLE_X0 + JUGGLE_L_VX * t);
	sphere->center.s[1] = 0.1f * (JUGGLE_Y0 + (JUGGLE_L_VY - 0.5f * JUGGLE_G * t) * t);

	// body (hips to chest) 3 .. 10
	float angle = HIPS_ANGLE_MULTIPLIER*t;
	float oscillation = 0.5f * (1.0f + cosf(angle));

	cl_float3 o;
	vecInit(o, 151.0f,
			HIPS_MIN_Y + (HIPS_MAX_Y - HIPS_MIN_Y) * oscillation,
			-151.0f);
	cl_float3 v;
	vecInit(v, 0.0f, 70.0f, (HIPS_MIN_Y - HIPS_MAX_Y) * sinf(angle));
	vecNormalize(v);

	cl_float3 u;
	vecInit(u, 0.0f, v.s[2], -v.s[1]);

	cl_float3 w;
	vecInit(w, 1.0f, 0.0f, 0.0f);

	for (int i = 3; i <= 10; i++) {
		float fraction = float(i - 3) / 7.0f;
		sphere = &(scene->spheres[i]);
		sphere->center = o;
		vecScaledAdd(sphere->center, 32.0f * fraction, v);
		vecScale(sphere->center, 0.1f);
	}

	// head
	sphere = &(scene->spheres[11]);
	sphere->center = o;
	vecScaledAdd(sphere->center, 70.0f, v);
	vecScale(sphere->center, 0.1f);

	// neck
	sphere = &(scene->spheres[12]);
	sphere->center = o;
	vecScaledAdd(sphere->center, 55.0f, v);
	vecScale(sphere->center, 0.1f);

	// left leg (13 .. 29)
	cl_float3 p;
	vecInit(p, 159.0f, 2.5f, -133.0f);
	cl_float3 q = o;
	vecScaledAdd(q, -9.0f, v);
	vecScaledAdd(q, -16.0f, u);
	updateAppendage(scene, 13, p, q, u, 42.58f, 34.07f, 8, 8);

	// right leg (30 .. 46)
	vecInit(p, 139.0f, 2.5f, -164.0f);
	q = o;
	vecScaledAdd(q, -9.0f, v);
	vecScaledAdd(q, 16.0f, u);
	updateAppendage(scene, 30, p, q, u, 42.58f, 34.07f, 8, 8);

	// left arm (47 .. 63)
	cl_float3 n;
	float armAngle = -0.35f * oscillation;
	vecInit(p, 69.0f + 41.0f * cosf(armAngle),
			60.0f - 41.0f * sinf(armAngle),
			-108.0f);
	q = o;
	vecScaledAdd(q, 45.0f, v);
	vecScaledAdd(q, -19.0f, u);
	n = o;
	vecScaledAdd(n, 45.41217f, v);
	vecScaledAdd(n, -19.91111f, u);
	vecSub(n, q);
	updateAppendage(scene, 47, p, q, n, 44.294f, 46.098f, 8, 8);

	// right arm (64 .. 80)
	p.s[2] = -182.0f;
	q = o;
	vecScaledAdd(q, 45.0f, v);
	vecScaledAdd(q, 19.0f, u);
	n = o;
	vecScaledAdd(n, 45.41217f, v);
	vecScaledAdd(n, 19.91111f, u);
	vecSub(n, q);
	vecScale(n, -1.0f);
	updateAppendage(scene, 64, p, q, n, 44.294f, 46.098f, 8, 8);

	// left eye (81)
	sphere = &(scene->spheres[81]);
	sphere->center = o;
	vecScaledAdd(sphere->center, 69.0f, v);
	vecScaledAdd(sphere->center, -7.0f, u);
	sphere->center.s[0] = 142.0f;
	vecScale(sphere->center, 0.1f);

	// left eye (82)
	sphere = &(scene->spheres[82]);
	sphere->center = o;
	vecScaledAdd(sphere->center, 69.0f, v);
	vecScaledAdd(sphere->center, 7.0f, u);
	sphere->center.s[0] = 142.0f;
	vecScale(sphere->center, 0.1f);

	// hair (83)
	sphere = &(scene->spheres[83]);
	sphere->center = o;
	vecScaledAdd(sphere->center, 71.0f, v);
	sphere->center.s[0] = 152.0f;
	vecScale(sphere->center, 0.1f);
}

// initialize scene state

void setupAnim(Scene* scene, int imgWidth, int imgHeight) {

	// Positions Everybody!
	scene->numSpheres = NUMSPHERES;

	// three mirrored juggling spheres
	for (int i = 0; i <= 2; i++) {
		scene->spheres[i].radius = 1.4f;
		vecInit(scene->spheres[i].color, 0.5f, 0.5f, 0.5f);
		scene->spheres[i].ambient = 0.1f;
		scene->spheres[i].diffuse = 0.3f;
		scene->spheres[i].highlight = 0.8f;
		scene->spheres[i].roughness = 0.05f;
		scene->spheres[i].reflection = 0.8f;
	}
	vecInit(scene->spheres[0].center, 11.0f, 0.0f, 0.0f);
	vecInit(scene->spheres[1].center, 11.0f, 0.0f, 0.0f);
	vecInit(scene->spheres[2].center, 11.0f, 0.0f, 0.0f);

	// torso: seven spheres
	for (int i = 3; i <= 10; i++) {
		float fraction = float(i - 3) / 7.0f;
		scene->spheres[i].radius = 1.6f + 0.4f * fraction;
		vecInit(scene->spheres[i].center, 15.1f, 8.5f + 3.2f * fraction, -15.1f);
		vecInit(scene->spheres[i].color, 0.843f, 0.12f, 0.11f);
		scene->spheres[i].ambient = 0.3f;
		scene->spheres[i].diffuse = 0.8f;
		scene->spheres[i].highlight = 1.0f;
		scene->spheres[i].roughness = 0.1f;
		scene->spheres[i].reflection = 0.0f;
	}

	// head
	scene->spheres[11].radius = 1.4;
	vecInit(scene->spheres[11].center, 15.1f, 15.5f, -15.1f);
	vecInit(scene->spheres[11].color, 0.95f, 0.64f, 0.63f);
	scene->spheres[11].ambient = 0.25f;
	scene->spheres[11].diffuse = 0.9f;
	scene->spheres[11].highlight = 1.0f;
	scene->spheres[11].roughness = 0.1f;
	scene->spheres[11].reflection = 0.0f;

	// neck
	scene->spheres[12].radius = 0.5;
	vecInit(scene->spheres[12].center, 15.1f, 14.0f, -15.1f);
	vecInit(scene->spheres[12].color, 0.95f, 0.64f, 0.63f);
	scene->spheres[12].ambient = 0.25f;
	scene->spheres[12].diffuse = 0.9f;
	scene->spheres[12].highlight = 1.0f;
	scene->spheres[12].roughness = 0.1f;
	scene->spheres[12].reflection = 0.0f;

	// outer limb parts
	for (int i = 13; i <= 20; i++) {
		for (int j = 0; j < 4; j++) {
			scene->spheres[i + 17 * j].radius = 0.25f + 0.25f * float(i - 13) / 7.0f;
			vecInit(scene->spheres[i + 17 * j].center, 0.0f, 0.0f, 0.0f);
			vecInit(scene->spheres[i + 17 * j].color, 0.95f, 0.64f, 0.63f);
			scene->spheres[i + 17 * j].ambient = 0.25f;
			scene->spheres[i + 17 * j].diffuse = 0.9f;
			scene->spheres[i + 17 * j].highlight = 1.0f;
			scene->spheres[i + 17 * j].roughness = 0.1f;
			scene->spheres[i + 17 * j].reflection = 0.0f;
		}
	}

	// inner limb parts
	for (int i = 21; i <= 29; i++) {
		for (int j = 0; j < 4; j++) {
			scene->spheres[i + 17 * j].radius = 0.5f;
			vecInit(scene->spheres[i + 17 * j].center, 0.0f, 0.0f, 0.0f);
			vecInit(scene->spheres[i + 17 * j].color, 0.95f, 0.64f, 0.63f);
			scene->spheres[i + 17 * j].ambient = 0.25f;
			scene->spheres[i + 17 * j].diffuse = 0.9f;
			scene->spheres[i + 17 * j].highlight = 1.0f;
			scene->spheres[i + 17 * j].roughness = 0.1f;
			scene->spheres[i + 17 * j].reflection = 0.0f;
		}
	}

	// eyes
	scene->spheres[81].radius = 0.4;
	vecInit(scene->spheres[81].center, 14.2f, 15.4f, -14.4f);
	vecInit(scene->spheres[81].color, 0.121f, 0.105f, 0.58f);
	scene->spheres[81].ambient = 0.25f;
	scene->spheres[81].diffuse = 0.9f;
	scene->spheres[81].highlight = 1.0f;
	scene->spheres[81].roughness = 0.1f;
	scene->spheres[81].reflection = 0.0f;

	scene->spheres[82].radius = 0.4;
	vecInit(scene->spheres[82].center, 14.2f, 15.4f, -14.4f);
	vecInit(scene->spheres[82].color, 0.121f, 0.105f, 0.58f);
	scene->spheres[82].ambient = 0.25f;
	scene->spheres[82].diffuse = 0.9f;
	scene->spheres[82].highlight = 1.0f;
	scene->spheres[82].roughness = 0.1f;
	scene->spheres[82].reflection = 0.0f;

	// hair
	scene->spheres[83].radius = 1.4;
	vecInit(scene->spheres[83].center, 15.2f, 15.4f, -15.1f);
	vecInit(scene->spheres[83].color, 0.15f, 0.066f, 0.09f);
	scene->spheres[83].ambient = 0.25f;
	scene->spheres[83].diffuse = 0.9f;
	scene->spheres[83].highlight = 1.0f;
	scene->spheres[83].roughness = 0.1f;
	scene->spheres[83].reflection = 0.0f;

	// Lights!
	vecInit(scene->lightDir, -56.4f, 68.6f, 34.7f);
	vecNormalize(scene->lightDir);

	// Camera!
	camInit(scene->cam);
	cl_float3 tmp;
	vecInit(tmp, -7.8f, 10.0f, 7.8f);
	camMove(scene->cam, tmp);
	vecInit(tmp, 80.0f, 10.7f, -100.0f);
	camLookAt(scene->cam, tmp);
	scene->cam.imgWidth = imgWidth;
	scene->cam.imgHeight = imgHeight;

	// Action!
	animatePositions(scene, true);
}
