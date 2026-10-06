/* Bounds.h: bounding boxes */

#ifndef _RT_BOUNDS_H_
#define _RT_BOUNDS_H_

#include "rt.h"
#include "R3.h"

namespace rt {

struct Bounds {

#define MIN_X 0
#define MIN_Y 1
#define MIN_Z 2
#define MAX_X 3
#define MAX_Y 4
#define MAX_Z 5
#define X 0
#define Y 1
#define Z 2

  float b[6];

  inline float operator[](const int i) const { return b[i]; }
  //  inline float& operator[](const int i) { return b[i]; }

  void init(void)
  {
    b[MIN_X] = b[MIN_Y] = b[MIN_Z] = (float)HUGE;
    b[MAX_X] = b[MAX_Y] = b[MAX_Z] = -(float)HUGE;
  }

  void init(const float* const b2)
  {
    b[MIN_X] = b2[MIN_X];
    b[MIN_Y] = b2[MIN_Y];
    b[MIN_Z] = b2[MIN_Z];
    b[MAX_X] = b2[MAX_X];
    b[MAX_Y] = b2[MAX_Y];
    b[MAX_Z] = b2[MAX_Z];
  }

  void initpoint(float* p)
  {
    b[MIN_X] = b[MAX_X] = p[X];
    b[MIN_Y] = b[MAX_Y] = p[Y];
    b[MIN_Z] = b[MAX_Z] = p[Z];
  }

  void init(const Triangle* const tri);

  Bounds()
  {
    init();
  }

  Bounds(const float* const b2)
  {
    init(b2);
  }

  Bounds(const Triangle* const tri)
  {
    init(tri);
  }

  void enlarge(const R3& point);
  void enlarge(const Bounds& b2);
  void enlarge(const Triangle* const tri);

  inline float& operator[](const int i) { return b[i]; }

  inline void get_size(R3& s)
  {
    s[X] = b[MAX_X] - b[MIN_X];
    s[Y] = b[MAX_Y] - b[MIN_Y];
    s[Z] = b[MAX_Z] - b[MIN_Z];
  }

  inline void get_center(R3& c)
  {
    c[X] = 0.5f * (b[MIN_X] + b[MAX_X]);
    c[Y] = 0.5f * (b[MIN_Y] + b[MAX_Y]);
    c[Z] = 0.5f * (b[MIN_Z] + b[MAX_Z]);
  }

  inline void get_min(R3& min) const
  {
    R3SET(min, b[MIN_X], b[MIN_Y], b[MIN_Z]);
  }

  inline void get_max(R3& max) const
  {
    R3SET(max, b[MAX_X], b[MAX_Y], b[MAX_Z]);
  }

  inline void get(R3& min, R3& max) const
  {
    get_min(min); get_max(max);
  }

  inline void set_min(const R3& min)
  {
    R3COPY(min, &b[MIN_X]);
  }

  inline void set_max(const R3& max)
  {
    R3COPY(max, &b[MAX_X]);
  }

  inline void set(const R3& min, const R3& max)
  {
    set_min(min); set_max(max);
  }

  inline bool is_out(const R3& point)
  {
    return ((point[X] < b[MIN_X] || point[X] > b[MAX_X]) ||
	    (point[Y] < b[MIN_Y] || point[Y] > b[MAX_Y]) ||
	    (point[Z] < b[MIN_Z] || point[Z] > b[MAX_Z]));
  }

  bool is_disjunct(const Bounds& b2)
  {
    return ((b[MIN_X] > b2[MAX_X]) || (b2[MIN_X] > b[MAX_X]) ||
	    (b[MIN_Y] > b2[MAX_Y]) || (b2[MIN_Y] > b[MAX_Y]) ||
	    (b[MIN_Z] > b2[MAX_Z]) || (b2[MIN_Z] > b[MAX_Z]));
  }

  // Returns true if ray intersects bounding box and false if not.
  // Fills in minimum and maximum distance to bounding box
  // (tmin == ray.mindist if the ray origin is inside the box)
  bool intersect(Ray& ray, float& tmin, float& tmax);  
  inline bool intersect(Ray& ray)
  {
    float tmin, tmax;
    return intersect(ray, tmin, tmax);
  }

  // Returns true if bounding box is behind plane defined by normal and d
  bool is_behind_plane(const R3& normal, const float d);
};

} // namespace rt

#endif /*_RT_BOUNDS_H_*/
