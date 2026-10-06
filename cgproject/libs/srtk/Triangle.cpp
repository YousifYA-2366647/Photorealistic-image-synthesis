// Triangle.cpp

#include "rt.h"
#include "R3.h"
#include "Bounds.h"

namespace rt {

  bool Triangle::close(void)
  {
    // Newells method (see e.g. Tampieri, Graphics Gems III p.231) with one
    // modification to make it more robust: polygon is translated to the
    // origin.
    R3* vcoord = &surface->vcoord[vidx];
    double prev[3], cur[3], dnormal[3];
    R3SET(dnormal,0,0,0);
    R3SUBTRACT(vcoord[2],vcoord[0],cur);
    for(int i=0; i<3; i++) {
      R3COPY(cur, prev); // prev = cur;
      R3SUBTRACT(vcoord[i],vcoord[0],cur);
      dnormal[0] += (prev[1] - cur[1]) * (prev[2] + cur[2]);
      dnormal[1] += (prev[2] - cur[2]) * (prev[0] + cur[0]);
      dnormal[2] += (prev[0] - cur[0]) * (prev[1] + cur[1]);
    }

    if (is_reversed()) {
      dnormal[0] = -dnormal[0]; dnormal[1] = -dnormal[1]; dnormal[2] = -dnormal[2];
    }

    double norm = R3NORM(dnormal);
    R3SCALEINVERSE(norm, dnormal, normal);
    plane_constant = -R3DOTPRODUCT(normal, vcoord[0]);

    set_degenerate(!(norm > EPSILON));

    // calculate dominant normal component (for projection to 2D)
    float n[3];
    R3ABS(normal, n);
    set_orientation((n[0]>n[1] && n[0]>n[2]) ? 0 : ((n[1]>n[2]) ? 1 : 2));

    set_last_ray_id(0);
    set_is_triangle(true);

    return is_degenerate();
  }

  Hit* Triangle::intersect(Ray& ray, Hit* hit)
  {
    ray.nr_triangle_tests ++;

    if (is_degenerate() ||
      ray.exclude[0] == this || ray.exclude[1] == this)
      return 0;   // excluded from intersection testing

    double dist = R3DOTPRODUCT(normal, ray.dir);
    if (dist > EPSILON) { // backfacing triangle
      if (!(ray.flags & RT_BACK))
        return 0;
    } else if (dist < -EPSILON) { // frontfacing triangle
      if (!(ray.flags & RT_FRONT))
        return 0;
    } else {  // ray is parallel with the triangle plane
      return 0;
    }

    dist = - (R3DOTPRODUCT(normal, ray.org) + plane_constant) / dist;
    if (dist > ray.maxdist || dist < ray.mindist)
      // intersection too far or too near
      return 0;

    // intersection point with plane of triangle
    double point[3];
    R3ADDSCALED(ray.org, dist, ray.dir, point);

    // point-in-triangle test with Badouels method 
    // (Graphics Gems I, p390).
    double u0, v0;
    double p0[2], p1[2], p2[2];

    // project to 2D according to dominant normal component
    R3* vcoord = getCoords();
#define R2SET(V, a, b) { (V)[0] = (a) ; (V)[1] = (b) ; }
    switch (orientation()) {
    case 0:           // project to YZ plane
      u0 = vcoord[0][1]; v0 = vcoord[0][2];
      R2SET(p0,     point[1] - u0,     point[2] - v0);
      R2SET(p1, vcoord[1][1] - u0, vcoord[1][2] - v0);
      R2SET(p2, vcoord[2][1] - u0, vcoord[2][2] - v0);
      break;

    case 1:           // project to XZ plane
      u0 = vcoord[0][0]; v0 = vcoord[0][2];
      R2SET(p0,     point[0] - u0,     point[2] - v0);
      R2SET(p1, vcoord[1][0] - u0, vcoord[1][2] - v0);
      R2SET(p2, vcoord[2][0] - u0, vcoord[2][2] - v0);
      break;

    case 2:           // project to XY plane
      u0 = vcoord[0][0]; v0 = vcoord[0][1];
      R2SET(p0,     point[0] - u0,     point[1] - v0);
      R2SET(p1, vcoord[1][0] - u0, vcoord[1][1] - v0);
      R2SET(p2, vcoord[2][0] - u0, vcoord[2][1] - v0);
      break;

    default:          // can't happen
      return 0;
    }

    // find barycentric coordinates of intersection point and
    // check whether they are in the valid range
    double alpha, beta;
    if (p1[0] < -EPSILON || p1[0] > EPSILON) {  /* p1[0] non zero */
      beta = (p0[1]*p1[0] - p0[0]*p1[1]) / (p2[1]*p1[0] - p2[0]*p1[1]);
      if (beta >= 0. && beta <= 1.)
        alpha = (p0[0] - beta * p2[0]) / p1[0];
      else
        return 0;
    } else {
      beta = p0[0] / p2[0];
      if (beta >= 0. && beta <= 1.)
        alpha = (p0[1] - beta * p2[1]) / p1[1];
      else
        return 0;
    }
    if (alpha < 0. || (alpha + beta) > 1.)
      return 0;

    // intersection point is inside triangle.
    // fill in hit record data
    hit->dist = dist;
    hit->s = alpha;
    hit->t = beta;
    hit->triangle = this;
    R3COPY(point, hit->point);

    return hit;
  }

} // nmaespace
