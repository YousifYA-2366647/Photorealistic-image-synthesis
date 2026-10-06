/* bounds.c */
#include "Bounds.h"

namespace rt {

#define SetIfLess(a, b)		(a = ((a) < (b) ? (a) : (b)))
#define SetIfGreater(a, b)	(a = ((a) > (b) ? (a) : (b)))

  /* enlarge bounds with extra */
  void Bounds::enlarge(const Bounds& extra)
  {
    SetIfLess(b[MIN_X], extra[MIN_X]);
    SetIfLess(b[MIN_Y], extra[MIN_Y]);
    SetIfLess(b[MIN_Z], extra[MIN_Z]);
    SetIfGreater(b[MAX_X], extra[MAX_X]);
    SetIfGreater(b[MAX_Y], extra[MAX_Y]);
    SetIfGreater(b[MAX_Z], extra[MAX_Z]);
  }

  void Bounds::enlarge(const R3& point)
  {
    SetIfLess(b[MIN_X], point[X]);
    SetIfLess(b[MIN_Y], point[Y]);
    SetIfLess(b[MIN_Z], point[Z]);
    SetIfGreater(b[MAX_X], point[X]);
    SetIfGreater(b[MAX_Y], point[Y]);
    SetIfGreater(b[MAX_Z], point[Z]);
  }

  void Bounds::enlarge(const Triangle* const tri)
  {
    R3* v = tri->getCoords();
    enlarge(v[0]);
    enlarge(v[1]);
    enlarge(v[2]);
  }

  void Bounds::init(const Triangle* const tri)
  {
    R3* v = tri->getCoords();
    initpoint(v[0]);
    enlarge(v[1]);
    enlarge(v[2]);
  }

  bool Bounds::intersect(Ray& ray, float& tmin, float& tmax)
  {
    ray.nr_bounding_box_tests ++;

    float t;
    float dir, pos;

    tmin = ray.mindist;
    tmax = ray.maxdist;

    dir = ray.dir[X];
    pos = ray.org[X];

    if (dir < 0) {
      t = (b[MIN_X] - pos) / dir;
      if (t < tmin)
        return false;
      if (t <= tmax)
        tmax = t;
      t = (b[MAX_X] - pos) / dir;
      if (t >= tmin) {
        if (t > tmax * (1.+EPSILON))
          return false;
        tmin = t;
      }
    } else if (dir > 0.) {
      t = (b[MAX_X] - pos) / dir;
      if (t < tmin)
        return false;
      if (t <= tmax)
        tmax = t;
      t = (b[MIN_X] - pos) / dir;
      if (t >= tmin) {
        if (t > tmax * (1.+EPSILON))
          return false;
        tmin = t;
      }
    } else if (pos < b[MIN_X] || pos > b[MAX_X])
      return false;

    dir = ray.dir[Y];
    pos = ray.org[Y];

    if (dir < 0) {
      t = (b[MIN_Y] - pos) / dir;
      if (t < tmin)
        return false;
      if (t <= tmax)
        tmax = t;
      t = (b[MAX_Y] - pos) / dir;
      if (t >= tmin) {
        if (t > tmax * (1.+EPSILON))
          return false;
        tmin = t;
      }
    } else if (dir > 0.) {
      t = (b[MAX_Y] - pos) / dir;
      if (t < tmin)
        return false;
      if (t <= tmax)
        tmax = t;
      t = (b[MIN_Y] - pos) / dir;
      if (t >= tmin) {
        if (t > tmax * (1.+EPSILON))
          return false;
        tmin = t;
      }
    } else if (pos < b[MIN_Y] || pos > b[MAX_Y])
      return false;

    dir = ray.dir[Z];
    pos = ray.org[Z];

    if (dir < 0) {
      t = (b[MIN_Z] - pos) / dir;
      if (t < tmin)
        return false;
      if (t <= tmax)
        tmax = t;
      t = (b[MAX_Z] - pos) / dir; 
      if (t >= tmin) {
        if (t > tmax * (1.+EPSILON))
          return false;
        tmin = t;
      }
    } else if (dir > 0.) {
      t = (b[MAX_Z] - pos) / dir;
      if (t < tmin)
        return false;
      if (t <= tmax)
        tmax = t;
      t = (b[MIN_Z] - pos) / dir;
      if (t >= tmin) {
        if (t > tmax * (1.+EPSILON))
          return false;
        tmin = t;
      }
    } else if (pos < b[MIN_Z] || pos > b[MAX_Z])
      return false;

    /*
    * If tmin == ray.mindist, then there was no "near"
    * intersection farther than EPSILON away.
    */
    if (tmin == ray.mindist) {
      if (tmax < ray.maxdist) {
        return true;
      }
    } else {
      if (tmin < ray.maxdist) {
        return true;
      }
    }
    return false;	/* hit, but not closer than maxdist */  
  }

  /* returns true if the boundingbox is behind the plane defined by norm and d */
  /* see F. Tampieri, Fast Vertex Radiosity Update, Graphics Gems II, p 303 */
  bool Bounds::is_behind_plane(const R3& norm, const float d)
  {
    R3 P;

    if (norm[X] > 0.)
      P[X] = b[MAX_X];
    else
      P[X] = b[MIN_X];

    if (norm[Y] > 0.)
      P[Y] = b[MAX_Y];
    else
      P[Y] = b[MIN_Y];

    if (norm[Z] > 0.)
      P[Z] = b[MAX_Z];
    else
      P[Z] = b[MIN_Z];

    return R3DOTPRODUCT(norm, P) + d <= 0.;
  }

} // namespace rt
