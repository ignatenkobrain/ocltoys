# 1 "<stdin>"
# 1 "<command-line>"
# 1 "<stdin>"
# 22 "<stdin>"
# 1 "../common/camera.h" 1
# 25 "../common/camera.h"
# 1 "../common/vec.h" 1
# 25 "../common/vec.h"
typedef struct {
 float x, y, z;
} Vec;
# 26 "../common/camera.h" 2

typedef struct {

 Vec orig, target;

 Vec dir, x, y;
} Camera;
# 23 "<stdin>" 2
# 1 "geom.h" 1
# 25 "geom.h"
# 1 "../common/vec.h" 1
# 26 "geom.h" 2




typedef struct {
 Vec o, d;
} Ray;




typedef enum {
 MATTE, MIRROR, GLASS, MATTETRANSLUCENT, GLOSSY, GLOSSYTRANSLUCENT
} MaterialType;

typedef struct {
 float rad;
 Vec p;
 Vec e;
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
   float ior;
   float sigmaS, sigmaA;
  } glass;
  struct {
   Vec c;
   float transparency;
   float sigmaS, sigmaA;
  } mattertranslucent;
  struct {
   Vec c;
   float exponent;
  } glossy;
  struct {
   Vec c;
   float exponent;
   float transparency;
   float sigmaS, sigmaA;
  } glossytranslucent;
 };
} Sphere;
# 24 "<stdin>" 2






float GetRandom(unsigned int *seed0, unsigned int *seed1) {
 *seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
 *seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

 unsigned int ires = ((*seed0) << 16) + (*seed1);


 union {
  float f;
  unsigned int ui;
 } res;
 res.ui = (ires & 0x007fffff) | 0x40000000;

 return (res.f - 2.f) / 2.f;
}

float SphereIntersect(
 __global const Sphere *s,
 const Ray *r) {
 Vec op;
 { (op).x = (s->p).x - (r->o).x; (op).y = (s->p).y - (r->o).y; (op).z = (s->p).z - (r->o).z; };

 float b = ((op).x * (r->d).x + (op).y * (r->d).y + (op).z * (r->d).z);
 float det = b * b - ((op).x * (op).x + (op).y * (op).y + (op).z * (op).z) + s->rad * s->rad;
 if (det < 0.f)
  return 0.f;
 else
  det = sqrt(det);

 float t = b - det;
 if (t > 0.01f)
  return t;
 else {
  t = b + det;

  if (t > 0.01f)
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

 { (*v3).x = (*v1).y * (*v2).z - (*v1).z * (*v2).y; (*v3).y = (*v1).z * (*v2).x - (*v1).x * (*v2).z; (*v3).z = (*v1).x * (*v2).y - (*v1).y * (*v2).x; };
}

float SampleSegment(const float epsilon, const float sigma, const float smax) {
 return -log(1.f - epsilon * (1.f - exp(-sigma * smax))) / sigma;
}

void SampleHG(const float g, const float e1, const float e2, Vec *dir) {
 const float s = 1.f - 2.f * e1;
 const float cost = (s + 2.f * g * g * g * (-1.f + e1) * e1 + g * g * s + 2.f * g * (1.f - e1 + e1 * e1)) / ((1.f + g * s)*(1.f + g * s));
 const float sint = sqrt(1.f - cost * cost);

 dir->x = cos(2.f * 3.14159265358979323846f * e2) * sint;
 dir->y = sin(2.f * 3.14159265358979323846f * e2) * sint;
 dir->z = cost;
}

float Scatter(const Ray *currentRay, const float distance, Ray *scatterRay,
  float *scatterDistance, unsigned int *seed0, unsigned int *seed1, const float sigmaS) {
 *scatterDistance = SampleSegment(GetRandom(seed0, seed1), sigmaS, distance - 0.01f) + 0.01f;

 Vec scatterPoint;
 { float k = (*scatterDistance); { (scatterPoint).x = k * (currentRay->d).x; (scatterPoint).y = k * (currentRay->d).y; (scatterPoint).z = k * (currentRay->d).z; } };
 { (scatterPoint).x = (currentRay->o).x + (scatterPoint).x; (scatterPoint).y = (currentRay->o).y + (scatterPoint).y; (scatterPoint).z = (currentRay->o).z + (scatterPoint).z; };


 Vec dir;
 SampleHG(-.5f, GetRandom(seed0, seed1), GetRandom(seed0, seed1), &dir);

 Vec u, v;
 CoordinateSystem(&currentRay->d, &u, &v);

 Vec scatterDir;
 scatterDir.x = u.x * dir.x + v.x * dir.y + currentRay->d.x * dir.z;
 scatterDir.y = u.y * dir.x + v.y * dir.y + currentRay->d.y * dir.z;
 scatterDir.z = u.z * dir.x + v.z * dir.y + currentRay->d.z * dir.z;

 { { ((*scatterRay).o).x = (scatterPoint).x; ((*scatterRay).o).y = (scatterPoint).y; ((*scatterRay).o).z = (scatterPoint).z; }; { ((*scatterRay).d).x = (scatterDir).x; ((*scatterRay).d).y = (scatterDir).y; ((*scatterRay).d).z = (scatterDir).z; }; };

 return (1.f - exp(-sigmaS * (distance - 0.01f)));
}

void SpecularReflection(const Vec *wi, Vec *wo, const Vec *normal) {
 { float k = (2.f * ((*normal).x * (*wi).x + (*normal).y * (*wi).y + (*normal).z * (*wi).z)); { (*wo).x = k * (*normal).x; (*wo).y = k * (*normal).y; (*wo).z = k * (*normal).z; } };
 { (*wo).x = (*wi).x - (*wo).x; (*wo).y = (*wi).y - (*wo).y; (*wo).z = (*wi).z - (*wo).z; };
}

void GlossyReflection(const Vec *wi, Vec *wo, const Vec *normal, const float exponent,
  const float u0, const float u1) {
 const float phi = 2.f * 3.14159265358979323846f * u0;
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
 const float phi = 2.f * 3.14159265358979323846f * u0;
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

 Ray currentRay; { { ((currentRay).o).x = ((*startRay).o).x; ((currentRay).o).y = ((*startRay).o).y; ((currentRay).o).z = ((*startRay).o).z; }; { ((currentRay).d).x = ((*startRay).d).x; ((currentRay).d).y = ((*startRay).d).y; ((currentRay).d).z = ((*startRay).d).z; }; };
 Vec rad; { (rad).x = 0.f; (rad).y = 0.f; (rad).z = 0.f; };

 Vec throughput; { (throughput).x = 1.f; (throughput).y = 1.f; (throughput).z = 1.f; };

 unsigned int depth = 0;
 for (;; ++depth) {

  if (depth > PARAM_MAX_DEPTH) {
   *result = rad;
   return;
  }

  float t;
  unsigned int id = 0;
  const bool hit = Intersect(spheres, sphereCount, &currentRay, &t, &id);

  if (currentSigmaS > 0.f) {

   Ray scatterRay;
   float scatterDistance;
   const float scatteringProbability = Scatter(&currentRay, hit ? t : 999.f, &scatterRay,
     &scatterDistance, seed0, seed1, currentSigmaS);


   if ((scatteringProbability > 0.f) && (GetRandom(seed0, seed1) < scatteringProbability)) {

    { { ((currentRay).o).x = ((scatterRay).o).x; ((currentRay).o).y = ((scatterRay).o).y; ((currentRay).o).z = ((scatterRay).o).z; }; { ((currentRay).d).x = ((scatterRay).d).x; ((currentRay).d).y = ((scatterRay).d).y; ((currentRay).d).z = ((scatterRay).d).z; }; };


    const float absorption = exp(-currentSigmaT * scatterDistance);
    { float k = (absorption); { (throughput).x = k * (throughput).x; (throughput).y = k * (throughput).y; (throughput).z = k * (throughput).z; } };
    continue;
   }
  }

  if (!hit) {
   *result = rad;
   return;
  }


  const float absorption = exp(-currentSigmaT * t);
  { float k = (absorption); { (throughput).x = k * (throughput).x; (throughput).y = k * (throughput).y; (throughput).z = k * (throughput).z; } };

  __global const Sphere *obj = &spheres[id];

  Vec hitPoint;
  { float k = (t); { (hitPoint).x = k * (currentRay.d).x; (hitPoint).y = k * (currentRay.d).y; (hitPoint).z = k * (currentRay.d).z; } };
  { (hitPoint).x = (currentRay.o).x + (hitPoint).x; (hitPoint).y = (currentRay.o).y + (hitPoint).y; (hitPoint).z = (currentRay.o).z + (hitPoint).z; };

  Vec normal;
  { (normal).x = (hitPoint).x - (obj->p).x; (normal).y = (hitPoint).y - (obj->p).y; (normal).z = (hitPoint).z - (obj->p).z; };
  { float l = 1.f / sqrt(((normal).x * (normal).x + (normal).y * (normal).y + (normal).z * (normal).z)); { float k = (l); { (normal).x = k * (normal).x; (normal).y = k * (normal).y; (normal).z = k * (normal).z; } }; };


  const bool into = (((normal).x * (currentRay.d).x + (normal).y * (currentRay.d).y + (normal).z * (currentRay.d).z) < 0.f);
  Vec shadeNormal;
  { float k = (into ? 1.f : -1.f); { (shadeNormal).x = k * (normal).x; (shadeNormal).y = k * (normal).y; (shadeNormal).z = k * (normal).z; } };


  Vec eCol; { (eCol).x = (obj->e).x; (eCol).y = (obj->e).y; (eCol).z = (obj->e).z; };
  if (!(((eCol).x == 0.f) && ((eCol).x == 0.f) && ((eCol).z == 0.f))) {
   { (eCol).x = (throughput).x * (eCol).x; (eCol).y = (throughput).y * (eCol).y; (eCol).z = (throughput).z * (eCol).z; };
   { (rad).x = (rad).x + (eCol).x; (rad).y = (rad).y + (eCol).y; (rad).z = (rad).z + (eCol).z; };

   *result = rad;
   return;
  }

  switch (obj->matType) {
   case MATTE: {
    { (throughput).x = (throughput).x * (obj->matte.c).x; (throughput).y = (throughput).y * (obj->matte.c).y; (throughput).z = (throughput).z * (obj->matte.c).z; };

    const float r1 = 2.f * 3.14159265358979323846f * GetRandom(seed0, seed1);
    const float r2 = GetRandom(seed0, seed1);
    const float r2s = sqrt(r2);

    Vec w = shadeNormal;
    Vec u, v;
    CoordinateSystem(&w, &u, &v);

    Vec newDir;
    { float k = (cos(r1) * r2s); { (u).x = k * (u).x; (u).y = k * (u).y; (u).z = k * (u).z; } };
    { float k = (sin(r1) * r2s); { (v).x = k * (v).x; (v).y = k * (v).y; (v).z = k * (v).z; } };
    { (newDir).x = (u).x + (v).x; (newDir).y = (u).y + (v).y; (newDir).z = (u).z + (v).z; };
    { float k = (sqrt(1 - r2)); { (w).x = k * (w).x; (w).y = k * (w).y; (w).z = k * (w).z; } };
    { (newDir).x = (newDir).x + (w).x; (newDir).y = (newDir).y + (w).y; (newDir).z = (newDir).z + (w).z; };

    { { ((currentRay).o).x = (hitPoint).x; ((currentRay).o).y = (hitPoint).y; ((currentRay).o).z = (hitPoint).z; }; { ((currentRay).d).x = (newDir).x; ((currentRay).d).y = (newDir).y; ((currentRay).d).z = (newDir).z; }; };
    break;
   }
   case MIRROR: {
    { (throughput).x = (throughput).x * (obj->mirror.c).x; (throughput).y = (throughput).y * (obj->mirror.c).y; (throughput).z = (throughput).z * (obj->mirror.c).z; };

    Vec newDir;
    SpecularReflection(&currentRay.d, &newDir, &shadeNormal);

    { { ((currentRay).o).x = (hitPoint).x; ((currentRay).o).y = (hitPoint).y; ((currentRay).o).z = (hitPoint).z; }; { ((currentRay).d).x = (newDir).x; ((currentRay).d).y = (newDir).y; ((currentRay).d).z = (newDir).z; }; };
    break;
   }
   case GLASS: {
    Vec newDir;
    { float k = (2.f * ((normal).x * (currentRay.d).x + (normal).y * (currentRay.d).y + (normal).z * (currentRay.d).z)); { (newDir).x = k * (normal).x; (newDir).y = k * (normal).y; (newDir).z = k * (normal).z; } };
    { (newDir).x = (currentRay.d).x - (newDir).x; (newDir).y = (currentRay.d).y - (newDir).y; (newDir).z = (currentRay.d).z - (newDir).z; };

    Ray reflRay; { { ((reflRay).o).x = (hitPoint).x; ((reflRay).o).y = (hitPoint).y; ((reflRay).o).z = (hitPoint).z; }; { ((reflRay).d).x = (newDir).x; ((reflRay).d).y = (newDir).y; ((reflRay).d).z = (newDir).z; }; };

    const float nc = 1.f;
    const float nt = obj->glass.ior;
    const float nnt = into ? nc / nt : nt / nc;
    const float ddn = ((currentRay.d).x * (shadeNormal).x + (currentRay.d).y * (shadeNormal).y + (currentRay.d).z * (shadeNormal).z);
    const float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

    if (cos2t < 0.f) {
     { (throughput).x = (throughput).x * (obj->glass.c).x; (throughput).y = (throughput).y * (obj->glass.c).y; (throughput).z = (throughput).z * (obj->glass.c).z; };

     { { ((currentRay).o).x = ((reflRay).o).x; ((currentRay).o).y = ((reflRay).o).y; ((currentRay).o).z = ((reflRay).o).z; }; { ((currentRay).d).x = ((reflRay).d).x; ((currentRay).d).y = ((reflRay).d).y; ((currentRay).d).z = ((reflRay).d).z; }; };
     continue;
    }

    const float kk = (into ? 1 : -1) * (ddn * nnt + sqrt(cos2t));
    Vec nkk;
    { float k = (kk); { (nkk).x = k * (normal).x; (nkk).y = k * (normal).y; (nkk).z = k * (normal).z; } };
    Vec transDir;
    { float k = (nnt); { (transDir).x = k * (currentRay.d).x; (transDir).y = k * (currentRay.d).y; (transDir).z = k * (currentRay.d).z; } };
    { (transDir).x = (transDir).x - (nkk).x; (transDir).y = (transDir).y - (nkk).y; (transDir).z = (transDir).z - (nkk).z; };
    { float l = 1.f / sqrt(((transDir).x * (transDir).x + (transDir).y * (transDir).y + (transDir).z * (transDir).z)); { float k = (l); { (transDir).x = k * (transDir).x; (transDir).y = k * (transDir).y; (transDir).z = k * (transDir).z; } }; };

    const float a = nt - nc;
    const float b = nt + nc;
    const float R0 = a * a / (b * b);
    const float c = 1 - (into ? -ddn : ((transDir).x * (normal).x + (transDir).y * (normal).y + (transDir).z * (normal).z));

    const float Re = R0 + (1 - R0) * c * c * c * c*c;
    const float Tr = 1.f - Re;
    const float P = .25f + .5f * Re;
    const float RP = Re / P;
    const float TP = Tr / (1.f - P);

    if (GetRandom(seed0, seed1) < P) {
     { float k = (RP); { (throughput).x = k * (throughput).x; (throughput).y = k * (throughput).y; (throughput).z = k * (throughput).z; } };
     { (throughput).x = (throughput).x * (obj->glass.c).x; (throughput).y = (throughput).y * (obj->glass.c).y; (throughput).z = (throughput).z * (obj->glass.c).z; };

     { { ((currentRay).o).x = ((reflRay).o).x; ((currentRay).o).y = ((reflRay).o).y; ((currentRay).o).z = ((reflRay).o).z; }; { ((currentRay).d).x = ((reflRay).d).x; ((currentRay).d).y = ((reflRay).d).y; ((currentRay).d).z = ((reflRay).d).z; }; };
    } else {
     { float k = (TP); { (throughput).x = k * (throughput).x; (throughput).y = k * (throughput).y; (throughput).z = k * (throughput).z; } };
     { (throughput).x = (throughput).x * (obj->glass.c).x; (throughput).y = (throughput).y * (obj->glass.c).y; (throughput).z = (throughput).z * (obj->glass.c).z; };

     { { ((currentRay).o).x = (hitPoint).x; ((currentRay).o).y = (hitPoint).y; ((currentRay).o).z = (hitPoint).z; }; { ((currentRay).d).x = (transDir).x; ((currentRay).d).y = (transDir).y; ((currentRay).d).z = (transDir).z; }; };

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
    { (throughput).x = (throughput).x * (obj->mattertranslucent.c).x; (throughput).y = (throughput).y * (obj->mattertranslucent.c).y; (throughput).z = (throughput).z * (obj->mattertranslucent.c).z; };


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

    const float r1 = 2.f * 3.14159265358979323846f * GetRandom(seed0, seed1);
    const float r2 = GetRandom(seed0, seed1);
    const float r2s = sqrt(r2);

    Vec u, v;
    CoordinateSystem(&shadeNormal, &u, &v);

    Vec newDir;
    { float k = (cos(r1) * r2s); { (u).x = k * (u).x; (u).y = k * (u).y; (u).z = k * (u).z; } };
    { float k = (sin(r1) * r2s); { (v).x = k * (v).x; (v).y = k * (v).y; (v).z = k * (v).z; } };
    { (newDir).x = (u).x + (v).x; (newDir).y = (u).y + (v).y; (newDir).z = (u).z + (v).z; };
    Vec w;
    { float k = ((transmit ? -1.f : 1.) * sqrt(1 - r2)); { (w).x = k * (shadeNormal).x; (w).y = k * (shadeNormal).y; (w).z = k * (shadeNormal).z; } };
    { (newDir).x = (newDir).x + (w).x; (newDir).y = (newDir).y + (w).y; (newDir).z = (newDir).z + (w).z; };

    { { ((currentRay).o).x = (hitPoint).x; ((currentRay).o).y = (hitPoint).y; ((currentRay).o).z = (hitPoint).z; }; { ((currentRay).d).x = (newDir).x; ((currentRay).d).y = (newDir).y; ((currentRay).d).z = (newDir).z; }; };
    break;
   }
   case GLOSSY: {
    { (throughput).x = (throughput).x * (obj->glossy.c).x; (throughput).y = (throughput).y * (obj->glossy.c).y; (throughput).z = (throughput).z * (obj->glossy.c).z; };

    Vec newDir;
    GlossyReflection(&currentRay.d, &newDir, &shadeNormal,
      obj->glossy.exponent,
      GetRandom(seed0, seed1), GetRandom(seed0, seed1));

    { { ((currentRay).o).x = (hitPoint).x; ((currentRay).o).y = (hitPoint).y; ((currentRay).o).z = (hitPoint).z; }; { ((currentRay).d).x = (newDir).x; ((currentRay).d).y = (newDir).y; ((currentRay).d).z = (newDir).z; }; };
    break;
   }
   case GLOSSYTRANSLUCENT: {

    Vec newDir;
    if (GetRandom(seed0, seed1) < obj->glossytranslucent.transparency) {
     { (throughput).x = (throughput).x * (obj->glossytranslucent.c).x; (throughput).y = (throughput).y * (obj->glossytranslucent.c).y; (throughput).z = (throughput).z * (obj->glossytranslucent.c).z; };

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

     GlossyReflection(&currentRay.d, &newDir, &shadeNormal,
       obj->glossytranslucent.exponent,
       GetRandom(seed0, seed1), GetRandom(seed0, seed1));
    }

    { { ((currentRay).o).x = (hitPoint).x; ((currentRay).o).y = (hitPoint).y; ((currentRay).o).z = (hitPoint).z; }; { ((currentRay).d).x = (newDir).x; ((currentRay).d).y = (newDir).y; ((currentRay).d).z = (newDir).z; }; };
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
 { (rdir).x = camera->x.x * kcx + camera->y.x * kcy + camera->dir.x; (rdir).y = camera->x.y * kcx + camera->y.y * kcy + camera->dir.y; (rdir).z = camera->x.z * kcx + camera->y.z * kcy + camera->dir.z; }


                                                         ;

 Vec rorig;
 { float k = (0.1f); { (rorig).x = k * (rdir).x; (rorig).y = k * (rdir).y; (rorig).z = k * (rdir).z; } };
 { (rorig).x = (rorig).x + (camera->orig).x; (rorig).y = (rorig).y + (camera->orig).y; (rorig).z = (rorig).z + (camera->orig).z; }

 { float l = 1.f / sqrt(((rdir).x * (rdir).x + (rdir).y * (rdir).y + (rdir).z * (rdir).z)); { float k = (l); { (rdir).x = k * (rdir).x; (rdir).y = k * (rdir).y; (rdir).z = k * (rdir).z; } }; };
 { { ((*ray).o).x = (rorig).x; ((*ray).o).y = (rorig).y; ((*ray).o).z = (rorig).z; }; { ((*ray).d).x = (rdir).x; ((*ray).d).y = (rdir).y; ((*ray).d).z = (rdir).z; }; };
}

__kernel void SmallPTGPU(
    __global Vec *samples, __global unsigned int *seedsInput,
 __global const Camera *camera,
 const unsigned int sphereCount, __global const Sphere *sphere,
 const unsigned int width, const unsigned int height,
 const unsigned int currentSample) {
 const int gid = get_global_id(0);

 if (gid >= width * height)
  return;

 const int scrX = gid % width;
 const int scrY = gid / width;


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
  sample->x = (sample->x * k1 + r.x) * k2;
  sample->y = (sample->y * k1 + r.y) * k2;
  sample->z = (sample->z * k1 + r.z) * k2;
 }

 seedsInput[2 * gid] = seed0;
 seedsInput[2 * gid + 1] = seed1;
}



__kernel void ToneMapping(
 __global Vec *samples, __global Vec *pixels,
 const unsigned int width, const unsigned int height) {
 const int gid = get_global_id(0);

 if (gid >= width * height)
  return;

 __global Vec *sample = &samples[gid];
 __global Vec *pixel = &pixels[gid];
 pixel->x = (pow(clamp(sample->x, 0.f, 1.f), 1.f / 2.2f));
 pixel->y = (pow(clamp(sample->y, 0.f, 1.f), 1.f / 2.2f));
 pixel->z = (pow(clamp(sample->z, 0.f, 1.f), 1.f / 2.2f));
}

__kernel void WebCLToneMapping(
 __global Vec *samples, __global int *pixels,
 const unsigned int width, const unsigned int height) {
 const int gid = get_global_id(0);

 if (gid >= width * height)
  return;

 __global Vec *sample = &samples[gid];

 const unsigned int x = gid % width;
 const unsigned int y = height - gid / width - 1;
 __global int *pixel = &pixels[x + y * width];
 const float r = (pow(clamp(sample->x, 0.f, 1.f), 1.f / 2.2f));
 const float g = (pow(clamp(sample->y, 0.f, 1.f), 1.f / 2.2f));
 const float b = (pow(clamp(sample->z, 0.f, 1.f), 1.f / 2.2f));
 const int ur = (int)(r * 255.f + .5f);
 const int ug = (int)(g * 255.f + .5f);
 const int ub = (int)(b * 255.f + .5f);
 *pixel = ur | (ug << 8) | (ub << 16) | (0xff << 24);
}
