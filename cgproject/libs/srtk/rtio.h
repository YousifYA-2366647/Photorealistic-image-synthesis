// rtio.h: 

#ifndef _RTIO_H_
#define _RTIO_H_

#ifdef OLD_OSTREAM
#include <ostream.h>
#else
#include <iostream>
#endif
#include "rt.h"
#include "Vec3.h"

namespace rt {

  inline std::ostream& operator<<(std::ostream& s, Texture* t)
  {
    s << t->width << "x" << t->height << "x" << t->channels << " Texture map=" << (void*)t->map << "\n";
    return s;
  }

  inline std::ostream& operator<<(std::ostream& s, const Bounds& b)
  {
    s << b[MIN_X] << " -> " << b[MAX_X] << ", " 
      << b[MIN_Y] << " -> " << b[MAX_Y] << ", " 
      << b[MIN_Z] << " -> " << b[MAX_Z];
    return s;
  }

  inline std::ostream& R3print(std::ostream& s, const R3& v)
  {
    s << "R3(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
    return s;
  }

  inline std::ostream& operator<<(std::ostream& s, const R3& v)
  {
    return R3print(s,v);
  }

  inline std::ostream& operator<<(std::ostream& s, const Vec3& v)
  {
    return s << "Vec3(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
  }

  inline std::ostream& operator<<(std::ostream& s, const Triangle& t)
  {
    R3 *v = t.getCoords();
    return s << v[0] << ", " << v[1] << ", " << v[2];
  }

}  // namespace

#endif /*_RTIO_H_*/
