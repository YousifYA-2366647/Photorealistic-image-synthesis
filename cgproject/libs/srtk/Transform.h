// Transform.h

#ifndef _RT_TRANSFORM_H_
#define _RT_TRANSFORM_H_

#include "R3.h"

namespace rt {

  struct Transform {
    float m[4][4];

    void set(const float* m4x4)
    {
      m[0][0] = m4x4[0]; 
      m[0][1] = m4x4[1];
      m[0][2] = m4x4[2];
      m[0][3] = m4x4[3];
      m[1][0] = m4x4[4];
      m[1][1] = m4x4[5];
      m[1][2] = m4x4[6];
      m[1][3] = m4x4[7];
      m[2][0] = m4x4[8];
      m[2][1] = m4x4[9];
      m[2][2] = m4x4[10];
      m[2][3] = m4x4[11];
      m[3][0] = m4x4[12];
      m[3][1] = m4x4[13];
      m[3][2] = m4x4[14];
      m[3][3] = m4x4[15];
    }

    void set_transpose(const float* m4x4)
    {
      m[0][0] = m4x4[0]; 
      m[1][0] = m4x4[1];
      m[2][0] = m4x4[2];
      m[3][0] = m4x4[3];
      m[0][1] = m4x4[4];
      m[1][1] = m4x4[5];
      m[2][1] = m4x4[6];
      m[3][1] = m4x4[7];
      m[0][2] = m4x4[8];
      m[1][2] = m4x4[9];
      m[2][2] = m4x4[10];
      m[3][2] = m4x4[11];
      m[0][3] = m4x4[12];
      m[1][3] = m4x4[13];
      m[2][3] = m4x4[14];
      m[3][3] = m4x4[15];
    }

    void set_identity(void)
    {
      m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.;
      m[0][1] = m[0][2] = m[0][3] =
      m[1][0] = m[1][2] = m[1][3] =
      m[2][0] = m[2][1] = m[2][3] =
      m[3][0] = m[3][1] = m[3][2] = 0;
    }

    void set_scale(float* xyz)
    {
      set_identity();
      m[0][0] = xyz[0];
      m[1][1] = xyz[1];
      m[2][2] = xyz[2];
    }

    void set_translate(float* xyz)
    {
      set_identity();
      m[0][3] = xyz[0];
      m[1][3] = xyz[1];
      m[2][3] = xyz[2];
    }

    void set_rotate(float radians, float* d)
    {
      double x,y,z,c,s,t;
      set_identity();
      if ((s = R3NORM(d)) > EPSILON) {
	R3SCALEINVERSE(s, d, d);   // normalize
	x=d[0]; y=d[1]; z=d[2];
	c=cos(radians); s=sin(radians); t=1-c;
	m[0][0] = x*x*t + c  ; m[0][1] = x*y*t - z*s; m[0][2] = x*z*t + y*s;
	m[1][0] = x*y*t + z*s; m[1][1] = y*y*t + c  ; m[1][2] = y*z*t - x*s;
	m[2][0] = x*z*t - y*s; m[2][1] = y*z*t + x*s; m[2][2] = z*z*t + c  ;
      }
    }

    Transform compose(const Transform& xf)  // xf applied first
    {
      Transform r;
      for (int j=0; j<=3; j++)
	for (int i=0; i<=3; i++)
	  r.m[i][j] = m[i][0] * xf.m[0][j] 
	            + m[i][1] * xf.m[1][j] 
	            + m[i][2] * xf.m[2][j] 
	            + m[i][3] * xf.m[3][j];
      return r;
    }

    void apply_affine_to_point(const float* p, float* result)
    {
      float r[3];
      for (int i=0; i<=2; i++)
	r[i] = m[i][0] * p[0] + m[i][1] * p[1] + m[i][2] * p[2] + m[i][3];
      R3COPY(r, result);
    }

    void apply_affine_to_vector(const float* v, float* result)
    {
      float r[3];
      for (int i=0; i<=2; i++)
	r[i] = m[i][0] * v[0] + m[i][1] * v[1] + m[i][2] * v[2];
      R3COPY(r, result);
    }

    void apply_affine_to_normal(const float* n, float* result)
    {
      float r[3];
      for (int i=0; i<=2; i++)
	r[i] = n[0] * m[0][i] + n[1] * m[1][i] + n[2] * m[2][i];
      R3NORMALIZE(r);
      R3COPY(r, result);
    }

    void scale(float* xyz)       // composes with scale transform
    {
      Transform xf;
      xf.set_scale(xyz);
      *this = compose(xf);
    }

    void translate(float* xyz)
    {
      Transform xf;
      xf.set_translate(xyz);
      *this = compose(xf);
    }

    void rotate(float radians, float* xyz)
    {
      Transform xf;
      xf.set_rotate(radians, xyz);
      *this = compose(xf);
    }

    void print(std::ostream& s)
    {
      s << "\n"
	   << "   " << m[0][0] << "   " << m[0][1] << "   " << m[0][2] << "   " << m[0][3] << "\n"
	   << "   " << m[1][0] << "   " << m[1][1] << "   " << m[1][2] << "   " << m[1][3] << "\n"
	   << "   " << m[2][0] << "   " << m[2][1] << "   " << m[2][2] << "   " << m[2][3] << "\n"
	   << "   " << m[3][0] << "   " << m[3][1] << "   " << m[3][2] << "   " << m[3][3] << "\n";
    }

    friend std::ostream& operator<<(std::ostream& s, Transform& t)
    {
      t.print(s);
      return s;
    }

  };

}

#endif /* _RT_TRANSFORM_H_ */
