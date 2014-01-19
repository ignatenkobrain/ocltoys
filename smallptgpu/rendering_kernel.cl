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

#include "camera.h"
#include "geom.h"

// Kernel compilation parameters:
//  PARAM_MAX_DEPTH
//  PARAM_DEFAULT_SIGMA_S
//  PARAM_DEFAULT_SIGMA_A

float GetRandom(unsigned int *seed0, unsigned int *seed1) {
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	/* Convert to float */
	union {
		float f;
		unsigned int ui;
	} res;
	res.ui = (ires & 0x007fffff) | 0x40000000;

	return (res.f - 2.f) / 2.f;
}

float SphereIntersect(
	__global const Sphere *s,
	const Ray *r) { /* returns distance, 0 if nohit */
	Vec op; /* Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0 */
	vsub(op, s->p, r->o);

	float b = vdot(op, r->d);
	float det = b * b - vdot(op, op) + s->rad * s->rad;
	if (det < 0.f)
		return 0.f;
	else
		det = sqrt(det);

	float t = b - det;
	if (t >  EPSILON)
		return t;
	else {
		t = b + det;

		if (t >  EPSILON)
			return t;
		else
			return 0.f;
	}
}

int Intersect(
	__global const Sphere *spheres,
	const unsigned int sphereCount,
	const Ray *r,
	float *t,
	unsigned int *id) {
	float inf = (*t) = 1e20f;

	unsigned int i = sphereCount;
	for (; i--;) {
		const float d = SphereIntersect(&spheres[i], r);
		if ((d != 0.f) && (d < *t)) {
			*t = d;
			*id = i;
		}
	}

	return (*t < inf);
}

void CoordinateSystem(const Vec *v1, Vec *v2, Vec *v3) {
	if (fabs(v1->x) > fabs(v1->y)) {
		float invLen = 1.f / sqrt(v1->x * v1->x + v1->z * v1->z);
		v2->x = -v1->z * invLen;
		v2->y = 0.f;
		v2->z = v1->x * invLen;
	} else {
		float invLen = 1.f / sqrt(v1->y * v1->y + v1->z * v1->z);
		v2->x = 0.f;
		v2->y = v1->z * invLen;
		v2->z = -v1->y * invLen;
	}

	vxcross(*v3, *v1, *v2);
}

float SampleSegment(const float epsilon, const float sigma, const float smax) {
	return -log(1.f - epsilon * (1.f - exp(-sigma * smax))) / sigma;
}

void SampleHG(const float g, const float e1, const float e2, Vec *dir) {
	const float s = 1.f - 2.f * e1;
	const float cost = (s + 2.f * g * g * g * (-1.f + e1) * e1 + g * g * s + 2.f * g * (1.f - e1 + e1 * e1)) / ((1.f + g * s)*(1.f + g * s));
	const float sint = sqrt(1.f - cost * cost);

	dir->x = cos(2.f * FLOAT_PI * e2) * sint;
	dir->y = sin(2.f * FLOAT_PI * e2) * sint;
	dir->z = cost;
}

float Scatter(const Ray *currentRay, const float distance, Ray *scatterRay,
		float *scatterDistance, unsigned int *seed0, unsigned int *seed1, const float sigmaS) {
	*scatterDistance = SampleSegment(GetRandom(seed0, seed1), sigmaS, distance - EPSILON) + EPSILON;

	Vec scatterPoint;
	vsmul(scatterPoint, *scatterDistance, currentRay->d);
	vadd(scatterPoint, currentRay->o, scatterPoint);

	// Sample a direction ~ Henyey-Greenstein's phase function
	Vec dir;
	SampleHG(-.5f, GetRandom(seed0, seed1), GetRandom(seed0, seed1), &dir);

	Vec u, v;
	CoordinateSystem(&currentRay->d, &u, &v);

	Vec scatterDir;
	scatterDir.x = u.x * dir.x + v.x * dir.y + currentRay->d.x * dir.z;
	scatterDir.y = u.y * dir.x + v.y * dir.y + currentRay->d.y * dir.z;
	scatterDir.z = u.z * dir.x + v.z * dir.y + currentRay->d.z * dir.z;

	rinit(*scatterRay, scatterPoint, scatterDir);

	return (1.f - exp(-sigmaS * (distance - EPSILON)));
}

void SpecularReflection(const Vec *wi, Vec *wo, const Vec *normal) {
	vsmul(*wo,  2.f * vdot(*normal, *wi), *normal);
	vsub(*wo, *wi, *wo);
}

void GlossyReflection(const Vec *wi, Vec *wo, const Vec *normal, const float exponent,
		const float u0, const float u1) {
	const float phi = 2.f * FLOAT_PI * u0;
	const float sinTheta = pow(1.f - u1, exponent);
	const float cosTheta = sqrt(1.f - sinTheta * sinTheta);
	const float x = cos(phi) * sinTheta;
	const float y = sin(phi) * sinTheta;
	const float z = cosTheta;

	Vec specDir;
	SpecularReflection(wi, &specDir, normal);

	Vec u, v;
	CoordinateSystem(&specDir, &u, &v);

	wo->x = x * u.x + y * v.x + z * specDir.x;
	wo->y = x * u.y + y * v.y + z * specDir.y;
	wo->z = x * u.z + y * v.z + z * specDir.z;
}

void GlossyTransmission(const Vec *wi, Vec *wo, const Vec *normal, const float exponent,
		const float u0, const float u1) {
	const float phi = 2.f * FLOAT_PI * u0;
	const float sinTheta = pow(1.f - u1, exponent);
	const float cosTheta = sqrt(1.f - sinTheta * sinTheta);
	const float x = cos(phi) * sinTheta;
	const float y = sin(phi) * sinTheta;
	const float z = cosTheta;

	Vec specDir = *wi;
	Vec u, v;
	CoordinateSystem(&specDir, &u, &v);

	wo->x = x * u.x + y * v.x + z * specDir.x;
	wo->y = x * u.y + y * v.y + z * specDir.y;
	wo->z = x * u.z + y * v.z + z * specDir.z;
}

void Radiance(
	__global const Sphere *spheres,
	const unsigned int sphereCount,
	const Ray *startRay,
	unsigned int *seed0, unsigned int *seed1,
	Vec *result) {
	float currentSigmaS = PARAM_DEFAULT_SIGMA_S;
	float currentSigmaA = PARAM_DEFAULT_SIGMA_A;
	float currentSigmaT = currentSigmaS + currentSigmaA;

	Ray currentRay; rassign(currentRay, *startRay);
	Vec rad; vinit(rad, 0.f, 0.f, 0.f);

	Vec throughput; vinit(throughput, 1.f, 1.f, 1.f);

	unsigned int depth = 0;
	for (;; ++depth) {
		// Removed Russian Roulette in order to improve execution on SIMT
		if (depth > PARAM_MAX_DEPTH) {
			*result = rad;
			return;
		}

		float t; /* distance to intersection */
		unsigned int id = 0; /* id of intersected object */
		const bool hit = Intersect(spheres, sphereCount, &currentRay, &t, &id);

		if (currentSigmaS > 0.f) {
			// Check if there is a scattering event
			Ray scatterRay;
			float scatterDistance;
			const float scatteringProbability = Scatter(&currentRay, hit ? t : 999.f, &scatterRay,
					&scatterDistance, seed0, seed1, currentSigmaS);

			// Is there the scatter event ?
			if ((scatteringProbability > 0.f) && (GetRandom(seed0, seed1) < scatteringProbability)) {
				// There is, sample the volume
				rassign(currentRay, scatterRay);

				// Absorption
				const float absorption = exp(-currentSigmaT * scatterDistance);
				vsmul(throughput, absorption, throughput);
				continue;
			}
		}
			
		if (!hit) {
			*result = rad; /* if miss, return */
			return;
		}

		// Absorption
		const float absorption = exp(-currentSigmaT * t);
		vsmul(throughput, absorption, throughput);

		__global const Sphere *obj = &spheres[id]; /* the hit object */

		Vec hitPoint;
		vsmul(hitPoint, t, currentRay.d);
		vadd(hitPoint, currentRay.o, hitPoint);

		Vec normal;
		vsub(normal, hitPoint, obj->p);
		vnorm(normal);

		// Ray from outside going in ?
		const bool into = (vdot(normal, currentRay.d) < 0.f);
		Vec shadeNormal;
		vsmul(shadeNormal, into ? 1.f : -1.f, normal);

		/* Add emitted light */
		Vec eCol; vassign(eCol, obj->e);
		if (!viszero(eCol)) {
			vmul(eCol, throughput, eCol);
			vadd(rad, rad, eCol);

			*result = rad;
			return;
		}

		switch (obj->matType) {
			case MATTE: {
				vmul(throughput, throughput, obj->matte.c);

				const float r1 = 2.f * FLOAT_PI * GetRandom(seed0, seed1);
				const float r2 = GetRandom(seed0, seed1);
				const float r2s = sqrt(r2);

				Vec w = shadeNormal;
				Vec u, v;
				CoordinateSystem(&w, &u, &v);

				Vec newDir;
				vsmul(u, cos(r1) * r2s, u);
				vsmul(v, sin(r1) * r2s, v);
				vadd(newDir, u, v);
				vsmul(w, sqrt(1 - r2), w);
				vadd(newDir, newDir, w);

				rinit(currentRay, hitPoint, newDir);
				break;
			}
			case MIRROR: {
				vmul(throughput, throughput, obj->mirror.c);

				Vec newDir;
				SpecularReflection(&currentRay.d, &newDir, &shadeNormal);

				rinit(currentRay, hitPoint, newDir);
				break;
			}
			case GLASS: {
				Vec newDir;
				vsmul(newDir,  2.f * vdot(normal, currentRay.d), normal);
				vsub(newDir, currentRay.d, newDir);

				Ray reflRay; rinit(reflRay, hitPoint, newDir); /* Ideal dielectric REFRACTION */

				const float nc = 1.f;
				const float nt = obj->glass.ior;
				const float nnt = into ? nc / nt : nt / nc;
				const float ddn = vdot(currentRay.d, shadeNormal);
				const float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

				if (cos2t < 0.f)  { /* Total internal reflection */
					vmul(throughput, throughput, obj->glass.c);

					rassign(currentRay, reflRay);
					continue;
				}

				const float kk = (into ? 1 : -1) * (ddn * nnt + sqrt(cos2t));
				Vec nkk;
				vsmul(nkk, kk, normal);
				Vec transDir;
				vsmul(transDir, nnt, currentRay.d);
				vsub(transDir, transDir, nkk);
				vnorm(transDir);

				const float a = nt - nc;
				const float b = nt + nc;
				const float R0 = a * a / (b * b);
				const float c = 1 - (into ? -ddn : vdot(transDir, normal));

				const float Re = R0 + (1 - R0) * c * c * c * c*c;
				const float Tr = 1.f - Re;
				const float P = .25f + .5f * Re;
				const float RP = Re / P;
				const float TP = Tr / (1.f - P);

				if (GetRandom(seed0, seed1) < P) { /* R.R. */
					vsmul(throughput, RP, throughput);
					vmul(throughput, throughput, obj->glass.c);

					rassign(currentRay, reflRay);
				} else {
					vsmul(throughput, TP, throughput);
					vmul(throughput, throughput, obj->glass.c);

					rinit(currentRay, hitPoint, transDir);

					if (into) {
						currentSigmaS = obj->glass.sigmaS;
						currentSigmaA = obj->glass.sigmaA;
					} else {
						currentSigmaS = PARAM_DEFAULT_SIGMA_S;
						currentSigmaA = PARAM_DEFAULT_SIGMA_A;					
					}
					currentSigmaT = currentSigmaS + currentSigmaA;
				}
				break;
			}
			case MATTETRANSLUCENT: {
				vmul(throughput, throughput, obj->mattertranslucent.c);

				// Transmitted or reflect ?
				bool transmit;
				if (GetRandom(seed0, seed1) < obj->mattertranslucent.transparency) {
					if (into) {
						currentSigmaS = obj->mattertranslucent.sigmaS;
						currentSigmaA = obj->mattertranslucent.sigmaA;
					} else {
						currentSigmaS = PARAM_DEFAULT_SIGMA_S;
						currentSigmaA = PARAM_DEFAULT_SIGMA_A;					
					}
					currentSigmaT = currentSigmaS + currentSigmaA;

					transmit = true;
				} else
					transmit = false;

				const float r1 = 2.f * FLOAT_PI * GetRandom(seed0, seed1);
				const float r2 = GetRandom(seed0, seed1);
				const float r2s = sqrt(r2);

				Vec u, v;
				CoordinateSystem(&shadeNormal, &u, &v);

				Vec newDir;
				vsmul(u, cos(r1) * r2s, u);
				vsmul(v, sin(r1) * r2s, v);
				vadd(newDir, u, v);
				Vec w;
				vsmul(w, (transmit ? -1.f : 1.) * sqrt(1 - r2), shadeNormal);
				vadd(newDir, newDir, w);

				rinit(currentRay, hitPoint, newDir);
				break;
			}
			case GLOSSY: {
				vmul(throughput, throughput, obj->glossy.c);

				Vec newDir;
				GlossyReflection(&currentRay.d, &newDir, &shadeNormal,
						obj->glossy.exponent,
						GetRandom(seed0, seed1), GetRandom(seed0, seed1));

				rinit(currentRay, hitPoint, newDir);
				break;
			}
			case GLOSSYTRANSLUCENT: {
				// Transmitted or reflect ?
				Vec newDir;
				if (GetRandom(seed0, seed1) < obj->glossytranslucent.transparency) {
					vmul(throughput, throughput, obj->glossytranslucent.c);

					if (into) {
						currentSigmaS = obj->glossytranslucent.sigmaS;
						currentSigmaA = obj->glossytranslucent.sigmaA;
					} else {
						currentSigmaS = PARAM_DEFAULT_SIGMA_S;
						currentSigmaA = PARAM_DEFAULT_SIGMA_A;					
					}
					currentSigmaT = currentSigmaS + currentSigmaA;

					GlossyTransmission(&currentRay.d, &newDir, &shadeNormal,
							obj->glossytranslucent.exponent,
							GetRandom(seed0, seed1), GetRandom(seed0, seed1));
				} else {
					// Using white reflections
					GlossyReflection(&currentRay.d, &newDir, &shadeNormal,
							obj->glossytranslucent.exponent,
							GetRandom(seed0, seed1), GetRandom(seed0, seed1));
				}

				rinit(currentRay, hitPoint, newDir);
				break;
			}
			default:
				*result = rad;
				return;
		}
	}
}

void GenerateCameraRay(__global const Camera *camera,
		unsigned int *seed0, unsigned int *seed1,
		const int width, const int height, const int x, const int y, Ray *ray) {
	const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float r1 = GetRandom(seed0, seed1) - .5f;
	const float r2 = GetRandom(seed0, seed1) - .5f;
	const float kcx = (x + r1) * invWidth - .5f;
	const float kcy = (y + r2) * invHeight - .5f;

	Vec rdir;
	vinit(rdir,
			camera->x.x * kcx + camera->y.x * kcy + camera->dir.x,
			camera->x.y * kcx + camera->y.y * kcy + camera->dir.y,
			camera->x.z * kcx + camera->y.z * kcy + camera->dir.z);

	Vec rorig;
	vsmul(rorig, 0.1f, rdir);
	vadd(rorig, rorig, camera->orig)

	vnorm(rdir);
	rinit(*ray, rorig, rdir);
}

__kernel void SmallPTGPU(
    __global Vec *samples, __global unsigned int *seedsInput,
	__global const Camera *camera,
	const unsigned int sphereCount, __global const Sphere *sphere,
	const unsigned int width, const unsigned int height,
	const unsigned int currentSample) {
	const int gid = get_global_id(0);
	// Check if we have to do something
	if (gid >= width * height)
		return;

	const int scrX = gid % width;
	const int scrY = gid / width;

	/* LordCRC: move seed to local store */
	unsigned int seed0 = seedsInput[2 * gid];
	unsigned int seed1 = seedsInput[2 * gid + 1];

	Ray ray;
	GenerateCameraRay(camera, &seed0, &seed1, width, height, scrX, scrY, &ray);

	Vec r;
	Radiance(sphere, sphereCount, &ray, &seed0, &seed1, &r);

	__global Vec *sample = &samples[gid];
	if (currentSample == 0)
		*sample = r;
	else {
		const float k1 = currentSample;
		const float k2 = 1.f / (currentSample + 1.f);
		sample->x = (sample->x * k1  + r.x) * k2;
		sample->y = (sample->y * k1  + r.y) * k2;
		sample->z = (sample->z * k1  + r.z) * k2;
	}

	seedsInput[2 * gid] = seed0;
	seedsInput[2 * gid + 1] = seed1;
}

#define toColor(x) (pow(clamp(x, 0.f, 1.f), 1.f / 2.2f))

__kernel void ToneMapping(
	__global Vec *samples, __global Vec *pixels,
	const unsigned int width, const unsigned int height) {
	const int gid = get_global_id(0);
	// Check if we have to do something
	if (gid >= width * height)
		return;

	__global Vec *sample = &samples[gid];
	__global Vec *pixel = &pixels[gid];
	pixel->x = toColor(sample->x);
	pixel->y = toColor(sample->y);
	pixel->z = toColor(sample->z);
}

__kernel void WebCLToneMapping(
	__global Vec *samples, __global int *pixels,
	const unsigned int width, const unsigned int height) {
	const int gid = get_global_id(0);
	// Check if we have to do something
	if (gid >= width * height)
		return;

	__global Vec *sample = &samples[gid];

	const unsigned int x = gid % width;
	const unsigned int y = height - gid / width - 1;
	__global int *pixel = &pixels[x + y * width];
	const float r = toColor(sample->x);
	const float g = toColor(sample->y);
	const float b = toColor(sample->z);
	const int ur = (int)(r * 255.f + .5f);
	const int ug = (int)(g * 255.f + .5f);
	const int ub = (int)(b * 255.f + .5f);
	*pixel = ur | (ug << 8) | (ub << 16) | (0xff << 24);
}
