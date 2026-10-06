// Hit.cpp

#include <math.h>
#include "rt.h"
#include "R3.h"

namespace rt {

  float* Hit::getShadingNormal(void)
  {
    if (!triangle->surface->vnorm)   // no per-vertex normals
      return triangle->normal;
    else {
      const R3* n = triangle->getNormals();
      PINT(n[0], n[1], n[2], s, t, normal);
      R3NORMALIZE(normal);
      return &normal[0];
    }
  }

  float* Hit::getTexCoord(void)
  {
    if (!triangle->surface->vtco) {  // no per-vertex texture coords
      texcoord[0] = s; texcoord[1] = t; texcoord[2] = 1-s-t;
    } else {
      const R3* c = triangle->getTexCoords();
      PINT(c[0], c[1], c[2], s, t, texcoord);
    }
    return &texcoord[0];
  }

}
