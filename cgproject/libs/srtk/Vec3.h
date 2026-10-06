// Vec3.h

#ifndef _RT_VEC3_H_
#define _RT_VEC3_H_

namespace rt {

  struct Vec3 {
    float v[3];

    inline void set(const float* w) { v[0]=w[0]; v[1]=w[1]; v[2]=w[2]; }
    
    inline float operator[](const int index) const { return v[index]; }
    inline float& operator[](const int index) { return v[index]; }
  };

} // namespace rt

#endif /* _RT_VEC3_H_ */
