/* r3.h: macros for 3-float vectors */

#ifndef RT_R3_H
#define RT_R3_H

#include <math.h>

#ifndef EPSILON
#define EPSILON 1e-6
#endif

#ifndef MAX
#define MAX(a,b) (a>b ? a : b)
#endif

#ifndef MIN
#define MIN(a,b) (a<b ? a : b)
#endif

/* fills in x, y, and z component of a vector */
#define R3SET(v, a, b, c)	{(v)[0] = a; (v)[1] = b; (v)[2] = c;}

/* copies the vector v to d: d = v. They may be different vector types. */
#define R3COPY(v, d)	{(d)[0] = (v)[0]; (d)[1] = (v)[1]; (d)[2] = (v)[2];}

/* tolerance value for e.g. a vertex position */
#define R3TOLERANCE(v)  	(EPSILON * (fabs((v)[0]) + fabs((v)[1]) + fabs((v)[2])))

/* two vectors are equal if their components are equal within the given tolerance. */
#define R3EQUAL(v, w, eps)	(FLOATEQUAL((v)[0], (w)[0], (eps)) && \
				 FLOATEQUAL((v)[1], (w)[1], (eps)) && \
				 FLOATEQUAL((v)[2], (w)[2], (eps)))

/* vector difference */
#define R3SUBTRACT R3DIFF
#define R3DIFF(a, b, d) {(d)[0] = (a)[0] - (b)[0]; \
			     (d)[1] = (a)[1] - (b)[1]; \
			     (d)[2] = (a)[2] - (b)[2];}

/* scaled vector difference: d = a-s.b */
#define R3SUBTRACTSCALED R3DIFFSCALED
#define R3DIFFSCALED(a, s, b, d){(d)[0] = (a)[0] - (s) * (b)[0]; \
                                     (d)[1] = (a)[1] - (s) * (b)[1]; \
				     (d)[2] = (a)[2] - (s) * (b)[2];}

/* vector sum: d = a+b */
#define R3ADD R3SUM
#define R3SUM(a, b, d) 	 {(d)[0] = (a)[0] + (b)[0]; \
				  (d)[1] = (a)[1] + (b)[1]; \
				  (d)[2] = (a)[2] + (b)[2];}

/* scaled vector sum: d = a+s.b */
#define R3ADDSCALED R3SUMSCALED
#define R3SUMSCALED(a, s, b, d) 	 {(d)[0] = (a)[0] + (s) * (b)[0]; \
					  (d)[1] = (a)[1] + (s) * (b)[1]; \
				          (d)[2] = (a)[2] + (s) * (b)[2];}

/* scalar vector product: a.b */
#define R3DOTPRODUCT(a, b) 	((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])

/* square of vector norm: scalar product with itself */
#define R3NORM2(v)		  ((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2])	 	

/* norm of a vector: sqaure root of the sqaure norm */
#define R3NORM(v)		  (double)sqrt((double)R3NORM2(v))

/* scale a vector: d = s.v (s is a real number) */
#define R3SCALE(s, v, d)	  {(d)[0] = (s) * (v)[0]; \
				   (d)[1] = (s) * (v)[1]; \
				   (d)[2] = (s) * (v)[2];}

/* scales a vector with the inverse of the real number s if not zero: d = (1/s).v */
#define R3SCALEINVERSE(s, v, d)	  {double _is_ = ((s) < -EPSILON || (s) > EPSILON ? 1./(s) : 1.); \
						   (d)[0] = _is_ * (v)[0]; \
						   (d)[1] = _is_ * (v)[1]; \
						   (d)[2] = _is_ * (v)[2];}

/* normalizes a vector: scale it with the inverse of its norm */
#define R3NORMALISE R3NORMALIZE
#define R3NORMALIZE(v)	{double _norm_; _norm_ = R3NORM(v); \
				 R3SCALEINVERSE(_norm_, v, v);}

/* inproduct of two vectors. */
#define R3CROSSPRODUCT(a, b, d)	{DR3 _d; \
					 (_d)[0] = (a)[1] * (b)[2] - (a)[2] * (b)[1]; \
					 (_d)[1] = (a)[2] * (b)[0] - (a)[0] * (b)[2]; \
					 (_d)[2] = (a)[0] * (b)[1] - (a)[1] * (b)[0]; \
				         R3COPY(_d, (d));}

/* normal of the triangle defined by three non-colinear vectors:
 * n = (v2-v1) x (v3-v2) normalized */
#define NORMAL(v1, v2, v3, n)		{DR3 _d1_, _d2_; \
					 R3SUBTRACT(v2, v1, _d1_); \
					 R3SUBTRACT(v3, v2, _d2_); \
					 R3CROSSPROD(_d1_, _d2_, n); \
					 R3NORMALIZE(n); }

/* linear combination of two vectors: d = a.v + b.w */
#define R3COMB2(a, v, b, w, d)	{(d)[0] = (a) * (v)[0] + (b) * (w)[0]; \
				 (d)[1] = (a) * (v)[1] + (b) * (w)[1]; \
				 (d)[2] = (a) * (v)[2] + (b) * (w)[2];}

/* affine linear combination of two vectors: d = o + a.v + b.w */
#define R3COMB3(o, a, v, b, w, d)	{(d)[0] = (o)[0] + (a) * (v)[0] + (b) * (w)[0]; \
				 (d)[1] = (o)[1] + (a) * (v)[1] + (b) * (w)[1]; \
				 (d)[2] = (o)[2] + (a) * (v)[2] + (b) * (w)[2];}

/* linear combination of three vectors */
#define R3COORD(a, X, b, Y, c, Z, d) {(d)[0] = (a) * (X)[0] + (b) * (Y)[0] + (c) * (Z)[0]; \
					  (d)[1] = (a) * (X)[1] + (b) * (Y)[1] + (c) * (Z)[1]; \
					  (d)[2] = (a) * (X)[2] + (b) * (Y)[2] + (c) * (Z)[2];}

/* triple (cross) product: d = (v3-v2) x (v1-v2) */
#define R3TRIPLECROSSPRODUCT(v1, v2, v3, d) {DR3 _D1, _D2;	\
	R3SUBTRACT(v3, v2, _D1);					\
	R3SUBTRACT(v1, v2, _D2);					\
	R3CROSSPRODUCT(_D1, _D2, d)					\
				      }

/* triple (dot) product: s = (v3-v2) . (v1-v2) */
#define R3TRIPLEDOTPRODUCT(v1, v2, v3, s) {DR3 _D1, _D2;	\
	R3SUBTRACT(v3, v2, _D1);					\
	R3SUBTRACT(v1, v2, _D2);					\
	s = R3DOTPRODUCT(_D1, _D2)					\
				      }

/* determinant s = (v1 x v2) . v3 = v1 . (v2 x v3) */
#define R3DETERMINANT(v1, v2, v3, s) {DR3 _D;	\
        R3CROSSPRODUCT(v1, v2, _D);			\
        s = R3DOTPRODUCT(_D, v3);			\
				       }

/* distance between two points in 3D space: s = |p2-p1| */
#define R3DIST(p1, p2, s) {	DR3 _D; 	\
        R3SUBTRACT(p2, p1, _D);		\
	s = R3NORM(_D);			\
}

/* squared distance between two points in 3D space: s = |p2-p1| */
#define R3DIST2(p1, p2, s) {	DR3 _D; 	\
        R3SUBTRACT(p2, p1, _D);		\
	s = R3NORM2(_D);			\
			      }

/* orthogonal component of X with respect to Y. Result is stored in d */
#define R3ORTHOCOMP(X, Y, d)	{double _dotp_;	\
	_dotp_ = R3DOTPRODUCT(X, Y);		\
        R3SUMSCALED(X, -_dotp_, Y, d);		\
				 }

#define XNORMAL 0
#define YNORMAL 1
#define ZNORMAL 2
#define R3INDEXNAME(index)	((index) == XNORMAL ? "XNORMAL" :	\
				 ((index) == YNORMAL ? "YNORMAL" : "ZNORMAL"))

/* given a vector p in 3D space and an index i, which is 
 * XNORMAL, YNORMAL or ZNORMAL, projects the vector on the YZ, XZ or XY plane 
 * respectively. */
#define R3PROJECT(r, p, i)	{switch(i) { \
				case XNORMAL: \
					r.u = (p)[1]; \
					r.v = (p)[2]; \
					break; \
				case YNORMAL: \
					r.u = (p)[0]; \
					r.v = (p)[2]; \
					break; \
				case ZNORMAL: \
					r.u = (p)[0]; \
					r.v = (p)[1]; \
					break; \
  				} }

/* centre of two points */
#define MIDPOINT(p1, p2, m)	{ 	\
	(m)[0] = 0.5 * ((p1)[0] + (p2)[0]);	\
	(m)[1] = 0.5 * ((p1)[1] + (p2)[1]);	\
	(m)[2] = 0.5 * ((p1)[2] + (p2)[2]);	\
				}

/* centre of four points */
#define MIDPOINT4(p1, p2, p3, p4, m)	{ 	\
	(m)[0] = 0.25 * ((p1)[0] + (p2)[0] + (p3)[0] + (p4)[0]);	\
	(m)[1] = 0.25 * ((p1)[1] + (p2)[1] + (p3)[1] + (p4)[1]);	\
	(m)[2] = 0.25 * ((p1)[2] + (p2)[2] + (p3)[2] + (p4)[2]);	\
				}

/* sum of four vectors */
#define R3SUM4(v1, v2, v3, v4, s)	{ 		\
	(s)[0] = (v1)[0] + (v2)[0] + (v3)[0] + (v4)[0];	\
	(s)[1] = (v1)[1] + (v2)[1] + (v3)[1] + (v4)[1];	\
	(s)[2] = (v1)[2] + (v2)[2] + (v3)[2] + (v4)[2];	\
				}


/* Point IN Triangle: barycentric parametrisation */
#define PINT(v0, v1, v2, u, v, p)     {						\
        double _u = (u), _v = (v); 						\
	(p)[0] = (v0)[0] + _u * ((v1)[0] - (v0)[0]) + _v * ((v2)[0] - (v0)[0]);	\
	(p)[1] = (v0)[1] + _u * ((v1)[1] - (v0)[1]) + _v * ((v2)[1] - (v0)[1]);	\
	(p)[2] = (v0)[2] + _u * ((v1)[2] - (v0)[2]) + _v * ((v2)[2] - (v0)[2]);	\
				      }

/* Point IN Quadrilateral: bilinear parametrisation */
#define PINQ(v0, v1 ,v2 ,v3, u, v, p)	{	\
        double _c=(u)*(v), _b=(u)-_c, _d=(v)-_c;	\
	(p)[0] = (v0)[0] + (_b) * ((v1)[0] - (v0)[0]) + (_c) * ((v2)[0] - (v0)[0])+ (_d) * ((v3)[0] - (v0)[0]);	\
	(p)[1] = (v0)[1] + (_b) * ((v1)[1] - (v0)[1]) + (_c) * ((v2)[1] - (v0)[1])+ (_d) * ((v3)[1] - (v0)[1]);	\
	(p)[2] = (v0)[2] + (_b) * ((v1)[2] - (v0)[2]) + (_c) * ((v2)[2] - (v0)[2])+ (_d) * ((v3)[2] - (v0)[2]);	\
					};

/* computes d = (1-s).p + s.q = p + s.(q-p) */
#define R3INTERPOLATE(p,q,s,d)  {double _s = (s);	\
	(d)[0] = (p)[0] + _s * ((q)[0] - (p)[0]); \
	(d)[1] = (p)[1] + _s * ((q)[1] - (p)[1]); \
	(d)[2] = (p)[2] + _s * ((q)[2] - (p)[2]); \
}

/* maximum of two vectors: d[0] = max(v1[0],v2[0]) etc ...*/
#define R3MAX(v1,v2,d) {		\
        (d)[0] = MAX((v1)[0], (v2)[0]);	\
        (d)[1] = MAX((v1)[1], (v2)[1]);	\
        (d)[2] = MAX((v1)[2], (v2)[2]);	\
}

/* minimum of two vectors: d[0] = min(v1[0],v2[0]) etc ...*/
#define R3MIN(v1,v2,d) {		\
        (d)[0] = MIN((v1)[0], (v2)[0]);	\
        (d)[1] = MIN((v1)[1], (v2)[1]);	\
        (d)[2] = MIN((v1)[2], (v2)[2]);	\
}

/* transforms a vector to the first quadrant by replacing its components
 * by their absolute value */
#define R3ABS(v,d) {	\
        (d)[0] = fabs((v)[0]); 	\
        (d)[1] = fabs((v)[1]); 	\
        (d)[2] = fabs((v)[2]); 	\
}

// returns code telling how two vectors compare. Code is
// combination of X|Y|Z_GREATER or XYZ_EQUAL if equal within
// tolerance.
#define X_GREATER 1
#define Y_GREATER 2
#define Z_GREATER 4
#define XYZ_EQUAL 8
inline int R3Compare(const float* const v1, const float* const v2, const float tolerance)
{
  int code = 0;
  if (v1[0] > v2[0] + tolerance) code += X_GREATER;
  if (v1[1] > v2[1] + tolerance) code += Y_GREATER;
  if (v1[2] > v2[2] + tolerance) code += Z_GREATER;
  if (code!=0) 	/* x1 > x2 || y1 > y2 || z1 > z2 */
    return code;

  if (v1[0] < v2[0] - tolerance ||
      v1[1] < v2[1] - tolerance ||
      v1[2] < v2[2] - tolerance)
    return code;	/* not the same coordinates */

  return XYZ_EQUAL;	/* same coordinates */  
}

#endif /* RT_R3_H */
