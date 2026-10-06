/* info ************************************************************************************

last update:
    June, 03 2003

author:
    tom.mertens@luc.ac.be
    philippe.bekaert@luc.ac.be

content:
    implementation and definition of following classes
    * vec3: default 3-component vector (x,y,z)
    * vec4: default 4-component vector for homogeneous coordinates (x,y,z,w)
    * mtx3: default 3x3 matrix, row major
    * mtx4: default 4x4 matrix, row major
    * quat: default quaternion (s,v1,v2,v3)
    * vec3f, vec4f, mtx3f, mtx4f, quatf: variants with single precision (float)
    * components
    * vec3d, vec4d, mtx3d, mtx4d, quatd: variants with double precision (double)
    * components
    * vec3t<T_vecmath>, vec4t<T_vecmath>,...: variants with components of type
    * 'T_vecmath'

TODO
    typecasts: float to double mtx3, etc...
    efficiency in subdeterminant()
    complete all operators
    normalize matrix
    w-coordinate for vectors/points?

ChangeLog:

20030602 - PhB  INCOMPATIBLE CHANGES!!!!!
* dot product is operator& instead of operator*
* cross product is operator^ instead of operator%
* Warning: use brackets to ensure proper precedence:
   a&b*c is interpreted as a & (b*c), not (a&b) * c
   a^b*c                   a ^ (b*c)      (a^b) * c
* component-wise multiplication always is operator* now
* added vec3<> normalized(T_vecmath*) member function returning normalized
* vector and optionally it's length
* added vec3<> operator/[=](scalar)
* added vec3<> min_comp, max_comp, min_comp_idx, max_comp_idx, min, max, abs
* renamed vec3<> [min|max|mid]_abs_comp into [min|max|mid]_abs_comp_idx,
* re-implemented vec3<> mid_abs_comp_idx
* added vec3<> is_zero(tolerance)
* added const in several places (should add more const)
* tagged several "TODO's"

20030603 PhB
* added operator<<(ostream&, const vec3t&)
* added const in some more places (still not all)

20030605 PhB
* added vec2::operator^ and U() and V() member selectors.

20030606 PhB
* added vec[234]t::get member functions for retrieving components into
array

20041005 PhB
* changed #include <ostream.h> into modern #include <ostream>
* prepended std:: to ostream allover the place

*******************************************************************************/

#ifndef VECMATH_H
#define VECMATH_H

#include <math.h>
#include <ostream>

// remove unwanted definitions of min and max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

//define default vecmath type
#define VECMATH_DEFAULT_TYPE    float

//set default type
#define vec2            vec2t<VECMATH_DEFAULT_TYPE>
#define vec3            vec3t<VECMATH_DEFAULT_TYPE>
#define vec4            vec4t<VECMATH_DEFAULT_TYPE>
#define mtx3            mtx3t<VECMATH_DEFAULT_TYPE>
#define mtx4            mtx4t<VECMATH_DEFAULT_TYPE>
#define quat            quatt<VECMATH_DEFAULT_TYPE>

//define classes for single and double presicion
#define vec2f           vec2t<float>
#define vec2d           vec2t<double>
#define vec3f           vec3t<float>
#define vec3d           vec3t<double>
#define vec4f           vec4t<float>
#define vec4d           vec4t<double>
#define mtx3f           mtx3t<float>
#define mtx3d           mtx3t<double>
#define mtx4f           mtx4t<float>
#define mtx4d           mtx4t<double>
#define quatf           quatt<float>
#define quatd           quatt<double>

//useful constants
#ifndef PI
#define PI ((VECMATH_DEFAULT_TYPE)3.141592654)
#endif
#ifndef DEGTORAD
#define DEGTORAD ((VECMATH_DEFAULT_TYPE)0.01745329252)
#endif
#ifndef RADTODEG
#define RADTODEG ((VECMATH_DEFAULT_TYPE)57.2957795056)
#endif
#ifndef PIf
#define PIf 3.141592654f
#endif
#ifndef DEGTORADf
#define DEGTORADf 0.01745329252f
#endif
#ifndef RADTODEGf
#define RADTODEGf 57.2957795056f
#endif
#ifndef PId
#define PId 3.141592654
#endif
#ifndef DEGTORADd
#define DEGTORADd 0.01745329252
#endif
#ifndef RADTODEGd
#define RADTODEGd 57.2957795056
#endif
#ifndef M_PI
#define M_PI PI
#endif

//predefine classes
template <class T_vecmath> class vec2t;
template <class T_vecmath> class vec3t;
template <class T_vecmath> class vec4t;
template <class T_vecmath> class quatt;
template <class T_vecmath> class mtx3t;
template <class T_vecmath> class mtx4t;

//quick conversion macros:
//e.g. QUAT(x) turns any vecmath.h object x into a quaternion
//use macros with 'p'-postfix for pointers to vecmath.h objects
//component types must match between source and target object!
#define VEC3(x)     (*((vec3*)&(x)))
#define VEC4(x)     (*((vec4*)&(x)))
#define QUAT(x)     (*((quat*)&(x)))
#define VEC3p(x)    (*((vec3*)(x)))
#define VEC4p(x)    (*((vec4*)(x)))
#define QUATp(x)    (*((quat*)(x)))
#define VEC3f(x)    (*((vec3f*)&(x)))
#define VEC4f(x)    (*((vec4f*)&(x)))
#define QUATf(x)    (*((quatf*)&(x)))
#define VEC3fp(x)   (*((vec3f*)(x)))
#define VEC4fp(x)   (*((vec4f*)(x)))
#define QUATfp(x)   (*((quatf*)(x)))
#define VEC3d(x)    (*((vec3d*)&(x)))
#define VEC4d(x)    (*((vec4d*)&(x)))
#define QUATd(x)    (*((quatd*)&(x)))
#define VEC3dp(x)   (*((vec3d*)(x)))
#define VEC4dp(x)   (*((vec4d*)(x)))
#define QUATdp(x)   (*((quatd*)(x)))

//make sure to pick appropriate routines from 'math.h':
//e.g. use 'sqrtf(float)' for float components, 'sqrt(double)' for the rest
template<class T_vecmath> inline T_vecmath vecmath_sin(T_vecmath arg) {
    return (T_vecmath)sin((double)arg);
}
template<class T_vecmath> inline T_vecmath vecmath_cos(T_vecmath arg) {
    return (T_vecmath)cos((double)arg);
}
template<class T_vecmath> inline T_vecmath vecmath_sqrt(T_vecmath arg) {
    return (T_vecmath)sqrt((double)arg);
}
template<class T_vecmath> inline T_vecmath vecmath_abs(T_vecmath arg) {
    return (T_vecmath)fabs((double)arg);
}
template<> inline float vecmath_cos(float arg) {
    return cosf(arg);
}
template<> inline float vecmath_sin(float arg) {
    return sinf(arg);
}
template<> inline float vecmath_sqrt(float arg) {
    return sqrtf(arg);
}
template<> inline float vecmath_abs(float arg) {
    return fabsf(arg);
}

#define vecmath_min(a,b) (((a)<(b)) ? (a) : (b))
#define vecmath_max(a,b) (((a)>(b)) ? (a) : (b))
#define vecmath_zero(a,tolerance) ((a)<(tolerance) && (a)>-(tolerance))

////////////////////
//class definition//
////////////////////

//////////////////////////////////////////////////////
//CLASS vec2t                                       //
//                                                  //
//2-component vector                                //
//////////////////////////////////////////////////////
template <class T_vecmath> class vec2t {
public:
    //ctor
    inline vec2t() {};
    inline vec2t(T_vecmath x, T_vecmath y) {
        c[0] = x; c[1] = y;
    }
    inline vec2t(T_vecmath* ptr) { //copy from pointer
        c[0] = ptr[0]; c[1] = ptr[1];
    }
    template <class T_vecmath_other> inline vec2t<T_vecmath>(const
            vec2t<T_vecmath_other>& o) {
        c[0] = (T_vecmath)o.X();  c[1] = (T_vecmath)o.Y();
    }
    //assignment
    inline void operator=(const vec2t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1];
    }

    //access
    inline T_vecmath* ptr() const {
        return (T_vecmath*)c;
    }
    inline operator T_vecmath* () const {
        return (T_vecmath*)c;
    }
    inline T_vecmath X() const {
        return c[0];
    }
    inline T_vecmath Y() const {
        return c[1];
    }
    inline T_vecmath U() const {
        return c[0];
    }
    inline T_vecmath V() const {
        return c[1];
    }
    inline T_vecmath& operator[](int i) {
        return c[i];
    }
    inline void set(T_vecmath* ptr) {
        c[0] = ptr[0]; c[1] = ptr[1];
    }
    inline void get(T_vecmath* ptr) {
        ptr[0] = c[0]; ptr[1] = c[1];
    }

    //operations
    //addition
    inline const vec2t<T_vecmath> operator+(const vec2t<T_vecmath>& o) const {
        return vec2t<T_vecmath>(c[0] + o.c[0],
                                c[1] + o.c[1]);
    }
    inline void operator+=(const vec2t<T_vecmath>& o) {
        c[0] += o.c[0];
        c[1] += o.c[1];
    }
    //subtraction
    inline const vec2t<T_vecmath> operator-(const vec2t<T_vecmath>& o) const {
        return vec2t<T_vecmath>(c[0] - o.c[0],
                                c[1] - o.c[1]);
    }
    inline void operator-=(const vec2t<T_vecmath>& o) {
        c[0] -= o.c[0];
        c[1] -= o.c[1];
    }
    //component-wise multiplication
    inline const vec2t<T_vecmath> operator*(const vec2t<T_vecmath>& o) const {
        return vec2t<T_vecmath>(c[0] * o.c[0], c[1] * o.c[1]);
    }
    //scalar multiplication
    // vec2 * scalar
    inline const vec2t<T_vecmath> operator*(const T_vecmath s) const {
        return vec2t<T_vecmath>(s * c[0], s * c[1]);
    }
    // scalar * vec2
    friend inline const vec2t<T_vecmath> operator*(const T_vecmath s,
            const vec2t& v) {
        return vec2t<T_vecmath>(s * v.c[0], s * v.c[1]);
    }
    inline void operator*=(const T_vecmath s) {
        c[0] *= s;
        c[1] *= s;
    }
    // division by scalar
    inline const vec2t<T_vecmath> operator/(const T_vecmath s) const {
        T_vecmath is = (s != 0.) ? (1. / s) : 1.;
        return vec2t<T_vecmath>(c[0] * is, c[1] * is);
    }
    inline void operator/=(const T_vecmath s) {
        T_vecmath is = (s != 0.) ? (1. / s) : 1.;
        c[0] *= is;
        c[1] *= is;
    }
    //dot product
    // TODO: should have higher precedence than * rather than lower
    inline T_vecmath operator&(const vec2t<T_vecmath>& o) const {
        return c[0] * o.c[0] + c[1] * o.c[1];
    }
    //cross product (scalar result)
    inline T_vecmath operator^(const vec2t<T_vecmath>& o) const {
        return c[0] * o.c[1] - c[1] * o.c[0];
    }
    // negation (unary)
    inline const vec2t<T_vecmath> operator-() const {
        return vec2t<T_vecmath>(-c[0], -c[1]);
    }

private:
    T_vecmath c[2];
};

//////////////////////////////////////////////////////
//CLASS vec3t                                       //
//                                                  //
//3-component vector                                //
//////////////////////////////////////////////////////
template <class T_vecmath> class vec3t {
    friend class mtx3t<T_vecmath>;
    friend class mtx4t<T_vecmath>;
    friend class vec4t<T_vecmath>;
    friend class quatt<T_vecmath>;

public:
    //ctor
    inline vec3t() {};
    inline vec3t(const T_vecmath x, const T_vecmath y, const T_vecmath z) {
        c[0] = x; c[1] = y; c[2] = z;
    }

    inline vec3t(const T_vecmath* ptr) { //copy from pointer
        c[0] = ptr[0]; c[1] = ptr[1]; c[2] = ptr[2];
    }

    template <class T_vecmath_other> inline vec3t<T_vecmath>(
            const vec3t<T_vecmath_other>& o) {
        //c[0]=o.c[0]; c[1]=o.c[1]; c[2]=o.c[2];
        c[0] = (T_vecmath)o.X();  c[1] = (T_vecmath)o.Y();  c[2] =
            (T_vecmath)o.Z();
    }

    inline vec3t<T_vecmath>(const vec4t<T_vecmath>& o);

    //assignment
    inline void operator=(const vec3t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
    }

    inline void operator=(const vec4t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
    }

    //access
    inline T_vecmath* ptr() const {
        return (T_vecmath*)c;
    }

    inline operator T_vecmath* () const {
        return (T_vecmath*)c;
    }

    inline T_vecmath X() const {
        return c[0];
    }

    inline T_vecmath Y() const {
        return c[1];
    }

    inline T_vecmath Z() const {
        return c[2];
    }

    inline T_vecmath& operator[](int i) {
        return c[i];
    }

    inline const T_vecmath& operator[](int i) const {
        return c[i];
    }

    inline void set(T_vecmath* ptr) {
        c[0] = ptr[0]; c[1] = ptr[1]; c[2] = ptr[2];
    }

    inline void get(T_vecmath* ptr) {
        ptr[0] = c[0]; ptr[1] = c[1]; ptr[2] = c[2];
    }

    //operations
    //addition
    inline const vec3t<T_vecmath> operator+(const vec3t<T_vecmath>& o) const {
        return vec3t<T_vecmath>(c[0] + o.c[0],
                                c[1] + o.c[1],
                                c[2] + o.c[2]);
    }

    inline void operator+=(const vec3t<T_vecmath>& o) {
        c[0] += o.c[0];
        c[1] += o.c[1];
        c[2] += o.c[2];
    }

    inline void operator+=(vec4t<T_vecmath>& o);

    //subtraction
    inline const vec3t<T_vecmath> operator-(const vec3t<T_vecmath>& o) const {
        return vec3t<T_vecmath>(c[0] - o.c[0],
                                c[1] - o.c[1],
                                c[2] - o.c[2]);
    }

    inline void operator-=(const vec3t<T_vecmath>& o) {
        c[0] -= o.c[0];
        c[1] -= o.c[1];
        c[2] -= o.c[2];
    }

    //component-wise multiplication
    inline const vec3t<T_vecmath> operator*(const vec3t<T_vecmath>& o) const {
        return vec3t<T_vecmath>(c[0] * o.c[0], c[1] * o.c[1] , c[2] * o.c[2]);
    }

    //scalar multiplication
    inline const vec3t<T_vecmath> operator*(const T_vecmath s) const {
        return vec3t<T_vecmath>(s * c[0], s * c[1], s * c[2]);
    }

    friend inline const vec3t<T_vecmath> operator*(const T_vecmath s,
            const vec3t& v) {
        return vec3t<T_vecmath>(s * v.c[0], s * v.c[1], s * v.c[2]);
    }

    inline void operator*=(const T_vecmath s) {
        c[0] *= s;
        c[1] *= s;
        c[2] *= s;
    }

    // division by scalar (checks for zero arg.)
    inline const vec3t<T_vecmath> operator/(const T_vecmath t) const {
        T_vecmath s = (t != 0.) ? (1. / t) : 1.;
        return vec3t<T_vecmath>(s * c[0], s * c[1], s * c[2]);
    }

    inline const vec3t<T_vecmath> operator/=(const T_vecmath t) {
        T_vecmath s = (t != 0.) ? (1. / t) : 1.;
        c[0] *= s;
        c[1] *= s;
        c[2] *= s;
        return *this;
    }

    //dot product
    // TODO: precedence should be higher than * rather than lower
    inline T_vecmath operator&(const vec3t<T_vecmath>& o) const {
        return c[0] * o.c[0] + c[1] * o.c[1] + c[2] * o.c[2];
    }

    //cross product
    // TODO: precedence should be same as * rather than lower
    inline const vec3t<T_vecmath> operator^(const vec3t<T_vecmath>& o) const {
        return vec3t<T_vecmath>(c[1] * o.c[2] - c[2] * o.c[1],
                                c[2] * o.c[0] - c[0] * o.c[2],
                                c[0] * o.c[1] - c[1] * o.c[0]);
    }

    inline const vec3t<T_vecmath> operator^=(const vec3t<T_vecmath>& o) {
        *this = *this ^ o;
        return *this;
    }

    //flipping
    inline void flip() {
        c[0] = -c[0];
        c[1] = -c[1];
        c[2] = -c[2];
    }

    friend inline vec3t<T_vecmath> flipped(vec3t& v) {
        return vec3t<T_vecmath>(-v.c[0], -v.c[1], -v.c[2]);
    }

    inline const vec3t<T_vecmath> operator-() const {
        return vec3t<T_vecmath>(-c[0], -c[1], -c[2]);
    }

    //distance & length
    inline T_vecmath square_length() const {
        return c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
    }

    inline T_vecmath length() const {
        return vecmath_sqrt(square_length());
    }

    friend inline T_vecmath euclidian_distance(vec3t a, vec3t b) {
        return (a - b).length();
    }

    //normalization
    inline void normalize() {
        *this /= length();
    }

    inline void normalize(T_vecmath* len) {
        *len = length();
        *this /= *len;
    }

    inline const vec3t<T_vecmath> normalized(void) const {
        return *this / length();
    }

    inline const vec3t<T_vecmath> normalized(T_vecmath* len) const {
        *len = length();
        return *this / *len;
    }

    friend inline const vec3t<T_vecmath> normalized(const vec3t& v) {
        return v.normalized();
    }

    //star (unary) operator
    inline mtx3t<T_vecmath> star() const;

    //spherical coordinates:
    //we want to convert a 3D vector v to spherical coordinates (r,phi,theta)
    //and vice versa
    //orientation is as follows:
    //  XZ is equator plane
    //  Y is up vector (or normal vector in case of BRDF parametrizations)
    //  phi = azimutal angle = angle between X-axis and v projected on XZ
    //  theta = elevation angle = angle between Y-axis and v
    inline void from_spherical(T_vecmath r, T_vecmath phi, T_vecmath theta) {
        c[0] = r * vecmath_sin(theta) * vecmath_cos(phi);
        c[1] = r * vecmath_cos(theta);
        c[2] = r * vecmath_sin(theta) * vecmath_sin(phi);
    }
    // we always return phi in [-pi,pi] and theta in [0,pi]
    inline void to_spherical(T_vecmath& r, T_vecmath& phi,
            T_vecmath& theta) const {
        r = vecmath_sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
        phi = atan2f(c[2], c[0]);
        theta = acosf(c[1] / r);
    }

    //min/max/abs components
    inline T_vecmath min_comp(void) const {
        return c[0] < c[1] && c[0] < c[2] ? c[0] : (c[1] < c[2] ? c[1] : c[2]);
    }

    inline T_vecmath max_comp(void) const {
        return c[0] > c[1] && c[0] > c[2] ? c[0] : (c[1] > c[2] ? c[1] : c[2]);
    }

    inline T_vecmath min_comp_idx(void) const {
        return c[0] < c[1] && c[0] < c[2] ? 0 : (c[1] < c[2] ? 1 : 2);
    }

    inline T_vecmath max_comp_idx(void) const {
        return c[0] > c[1] && c[0] > c[2] ? 0 : (c[1] > c[2] ? 1 : 2);
    }

    inline const vec3t<T_vecmath> min(const vec3t<T_vecmath>& o) const {
        return vec3t<T_vecmath>(vecmath_min(c[0], o.c[0]),
                                vecmath_min(c[1], o.c[1]),
                                vecmath_min(c[2], o.c[2]));
    }

    inline const vec3t<T_vecmath> max(const vec3t<T_vecmath>& o) const {
        return vec3t<T_vecmath>(vecmath_max(c[0], o.c[0]),
                                vecmath_max(c[1], o.c[1]),
                                vecmath_max(c[2], o.c[2]));
    }

    inline const vec3t<T_vecmath> abs(void) const {
        return vec3t<T_vecmath>(vecmath_abs(c[0]), vecmath_abs(c[1]),
                vecmath_abs(c[2]));
    }

    inline int min_abs_comp_idx() const { // x->0, y->1, z->2
        return min_comp_idx(abs());
    }

    inline int max_abs_comp_idx() const { // x->0, y->1, z->2
        return min_comp_idx(abs());
    }

    inline int mid_abs_comp_idx() const { // x->0, y->1, z->2
        vec3t<T_vecmath> tabs = abs();
        int cmin = min_comp_idx(tabs);
        int cmax = max_comp_idx(tabs);
        return 3 - cmin - cmax;
    }

    // sum of components
    inline T_vecmath sum_comp(void) const {
        return c[0] + c[1] + c[2];
    }

    // sum of absolute value of the components
    inline T_vecmath sum_abs_comp(void) const {
        return vecmath_abs(c[0]) + vecmath_abs(c[1]) + vecmath_abs(c[2]);
    }

    inline bool is_zero(void) const {
        return (c[0] == 0.0f && c[1] == 0.0f && c[2] == 0.0f);
    }

    inline bool is_zero(const T_vecmath tolerance) const {
        return (vecmath_zero(c[0], tolerance) &&
                vecmath_zero(c[1], tolerance) &&
                vecmath_zero(c[2], tolerance));
    }

    // printing
    friend std::ostream& operator<<(std::ostream& s,
            const vec3t<T_vecmath>& v) {
        return s << v.c[0] << "," << v.c[1] << "," << v.c[2];
    }

private:
    T_vecmath c[3];
};

//////////////////////////////////////////////////////
//CLASS vec4t                                       //
//                                                  //
//4-component vector, for homogeneous coordinates   //
//////////////////////////////////////////////////////
template <class T_vecmath>
class vec4t {
    friend class vec3t<T_vecmath>;
    friend class mtx3t<T_vecmath>;
    friend class mtx4t<T_vecmath>;
    friend class quatt<T_vecmath>;

public:
    //ctor
    inline vec4t() {};
    inline vec4t(T_vecmath x, T_vecmath y, T_vecmath z, T_vecmath w = 1.0f) {
        c[0] = x; c[1] = y; c[2] = z; c[3] = w;
    }

    inline vec4t(T_vecmath* ptr) { //copy from pointer
        c[0] = ptr[0]; c[1] = ptr[1]; c[2] = ptr[2]; c[3] = ptr[3];
    }

    inline vec4t<T_vecmath>(const vec3t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2]; c[3] = 1.0f;
    }

    inline vec4t<T_vecmath>(const vec4t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2]; c[3] = o.c[3];
    }

    //assignment
    inline void operator=(vec3t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
    }

    inline void operator=(vec4t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2]; c[3] = o.c[3];
    }

    //access
    inline T_vecmath* ptr() const {
        return (T_vecmath*)c;
    }

    inline operator T_vecmath* () const {
        return (T_vecmath*)c;
    }

    inline T_vecmath X() const {
        return c[0];
    }

    inline T_vecmath Y() const {
        return c[1];
    }

    inline T_vecmath Z() const {
        return c[2];
    }

    inline T_vecmath W() const {
        return c[3];
    }

    inline T_vecmath& operator[](int i) {
        return c[i];
    }

    inline void set(T_vecmath* ptr) {
        c[0] = ptr[0]; c[1] = ptr[1]; c[2] = ptr[2]; c[3] = ptr[3];
    }

    inline void get(T_vecmath* ptr) {
        ptr[0] = c[0]; ptr[1] = c[1]; ptr[2] = c[2]; ptr[3] = c[3];
    }

    //operations
    inline void div() { //perform division by w
        T_vecmath oo_w = 1.0f / c[3];
        c[0] *= oo_w;
        c[1] *= oo_w;
        c[2] *= oo_w;
        c[3] = 1.0f;
    };

    //addition
    inline const vec4t<T_vecmath> operator+(const vec4t& o) const {
        return vec4t<T_vecmath>(c[0] + o.c[0],
                                c[1] + o.c[1],
                                c[2] + o.c[2],
                                c[3] + o.c[3]);
    }

    inline void operator+=(const vec4t& o) {
        c[0] += o.c[0];
        c[1] += o.c[1];
        c[2] += o.c[2];
        c[3] += o.c[3];
    }

    //subtraction
    inline const vec4t<T_vecmath> operator-(const vec4t& o) const {
        return vec4t(c[0] - o.c[0],
                     c[1] - o.c[1],
                     c[2] - o.c[2],
                     c[3] - o.c[3]);
    }

    inline void operator-=(const vec4t& o) {
        c[0] -= o.c[0];
        c[1] -= o.c[1];
        c[2] -= o.c[2];
        c[3] -= o.c[3];
    }

    //scalar multiplication
    inline const vec4t<T_vecmath> operator*(const T_vecmath s) const {
        return vec4t(s * c[0], s * c[1], s * c[2], s * c[3]);
    }

    friend inline const vec4t<T_vecmath> operator*(T_vecmath s, const vec4t& v) {
        return vec4t(s * v.c[0], s * v.c[1], s * v.c[2], s * v.c[3]);
    }

    inline void operator*=(T_vecmath s) {
        c[0] *= s;
        c[1] *= s;
        c[2] *= s;
        c[4] *= s;
    }

    //dot
    // TODO: should get higher precedence than ordinary *
    inline T_vecmath operator&(const vec4t<T_vecmath>& o) const {
        return c[0] * o.c[0] + c[1] * o.c[1] + c[2] * o.c[2];
    }

    //flipping
    inline void flip() {
        c[0] = -c[0];
        c[1] = -c[1];
        c[2] = -c[2];
    }

    friend inline const vec4t<T_vecmath> flipped(const vec4t& v) {
        return vec4t<T_vecmath>(-v.c[0], -v.c[1], -v.c[2]);
    }

    inline const vec4t<T_vecmath> operator-() const {
        return vec4t<T_vecmath>(-c[0], -c[1], -c[2]);
    }

    //normalization
    inline void normalize4() {
        T_vecmath oo_l = 1.0f / vecmath_sqrt(c[0] * c[0] + c[1] * c[1] +
                c[2] * c[2] + c[3] * c[3]);
        c[0] *= oo_l;
        c[1] *= oo_l;
        c[2] *= oo_l;
        c[3] *= oo_l;
    }
    inline void normalize() {
        T_vecmath oo_l = 1.0f / vecmath_sqrt(c[0] * c[0] + c[1] * c[1] +
                c[2] * c[2]);
        c[0] *= oo_l;
        c[1] *= oo_l;
        c[2] *= oo_l;
    }

    friend inline const vec4t<T_vecmath> normalized(const vec4t& v) {
        T_vecmath oo_len = 1.0f / vecmath_sqrt(v.c[0] * v.c[0] +
                v.c[1] * v.c[1] + v.c[2] * v.c[2] + v.c[3] * v.c[3]);
        return vec4t(oo_len * v.c[0], oo_len * v.c[1], oo_len * v.c[2],
                oo_len * v.c[3]);
    }

    friend inline const vec4t<T_vecmath> normalized3(const vec4t& v) {
        T_vecmath oo_len = 1.0f / vecmath_sqrt(v.c[0] * v.c[0] +
                v.c[1] * v.c[1] + v.c[2] * v.c[2]);
        return vec4t(oo_len * v.c[0], oo_len * v.c[1], oo_len * v.c[2], v.c[3]);
    }

protected:
    T_vecmath c[4];
};

//////////////////////////////////////////////////////
//CLASS quatt                                       //
//                                                  //
//quaternion                                        //
//////////////////////////////////////////////////////
template <class T_vecmath>
class quatt {
public:
    //ctor
    inline quatt() {};
    inline quatt(T_vecmath s, const vec3t<T_vecmath>& v) {
        this->s = s;
        this->v = v;
    }

    // TODO: this may be very confusing (compare above)
    inline quatt(vec3t<T_vecmath>& u, T_vecmath angle) { //defines a roation quaternion
        angle *= 0.5f;
        s = vecmath_cos(angle);
        v = vecmath_sin(angle) * u;
    }

    inline quatt(const vec4t<T_vecmath>& v) {
        s = v.c[0];
        this->v.c[0] = v.c[1];
        this->v.c[1] = v.c[2];
        this->v.c[2] = v.c[3];
    }

    //assignment
    inline void operator=(const quatt<T_vecmath>& o) {
        s = o.s;
        v = o.v;
    }

    //access
    inline vec3t<T_vecmath> second() const { //returns second part (vector)
        return v;
    }

    inline T_vecmath first() const { //returns first part (single component)
        return s;
    }

    //operations
    //multiplication
    inline const quatt operator*(const quatt<T_vecmath>& other) {
        return quatt(s * other.s - v * other.v, s * other.v +
                other.s * v + v % other.v);
    }

    inline void operator*=(const quatt<T_vecmath>& other) {
        T_vecmath temp = s;
        s = s * other.s - v * other.v;
        v = other.v * temp + other.s * v + v % other.v;
    }

    //returns a rotation matrix
    inline mtx3t<T_vecmath> make_matrix() const;
    inline void make_matrix(mtx3t<T_vecmath>& m) const;
    inline void make_matrix(mtx4t<T_vecmath>& m) const;

    inline void rotation(vec3t<T_vecmath>& u, T_vecmath angle) {
        angle *= 0.5f;
        s = vecmath_cos(angle);
        v = vecmath_sin(angle) * u;
    }

private:
    T_vecmath s;
    vec3t<T_vecmath> v;
};

//////////////////////////////////////////////////////
//CLASS mtx3t                                       //
//                                                  //
//3x3 matrix, elements stored in row major order    //
//////////////////////////////////////////////////////
template <class T_vecmath>
class mtx3t {
    friend class vec3t<T_vecmath>;
    friend class vec4t<T_vecmath>;
    friend class mtx4t<T_vecmath>;
    friend class quatt<T_vecmath>;

public:
    //ctor
    inline mtx3t(bool identity = false) {
        if(identity)
            this->id();
    };

    inline mtx3t(const mtx3t<T_vecmath>& o) { //row major order
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
        c[3] = o.c[3]; c[4] = o.c[4]; c[5] = o.c[5];
        c[6] = o.c[6]; c[7] = o.c[7]; c[8] = o.c[8];
    };

    inline mtx3t(const mtx4t<T_vecmath>& o);

    inline mtx3t(const quatt<T_vecmath>& q) {
        q.make_matrix(*this);
    };

    inline mtx3t(T_vecmath a00, T_vecmath a01, T_vecmath a02,
                 T_vecmath a10, T_vecmath a11, T_vecmath a12,
                 T_vecmath a20, T_vecmath a21, T_vecmath a22) {
        c[0] = a00;   c[3] = a01;   c[6] = a02;
        c[1] = a10;   c[4] = a11;   c[7] = a12;
        c[2] = a20;   c[5] = a21;   c[8] = a22;
    }

    //assignment
    inline void operator=(mtx3t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
        c[3] = o.c[3]; c[4] = o.c[4]; c[5] = o.c[5];
        c[6] = o.c[6]; c[7] = o.c[7]; c[8] = o.c[8];
    }

    inline void operator=(mtx4t<T_vecmath>& o) {
        c[0] = o.c[0]; c[3] = o.c[4]; c[7] = o.c[8];
        c[1] = o.c[1]; c[4] = o.c[5]; c[8] = o.c[9];
        c[2] = o.c[2]; c[5] = o.c[6]; c[9] = o.c[10];
    }

    //access
    inline T_vecmath* ptr() const {
        return (T_vecmath*)c;
    }

    inline T_vecmath& operator()(int i, int j) { //(row,col); 0-based indices
        return c[i + j + (j << 1)];
    }

    inline operator T_vecmath* () const {
        return (T_vecmath*)c;
    }

    //operations
    inline void id() {
        c[0] = 1.0f; c[1] = 0.0f; c[2] = 0.0f;
        c[3] = 0.0f; c[4] = 1.0f; c[5] = 0.0f;
        c[6] = 0.0f; c[7] = 0.0f; c[8] = 1.0f;
    }

    inline const mtx3t<T_vecmath> operator*(const mtx3t<T_vecmath>& o) const {
        mtx3t m;

        m.c[0] = c[0] * o.c[0] + c[3] * o.c[1] + c[6] * o.c[2];
        m.c[1] = c[1] * o.c[0] + c[4] * o.c[1] + c[7] * o.c[2];
        m.c[2] = c[2] * o.c[0] + c[5] * o.c[1] + c[8] * o.c[2];

        m.c[3] = c[0] * o.c[3] + c[3] * o.c[4] + c[6] * o.c[5];
        m.c[4] = c[1] * o.c[3] + c[4] * o.c[4] + c[7] * o.c[5];
        m.c[5] = c[2] * o.c[3] + c[5] * o.c[4] + c[8] * o.c[5];

        m.c[6] = c[0] * o.c[6] + c[3] * o.c[7] + c[6] * o.c[8];
        m.c[7] = c[1] * o.c[6] + c[4] * o.c[7] + c[7] * o.c[8];
        m.c[8] = c[2] * o.c[6] + c[5] * o.c[7] + c[8] * o.c[8];

        return m;
    }

    inline void operator*=(const mtx3t<T_vecmath>& o) {
        T_vecmath temp[9];

        temp[0] = c[0] * o.c[0] + c[3] * o.c[1] + c[6] * o.c[2];
        temp[1] = c[1] * o.c[0] + c[4] * o.c[1] + c[7] * o.c[2];
        temp[2] = c[2] * o.c[0] + c[5] * o.c[1] + c[8] * o.c[2];

        temp[3] = c[0] * o.c[3] + c[3] * o.c[4] + c[6] * o.c[5];
        temp[4] = c[1] * o.c[3] + c[4] * o.c[4] + c[7] * o.c[5];
        temp[5] = c[2] * o.c[3] + c[5] * o.c[4] + c[8] * o.c[5];

        temp[6] = c[0] * o.c[6] + c[3] * o.c[7] + c[6] * o.c[8];
        temp[7] = c[1] * o.c[6] + c[4] * o.c[7] + c[7] * o.c[8];
        temp[8] = c[2] * o.c[6] + c[5] * o.c[7] + c[8] * o.c[8];

        c[0] = temp[0]; c[1] = temp[1]; c[2] = temp[2];
        c[3] = temp[3]; c[4] = temp[4]; c[5] = temp[5];
        c[6] = temp[6]; c[7] = temp[7]; c[8] = temp[8];
    }

    inline vec3t<T_vecmath> operator*(const vec3t<T_vecmath>& v) {
        return vec3t<T_vecmath>(c[0] * v.c[0] + c[3] * v.c[1] + c[6] * v.c[2],
                                c[1] * v.c[0] + c[4] * v.c[1] + c[7] * v.c[2],
                                c[2] * v.c[0] + c[5] * v.c[1] + c[8] * v.c[2]);
    }

    inline vec4t<T_vecmath> operator*(const vec4t<T_vecmath>& v) {
        return vec4t<T_vecmath>(c[0] * v.c[0] + c[3] * v.c[1] + c[6] * v.c[2],
                                c[1] * v.c[0] + c[4] * v.c[1] + c[7] * v.c[2],
                                c[2] * v.c[0] + c[5] * v.c[1] + c[8] * v.c[2]);
    }

    inline void operator*=(T_vecmath s) {
        c[0] *= s;    c[3] *= s;    c[6] *= s;
        c[1] *= s;    c[4] *= s;    c[7] *= s;
        c[2] *= s;    c[5] *= s;    c[8] *= s;
    }

    friend inline mtx3t<T_vecmath> transposed(const mtx3t<T_vecmath>& m) {
        mtx3t<T_vecmath> n;

        n.c[0] = m.c[0]; n.c[3] = m.c[1]; n.c[6] = m.c[2];
        n.c[1] = m.c[3]; n.c[4] = m.c[4]; n.c[7] = m.c[5];
        n.c[2] = m.c[6]; n.c[5] = m.c[7]; n.c[8] = m.c[8];

        return n;
    }

    inline void operator+=(const mtx3t<T_vecmath>& o) {
        c[0] += o.c[0];   c[1] += o.c[1];   c[2] += o.c[2];
        c[3] += o.c[3];   c[4] += o.c[4];   c[5] += o.c[5];
        c[6] += o.c[6];   c[7] += o.c[7];   c[8] += o.c[8];
    }

    friend inline mtx3t<T_vecmath> operator*(T_vecmath s,
            const mtx3t<T_vecmath>& m) {
        mtx3t n;

        n.c[0] = s * m.c[0];    n.c[1] = s * m.c[1];    n.c[2] = s * m.c[2];
        n.c[3] = s * m.c[3];    n.c[4] = s * m.c[4];    n.c[5] = s * m.c[5];
        n.c[6] = s * m.c[6];    n.c[7] = s * m.c[7];    n.c[8] = s * m.c[8];

        return n;
    }

    inline void scale(T_vecmath sx, T_vecmath sy, T_vecmath sz) {
        c[0] *= sx;
        c[4] *= sy;
        c[8] *= sz;
    }

    inline void rotateX(T_vecmath angle) {
        mtx3t<T_vecmath> m;
        T_vecmath s = vecmath_sin(angle);
        T_vecmath c = vecmath_cos(angle);
        m.c[0] = 1.0f; m.c[3] = 0.0f; m.c[6] = 0.0f;
        m.c[1] = 0.0f; m.c[4] = c;    m.c[7] = -s;
        m.c[2] = 0.0f; m.c[5] = s;    m.c[8] = c;

        (*this) *= m;
    }

    inline void rotateY(T_vecmath angle) {
        mtx3t<T_vecmath> m;
        T_vecmath s = vecmath_sin(angle);
        T_vecmath c = vecmath_cos(angle);
        m.c[0] = c;    m.c[3] = 0.0f; m.c[6] = s;
        m.c[1] = 0.0f; m.c[4] = 1.0f; m.c[7] = 0.0f;
        m.c[2] = -s;   m.c[5] = 0.0f; m.c[8] = c;

        (*this) *= m;
    }

    inline void rotateZ(T_vecmath angle) {
        mtx3t<T_vecmath> m;
        T_vecmath s = vecmath_sin(angle);
        T_vecmath c = vecmath_cos(angle);
        m.c[0] = c;    m.c[3] = -s;   m.c[6] = 0.0f;
        m.c[1] = s;    m.c[4] = c;    m.c[7] = 0.0f;
        m.c[2] = 0.0f; m.c[5] = 0.0f; m.c[8] = 1.0f;

        (*this) *= m;
    }

    inline T_vecmath determinant() {
        mtx3t<T_vecmath>& m = *this;

        T_vecmath det = m(0, 0) * m.subdeterminant(0, 0) - m(0, 1) *
            m.subdeterminant(0, 1) + m(0, 2) * m.subdeterminant(0, 2);

        return det;
    }

    inline T_vecmath subdeterminant(int i, int j) { //TODO: more efficient
        T_vecmath sub[2][2];
        mtx3t<T_vecmath>& t = *this;
        int y = 0;

        for(int n = 0; n < 3; n++)
            if(n != j) {
                int x = 0;

                for(int m = 0; m < 3; m++)
                    if(m != i) {
                        sub[x][y] = t(m, n);
                        x++;
                    }

                y++;
            }

        return sub[0][0] * sub[1][1] - sub[0][1] * sub[1][0];
    }

    // TODO: better by solving set of linear equations (numerically more stable)
    inline friend mtx3t<T_vecmath> inversed(mtx3t<T_vecmath>& o) {
        mtx3t<T_vecmath> m;

        m(0, 0) = o.subdeterminant(0, 0);
        m(0, 1) = -o.subdeterminant(1, 0);
        m(0, 2) = o.subdeterminant(2, 0);
        m(1, 0) = -o.subdeterminant(0, 1);
        m(1, 1) = o.subdeterminant(1, 1);
        m(1, 2) = -o.subdeterminant(2, 1);
        m(2, 0) = o.subdeterminant(0, 2);
        m(2, 1) = -o.subdeterminant(1, 2);
        m(2, 2) = o.subdeterminant(2, 2);

        m *= 1.0f / o.determinant();

        return m;
    }

    inline void transpose() {
        T_vecmath temp;
        temp = c[3]; c[3] = c[1]; c[1] = temp;
        temp = c[2]; c[2] = c[6]; c[6] = temp;
        temp = c[5]; c[5] = c[7]; c[7] = temp;
    }

    //misc
    inline void column_fill(vec3t<T_vecmath>* cols) {
        c[0] = cols[0][0]; c[3] = cols[1][0]; c[6] = cols[2][0];
        c[1] = cols[0][1]; c[4] = cols[1][1]; c[7] = cols[2][1];
        c[2] = cols[0][2]; c[5] = cols[1][2]; c[8] = cols[2][2];
    }

    inline void row_fill(vec3t<T_vecmath>* rows) {
        c[0] = rows[0][0]; c[3] = rows[0][1]; c[6] = rows[0][2];
        c[1] = rows[1][0]; c[4] = rows[1][1]; c[7] = rows[1][2];
        c[2] = rows[2][0]; c[5] = rows[2][1]; c[8] = rows[2][2];
    }

    // TODO: throw these ones out
    inline void opengl_to_globillum() {
        c[0] = 0.0f; c[3] = 0.0f; c[6] = 1.0f;
        c[1] = 1.0f; c[4] = 0.0f; c[7] = 0.0f;
        c[2] = 0.0f; c[5] = 1.0f; c[8] = 0.0f;
    }

    inline void globillum_to_opengl() {
        c[0] = 0.0f; c[3] = 1.0f; c[6] = 0.0f;
        c[1] = 0.0f; c[4] = 0.0f; c[7] = 1.0f;
        c[2] = 1.0f; c[5] = 0.0f; c[8] = 0.0f;
    }

protected:
    T_vecmath c[9];
};

//////////////////////////////////////////////////////
//CLASS mtx4t                                       //
//                                                  //
//4x4 matrix, elements stored in row major order    //
//////////////////////////////////////////////////////
template <class T_vecmath>
class mtx4t {
    friend class vec3t<T_vecmath>;
    friend class vec4t<T_vecmath>;
    friend class mtx3t<T_vecmath>;
    friend class quatt<T_vecmath>;

public:
    //ctor
    inline mtx4t(bool identity = false) {
        if(identity)
            this->id();
    };
    inline mtx4t(T_vecmath a, T_vecmath b, T_vecmath c, T_vecmath d,
                 T_vecmath e, T_vecmath f, T_vecmath g, T_vecmath h,
                 T_vecmath i, T_vecmath j, T_vecmath k, T_vecmath l,
                 T_vecmath m, T_vecmath n, T_vecmath o, T_vecmath p) {
        this->c[0] = a;   this->c[4] = b;   this->c[8] = c;   this->c[12] = d;
        this->c[1] = e;   this->c[5] = f;   this->c[9] = g;   this->c[13] = h;
        this->c[2] = i;   this->c[6] = j;   this->c[10] = k;  this->c[14] = l;
        this->c[3] = m;   this->c[7] = n;   this->c[11] = o;  this->c[15] = p;
    }

    inline mtx4t(const mtx3t<T_vecmath>& o) {
        c[0] = o.c[0];
        c[1] = o.c[1];
        c[2] = o.c[2];
        c[3] = (T_vecmath)0.0;
        c[4] = o.c[3];
        c[5] = o.c[4];
        c[6] = o.c[5];
        c[7] = (T_vecmath)0.0;
        c[8] = o.c[6];
        c[9] = o.c[7];
        c[10] = o.c[8];
        c[11] = (T_vecmath)0.0;
        c[12] = (T_vecmath)0.0;
        c[13] = (T_vecmath)0.0;
        c[14] = (T_vecmath)0.0;
        c[15] = (T_vecmath)1.0;
    }

    template <class T_vecmath_other> inline mtx4t(mtx4t<T_vecmath_other>& o) {
        c[0] = (T_vecmath)o.get(0, 0);
        c[4] = (T_vecmath)o.get(0, 1);
        c[8] = (T_vecmath)o.get(0, 2);
        c[12] = (T_vecmath)o.get(0, 3);
        c[1] = (T_vecmath)o.get(1, 0);
        c[5] = (T_vecmath)o.get(1, 1);
        c[9] = (T_vecmath)o.get(1, 2);
        c[13] = (T_vecmath)o.get(1, 3);
        c[2] = (T_vecmath)o.get(2, 0);
        c[6] = (T_vecmath)o.get(2, 1);
        c[10] = (T_vecmath)o.get(2, 2);
        c[14] = (T_vecmath)o.get(2, 3);
        c[3] = (T_vecmath)o.get(3, 0);
        c[7] = (T_vecmath)o.get(3, 1);
        c[11] = (T_vecmath)o.get(3, 2);
        c[15] = (T_vecmath)o.get(3, 3);
    }

    inline mtx4t(T_vecmath* ptr) {
        c[0] = ptr[0];
        c[1] = ptr[1];
        c[2] = ptr[2];
        c[3] = ptr[3];
        c[4] = ptr[4];
        c[5] = ptr[5];
        c[6] = ptr[6];
        c[7] = ptr[7];
        c[8] = ptr[8];
        c[9] = ptr[9];
        c[10] = ptr[10];
        c[11] = ptr[11];
        c[12] = ptr[12];
        c[13] = ptr[13];
        c[14] = ptr[14];
        c[15] = ptr[15];
    }

    inline mtx4t(T_vecmath array[4][4]) {
        c[0] = array[0][0];
        c[4] = array[1][0];
        c[8] = array[2][0];
        c[12] = array[3][0];
        c[1] = array[0][1];
        c[5] = array[1][1];
        c[9] = array[2][1];
        c[13] = array[3][1];
        c[2] = array[0][2];
        c[6] = array[1][2];
        c[10] = array[2][2];
        c[14] = array[3][2];
        c[3] = array[0][3];
        c[7] = array[1][3];
        c[11] = array[2][3];
        c[15] = array[3][3];
    }

    inline mtx4t(const quatt<T_vecmath>& q) {
        q.make_matrix(*this);
    }

    inline void operator=(const mtx4t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2]; c[3] = o.c[3];
        c[4] = o.c[4]; c[5] = o.c[5]; c[6] = o.c[6]; c[7] = o.c[7];
        c[8] = o.c[8]; c[9] = o.c[9]; c[10] = o.c[10]; c[11] = o.c[11];
        c[12] = o.c[12]; c[13] = o.c[13]; c[14] = o.c[14]; c[15] = o.c[15];
    }

    inline void operator=(const mtx3t<T_vecmath>& o) {
        c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2]; c[3] = 0.0f;
        c[4] = o.c[3]; c[5] = o.c[4]; c[6] = o.c[5]; c[7] = 0.0f;
        c[8] = o.c[6]; c[9] = o.c[7]; c[10] = o.c[8]; c[11] = 0.0f;
        c[12] = 0.0f; c[13] = 0.0f; c[14] = 0.0f; c[15] = 1.0f;
    }

    //access
    inline T_vecmath* ptr() const {
        return (T_vecmath*)c;
    }

    inline T_vecmath& operator()(int i, int j) { //(row,col); 0-based indices
        return c[i + (j << 2)];
        //return c[j+(i<<2)];
    }

    inline T_vecmath get(int i, int j) {
        return c[i + (j << 2)];
    }

    inline operator T_vecmath* () const {
        return (T_vecmath*)c;
    }

    inline vec4t<T_vecmath> get_column(const int i) const {
        return vec4t<T_vecmath>(c + (i << 2));
    }

    inline vec4t<T_vecmath> get_row(const int i) const {
        return vec4t<T_vecmath>(c[i], c[i + 4], c[i + 8], c[i + 12]);
    }

    //operations
    //addition
    inline void operator+=(const mtx4t<T_vecmath>& o) {
        c[0] += o.c[0];   c[4] += o.c[4];   c[8] += o.c[8];   c[12] += o.c[12];
        c[1] += o.c[1];   c[5] += o.c[5];   c[9] += o.c[9];   c[13] += o.c[13];
        c[2] += o.c[2];   c[6] += o.c[6];   c[10] += o.c[10]; c[14] += o.c[14];
        c[3] += o.c[3];   c[7] += o.c[7];   c[11] += o.c[11]; c[15] += o.c[15];
    }

    //multiplication
    inline const mtx4t<T_vecmath> operator*(const mtx4t<T_vecmath>& o) const {
        mtx4t<T_vecmath> m;

        m.c[0] = c[0] * o.c[0] + c[4] * o.c[1] + c[8] * o.c[2] + c[12] * o.c[3];
        m.c[1] = c[1] * o.c[0] + c[5] * o.c[1] + c[9] * o.c[2] + c[13] * o.c[3];
        m.c[2] = c[2] * o.c[0] + c[6] * o.c[1] + c[10] * o.c[2] + c[14] *
            o.c[3];
        m.c[3] = c[3] * o.c[0] + c[7] * o.c[1] + c[11] * o.c[2] + c[15] *
            o.c[3];

        m.c[4] = c[0] * o.c[4] + c[4] * o.c[5] + c[8] * o.c[6] + c[12] * o.c[7];
        m.c[5] = c[1] * o.c[4] + c[5] * o.c[5] + c[9] * o.c[6] + c[13] * o.c[7];
        m.c[6] = c[2] * o.c[4] + c[6] * o.c[5] + c[10] * o.c[6] + c[14] *
            o.c[7];
        m.c[7] = c[3] * o.c[4] + c[7] * o.c[5] + c[11] * o.c[6] + c[15] *
            o.c[7];

        m.c[8] = c[0] * o.c[8] + c[4] * o.c[9] + c[8] * o.c[10] + c[12] *
            o.c[11];
        m.c[9] = c[1] * o.c[8] + c[5] * o.c[9] + c[9] * o.c[10] + c[13] *
            o.c[11];
        m.c[10] = c[2] * o.c[8] + c[6] * o.c[9] + c[10] * o.c[10] + c[14] *
            o.c[11];
        m.c[11] = c[3] * o.c[8] + c[7] * o.c[9] + c[11] * o.c[10] + c[15] *
            o.c[11];

        m.c[12] = c[0] * o.c[12] + c[4] * o.c[13] + c[8] * o.c[14] + c[12] *
            o.c[15];
        m.c[13] = c[1] * o.c[12] + c[5] * o.c[13] + c[9] * o.c[14] + c[13] *
            o.c[15];
        m.c[14] = c[2] * o.c[12] + c[6] * o.c[13] + c[10] * o.c[14] + c[14] *
            o.c[15];
        m.c[15] = c[3] * o.c[12] + c[7] * o.c[13] + c[11] * o.c[14] + c[15] *
            o.c[15];

        return m;
    }

    inline void operator*=(const mtx4t<T_vecmath>& o) {
        T_vecmath temp[16];

        temp[0] = c[0] * o.c[0] + c[4] * o.c[1] + c[8] * o.c[2] + c[12] *
            o.c[3];
        temp[1] = c[1] * o.c[0] + c[5] * o.c[1] + c[9] * o.c[2] + c[13] *
            o.c[3];
        temp[2] = c[2] * o.c[0] + c[6] * o.c[1] + c[10] * o.c[2] + c[14] *
            o.c[3];
        temp[3] = c[3] * o.c[0] + c[7] * o.c[1] + c[11] * o.c[2] + c[15] *
            o.c[3];

        temp[4] = c[0] * o.c[4] + c[4] * o.c[5] + c[8] * o.c[6] + c[12] *
            o.c[7];
        temp[5] = c[1] * o.c[4] + c[5] * o.c[5] + c[9] * o.c[6] + c[13] *
            o.c[7];
        temp[6] = c[2] * o.c[4] + c[6] * o.c[5] + c[10] * o.c[6] + c[14] *
            o.c[7];
        temp[7] = c[3] * o.c[4] + c[7] * o.c[5] + c[11] * o.c[6] + c[15] *
            o.c[7];

        temp[8] = c[0] * o.c[8] + c[4] * o.c[9] + c[8] * o.c[10] + c[12] *
            o.c[11];
        temp[9] = c[1] * o.c[8] + c[5] * o.c[9] + c[9] * o.c[10] + c[13] *
            o.c[11];
        temp[10] = c[2] * o.c[8] + c[6] * o.c[9] + c[10] * o.c[10] + c[14] *
            o.c[11];
        temp[11] = c[3] * o.c[8] + c[7] * o.c[9] + c[11] * o.c[10] + c[15] *
            o.c[11];

        temp[12] = c[0] * o.c[12] + c[4] * o.c[13] + c[8] * o.c[14] + c[12] *
            o.c[15];
        temp[13] = c[1] * o.c[12] + c[5] * o.c[13] + c[9] * o.c[14] + c[13] *
            o.c[15];
        temp[14] = c[2] * o.c[12] + c[6] * o.c[13] + c[10] * o.c[14] + c[14] *
            o.c[15];
        temp[15] = c[3] * o.c[12] + c[7] * o.c[13] + c[11] * o.c[14] + c[15] *
            o.c[15];

        c[0] = temp[0]; c[1] = temp[1]; c[2] = temp[2]; c[3] = temp[3];
        c[4] = temp[4]; c[5] = temp[5]; c[6] = temp[6]; c[7] = temp[7];
        c[8] = temp[8]; c[9] = temp[9]; c[10] = temp[10]; c[11] = temp[11];
        c[12] = temp[12]; c[13] = temp[13]; c[14] = temp[14]; c[15] = temp[15];
    }

    inline void operator*=(const mtx3t<T_vecmath>& o) {
        T_vecmath temp[16];

        temp[0] = c[0] * o.c[0] + c[4] * o.c[1] + c[8] * o.c[2];
        temp[1] = c[1] * o.c[0] + c[5] * o.c[1] + c[9] * o.c[2];
        temp[2] = c[2] * o.c[0] + c[6] * o.c[1] + c[10] * o.c[2];
        temp[3] = c[3] * o.c[0] + c[7] * o.c[1] + c[11] * o.c[2];

        temp[4] = c[0] * o.c[3] + c[4] * o.c[4] + c[8] * o.c[5];
        temp[5] = c[1] * o.c[3] + c[5] * o.c[4] + c[9] * o.c[5];
        temp[6] = c[2] * o.c[3] + c[6] * o.c[4] + c[10] * o.c[5];
        temp[7] = c[3] * o.c[3] + c[7] * o.c[4] + c[11] * o.c[5];

        temp[8] = c[0] * o.c[6] + c[4] * o.c[7] + c[8] * o.c[8];
        temp[9] = c[1] * o.c[6] + c[5] * o.c[7] + c[9] * o.c[8];
        temp[10] = c[2] * o.c[6] + c[6] * o.c[7] + c[10] * o.c[8];
        temp[11] = c[3] * o.c[6] + c[7] * o.c[7] + c[11] * o.c[8];

        temp[12] = c[12];
        temp[13] = c[13];
        temp[14] = c[14];
        temp[15] = c[15];

        c[0] = temp[0]; c[1] = temp[1]; c[2] = temp[2]; c[3] = temp[3];
        c[4] = temp[4]; c[5] = temp[5]; c[6] = temp[6]; c[7] = temp[7];
        c[8] = temp[8]; c[9] = temp[9]; c[10] = temp[10]; c[11] = temp[11];
        c[12] = temp[12]; c[13] = temp[13]; c[14] = temp[14]; c[15] = temp[15];
    }

    inline const vec4t<T_vecmath> operator*(const vec3t<T_vecmath>& v) const {
        return vec4t<T_vecmath>(c[0] * v.c[0] + c[4] * v.c[1] + c[8] * v.c[2] +
                c[12], c[1] * v.c[0] + c[5] * v.c[1] + c[9] * v.c[2] + c[13],
                c[2] * v.c[0] + c[6] * v.c[1] + c[10] * v.c[2] + c[14], c[3] *
                v.c[0] + c[7] * v.c[1] + c[11] * v.c[2] + c[15]);
    }

    inline const vec4t<T_vecmath> operator*(const vec4t<T_vecmath>& v) const {
        return vec4t<T_vecmath>(c[0] * v.c[0] + c[4] * v.c[1] + c[8] * v.c[2] +
                c[12] * v.c[3], c[1] * v.c[0] + c[5] * v.c[1] + c[9] * v.c[2] +
                c[13] * v.c[3], c[2] * v.c[0] + c[6] * v.c[1] + c[10] * v.c[2] +
                c[14] * v.c[3], c[3] * v.c[0] + c[7] * v.c[1] + c[11] * v.c[2] +
                c[15] * v.c[3]);
    }

    friend inline const mtx4t<T_vecmath> operator*(T_vecmath s,
            const mtx4t<T_vecmath>& m) {
        mtx4t n;

        n.c[0] = s * m.c[0];    n.c[4] = s * m.c[4];    n.c[8] = s * m.c[8];
        n.c[12] = s * m.c[12];
        n.c[1] = s * m.c[1];    n.c[5] = s * m.c[5];    n.c[9] = s * m.c[9];
        n.c[13] = s * m.c[13];
        n.c[2] = s * m.c[2];    n.c[6] = s * m.c[6];    n.c[10] = s * m.c[10];
        n.c[14] = s * m.c[14];
        n.c[3] = s * m.c[3];    n.c[7] = s * m.c[7];    n.c[11] = s * m.c[11];
        n.c[15] = s * m.c[15];

        return n;
    }

    inline void operator*=(T_vecmath s) {
        c[0] *= s;    c[4] *= s;    c[8] *= s;    c[12] *= s;
        c[1] *= s;    c[5] *= s;    c[9] *= s;    c[13] *= s;
        c[2] *= s;    c[6] *= s;    c[10] *= s;   c[14] *= s;
        c[3] *= s;    c[7] *= s;    c[11] *= s;   c[15] *= s;
    }

    inline void id() {
        c[0] = 1.0f; c[4] = 0.0f; c[8] = 0.0f; c[12] = 0.0f;
        c[1] = 0.0f; c[5] = 1.0f; c[9] = 0.0f; c[13] = 0.0f;
        c[2] = 0.0f; c[6] = 0.0f; c[10] = 1.0f; c[14] = 0.0f;
        c[3] = 0.0f; c[7] = 0.0f; c[11] = 0.0f; c[15] = 1.0f;
    }

    //transformations
    inline void translate(const vec3t<T_vecmath>& v) {
        mtx4t<T_vecmath> m;
        m.c[0] = 1.0f; m.c[4] = 0.0f; m.c[8] = 0.0f; m.c[12] = v.c[0];
        m.c[1] = 0.0f; m.c[5] = 1.0f; m.c[9] = 0.0f; m.c[13] = v.c[1];
        m.c[2] = 0.0f; m.c[6] = 0.0f; m.c[10] = 1.0f; m.c[14] = v.c[2];
        m.c[3] = 0.0f; m.c[7] = 0.0f; m.c[11] = 0.0f; m.c[15] = 1.0f;

        (*this) *= m;
    }

    inline void scale(T_vecmath sx, T_vecmath sy, T_vecmath sz) {
        mtx4t<T_vecmath> m;
        m.c[0] = sx;      m.c[4] = 0.0f;    m.c[8] = 0.0f;    m.c[12] = 0.0f;
        m.c[1] = 0.0f;    m.c[5] = sy;      m.c[9] = 0.0f;    m.c[13] = 0.0f;
        m.c[2] = 0.0f;    m.c[6] = 0.0f;    m.c[10] = sz;     m.c[14] = 0.0f;
        m.c[3] = 0.0f;    m.c[7] = 0.0f;    m.c[11] = 0.0f;   m.c[15] = 1.0f;

        (*this) *= m;
    }

    inline void rotate(quatt<T_vecmath>& q) {
        mtx4t<T_vecmath> m(q.make_matrix());

        (*this) *= m;
    }

    inline void rotateX(T_vecmath angle) {
        mtx4t<T_vecmath> m;
        T_vecmath s = vecmath_sin(angle);
        T_vecmath c = vecmath_cos(angle);
        m.c[0] = 1.0f; m.c[4] = 0.0f; m.c[8] = 0.0f;  m.c[12] = 0.0f;
        m.c[1] = 0.0f; m.c[5] = c;    m.c[9] = -s;    m.c[13] = 0.0f;
        m.c[2] = 0.0f; m.c[6] = s;    m.c[10] = c;    m.c[14] = 0.0f;
        m.c[3] = 0.0f; m.c[7] = 0.0f; m.c[11] = 0.0f; m.c[15] = 1.0f;

        (*this) *= m;
    }

    inline void rotateY(T_vecmath angle) {
        mtx4t<T_vecmath> m;
        T_vecmath s = vecmath_sin(angle);
        T_vecmath c = vecmath_cos(angle);
        m.c[0] = c;    m.c[4] = 0.0f; m.c[8] = s;     m.c[12] = 0.0f;
        m.c[1] = 0.0f; m.c[5] = 1.0f; m.c[9] = 0.0f;  m.c[13] = 0.0f;
        m.c[2] = -s;   m.c[6] = 0.0f; m.c[10] = c;    m.c[14] = 0.0f;
        m.c[3] = 0.0f; m.c[7] = 0.0f; m.c[11] = 0.0f; m.c[15] = 1.0f;

        (*this) *= m;
    }

    inline void rotateZ(T_vecmath angle) {
        mtx4t<T_vecmath> m;
        T_vecmath s = vecmath_sin(angle);
        T_vecmath c = vecmath_cos(angle);
        m.c[0] = c;    m.c[4] = -s;   m.c[8] = 0.0f;  m.c[12] = 0.0f;
        m.c[1] = s;    m.c[5] = c;    m.c[9] = 0.0f;  m.c[13] = 0.0f;
        m.c[2] = 0.0f; m.c[6] = 0.0f; m.c[10] = 1.0f; m.c[14] = 0.0f;
        m.c[3] = 0.0f; m.c[7] = 0.0f; m.c[11] = 0.0f; m.c[15] = 1.0f;

        (*this) *= m;
    }

    inline void look(const vec3t<T_vecmath>& pos, const vec3t<T_vecmath>& dir,
            const vec3t<T_vecmath>& up) {
        vec3t<T_vecmath> x = normalized(dir ^ up);
        vec3t<T_vecmath> y = x ^ dir;
        vec3t<T_vecmath> z = -dir;
        mtx4t<T_vecmath> m;
        m.c[0] = x[0];    m.c[4] = x[1];    m.c[8] = x[2];    m.c[12] = 0.0f;
        m.c[1] = y[0];    m.c[5] = y[1];    m.c[9] = y[2];    m.c[13] = 0.0f;
        m.c[2] = z[0];    m.c[6] = z[1];    m.c[10] = z[2];   m.c[14] = 0.0f;
        m.c[3] = 0.0f;    m.c[7] = 0.0f;    m.c[11] = 0.0f;   m.c[15] = 1.0f;
        (*this) *= m;
        this->translate(-pos);
    }

    inline void inverse_look(const vec3t<T_vecmath>& pos,
            const vec3t<T_vecmath>& dir, const vec3t<T_vecmath>& up) {
        vec3t<T_vecmath> x = normalized(dir ^ up);
        vec3t<T_vecmath> y = x ^ dir;
        vec3t<T_vecmath> z = -dir;
        mtx4t<T_vecmath> m;
        m.c[0] = x[0];    m.c[4] = y[0];    m.c[8] = z[0];    m.c[12] = 0.0f;
        m.c[1] = x[1];    m.c[5] = y[1];    m.c[9] = z[1];    m.c[13] = 0.0f;
        m.c[2] = x[2];    m.c[6] = y[2];    m.c[10] = z[2];   m.c[14] = 0.0f;
        m.c[3] = 0.0f;    m.c[7] = 0.0f;    m.c[11] = 0.0f;   m.c[15] = 1.0f;
        this->translate(pos);
        (*this) *= m;
    }

    inline void frustum(T_vecmath l, T_vecmath r, T_vecmath b, T_vecmath t,
            T_vecmath n, T_vecmath f) {
        mtx4t<T_vecmath> m;
        m.c[0] = 2.0f * n / (r - l);    m.c[4] = 0.0f;
        m.c[8] = (r + l) / (r - l);     m.c[12] = 0.0f;
        m.c[1] = 0.0f;            m.c[5] = 2.0f * n / (t - b);
        m.c[9] = (t + b) / (t - b);     m.c[13] = 0.0f;
        m.c[2] = 0.0f;            m.c[6] = 0.0f;
        m.c[10] = -(f + n) / (f - n);   m.c[14] = -2.0f * f * n / (f - n);
        m.c[3] = 0.0f;            m.c[7] = 0.0f;
        m.c[11] = -1.0f;          m.c[15] = 0.0f;
        (*this) *= m;
    }

    inline void inverse_frustum(T_vecmath l, T_vecmath r, T_vecmath b,
            T_vecmath t, T_vecmath n, T_vecmath f) {
        mtx4t m;
        m.c[0] = (r - l) / (2.0f * n);      m.c[4] = 0.0f;
        m.c[8] = 0.0f;                m.c[12] = (r + l) / (2.0f * n);
        m.c[1] = 0.0f;                m.c[5] = (t - b) / (2.0f * n);
        m.c[9] = 0.0f;                m.c[13] = (t + b) / (2.0f * n);
        m.c[2] = 0.0f;                m.c[6] = 0.0f;
        m.c[10] = 0.0f;               m.c[14] = -1.0f;
        m.c[3] = 0.0f;                m.c[7] = 0.0f;
        m.c[11] = (n - f) / (2.0f * f * n);
        m.c[15] = (f + n) / (2.0f * f * n);
        (*this) *= m;
    }

    inline void transpose() {
        T_vecmath temp;
        temp = c[1]; c[1] = c[4]; c[4] = temp;
        temp = c[2]; c[2] = c[8]; c[8] = temp;
        temp = c[3]; c[3] = c[12]; c[12] = temp;
        temp = c[6]; c[6] = c[9]; c[9] = temp;
        temp = c[7]; c[7] = c[13]; c[13] = temp;
        temp = c[11]; c[11] = c[14]; c[14] = temp;
    }

    inline friend const mtx4t<T_vecmath> transposed(const mtx4t<T_vecmath>& o) {
        mtx4t m;
        m.c[0] = o.c[0];  m.c[4] = o.c[1];  m.c[8] = o.c[2];
        m.c[12] = o.c[3];
        m.c[1] = o.c[4];  m.c[5] = o.c[5];  m.c[9] = o.c[6];
        m.c[13] = o.c[7];
        m.c[2] = o.c[8];  m.c[6] = o.c[9];  m.c[10] = o.c[10];
        m.c[14] = o.c[11];
        m.c[3] = o.c[12]; m.c[7] = o.c[13]; m.c[11] = o.c[14];
        m.c[15] = o.c[15];
        return m;
    }

    //inverse
    inline void rigid_inverse() {
        vec4t<T_vecmath> transl(c + 12);
        c[12] = 0.0f;
        c[13] = 0.0f;
        c[14] = 0.0f;
        transpose();
        transl = -((*this) * transl);
        c[12] = transl[0];
        c[13] = transl[1];
        c[14] = transl[2];
    }


    inline T_vecmath subdeterminant(int i, int j) {
        //determinant of 3x3 "sub-matrix" which round (i,j)
        mtx3t<T_vecmath> temp;
        mtx4t<T_vecmath>& t = *this;

        int y = 0;

        for(int n = 0; n < 4; n++)
            if(n != j) {
                int x = 0;

                for(int m = 0; m < 4; m++)
                    if(m != i) {
                        temp(x, y) = t(m, n);
                        x++;
                    }

                y++;
            }

        return temp.determinant();
    }

    inline T_vecmath determinant() {
        mtx4t<T_vecmath>& m = *this;

        return  m(0, 0) * subdeterminant(0, 0)
                - m(0, 1) * subdeterminant(0, 1)
                + m(0, 2) * subdeterminant(0, 2);
    }

    inline void inverse() {
        *this = inversed(*this);
    }

    // TODO: better by solving set of linear equations (numerically more stable)
    friend inline mtx4t<T_vecmath> inversed(mtx4t<T_vecmath>& m) {
        //assumes determinant>0
        mtx4t<T_vecmath> r;

        //row 1
        r(0, 0) = m.subdeterminant(0, 0);   r(0, 1) = -m.subdeterminant(1, 0);
        r(0, 2) = m.subdeterminant(2, 0);   r(0, 3) = -m.subdeterminant(3, 0);
        //row 2
        r(1, 0) = -m.subdeterminant(0, 1);  r(1, 1) = m.subdeterminant(1, 1);
        r(1, 2) = -m.subdeterminant(2, 1);  r(1, 3) = m.subdeterminant(3, 1);
        //row 3
        r(2, 0) = m.subdeterminant(0, 2);   r(2, 1) = -m.subdeterminant(1, 2);
        r(2, 2) = m.subdeterminant(2, 2);   r(2, 3) = -m.subdeterminant(3, 2);
        //row 4
        r(3, 0) = -m.subdeterminant(0, 3);  r(3, 1) = m.subdeterminant(1, 3);
        r(3, 2) = -m.subdeterminant(2, 3);  r(3, 3) = m.subdeterminant(3, 3);

        T_vecmath oo_det = 1.0f / m.determinant();
        r *= oo_det;
        return r;
    }

protected:
    T_vecmath c[16];
};

//////////////////////////
//further implementation//
//////////////////////////

//CLASS vec3t
template <class T_vecmath> vec3t<T_vecmath>::vec3t(const vec4t<T_vecmath>& o) {
    c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
}

template <class T_vecmath> mtx3t<T_vecmath> vec3t<T_vecmath>::star() const {
    mtx3t<T_vecmath> m;
    m.c[0] = 0.0f;    m.c[3] = -c[2];   m.c[6] = c[1];
    m.c[1] = c[2];    m.c[4] = 0.0f;    m.c[7] = -c[0];
    m.c[2] = -c[1];   m.c[5] = c[0];    m.c[8] = 0.0f;
    return m;
}

template <class T_vecmath> void vec3t<T_vecmath>::operator+=(
        vec4t<T_vecmath>& o) {
    c[0] += o.c[0];
    c[1] += o.c[1];
    c[2] += o.c[2];
}

//CLASS mtx3t
template <class T_vecmath> mtx3t<T_vecmath>::mtx3t(const mtx4t<T_vecmath>& o) {
    c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2];
    c[3] = o.c[4]; c[4] = o.c[5]; c[5] = o.c[6];
    c[6] = o.c[8]; c[7] = o.c[9]; c[8] = o.c[10];
};

//CLASS quatt
template <class T_vecmath> mtx3t<T_vecmath> quatt<T_vecmath>::make_matrix()
        const {
    mtx3t<T_vecmath> m;

    m.c[0] = 1.0f - 2 * (v.Y() * v.Y() + v.Z() * v.Z());
    m.c[1] = 2 * (v.X() * v.Y() + s * v.Z());
    m.c[2] = 2 * (v.X() * v.Z() - s * v.Y());

    m.c[3] = 2 * (v.X() * v.Y() - s * v.Z());
    m.c[4] = 1.0f - 2 * (v.X() * v.X() + v.Z() * v.Z());
    m.c[5] = 2 * (v.Y() * v.Z() + s * v.X());

    m.c[6] = 2 * (v.X() * v.Z() + s * v.Y());
    m.c[7] = 2 * (v.Y() * v.Z() - s * v.X());
    m.c[8] = 1.0f - 2 * (v.X() * v.X() + v.Y() * v.Y());

    return m;
}

template <class T_vecmath> void quatt<T_vecmath>::make_matrix(
        mtx3t<T_vecmath>& m) const {
    m.c[0] = 1.0f - 2 * (v.Y() * v.Y() + v.Z() * v.Z());
    m.c[1] = 2 * (v.X() * v.Y() + s * v.Z());
    m.c[2] = 2 * (v.X() * v.Z() - s * v.Y());

    m.c[3] = 2 * (v.X() * v.Y() - s * v.Z());
    m.c[4] = 1.0f - 2 * (v.X() * v.X() + v.Z() * v.Z());
    m.c[5] = 2 * (v.Y() * v.Z() + s * v.X());

    m.c[6] = 2 * (v.X() * v.Z() + s * v.Y());
    m.c[7] = 2 * (v.Y() * v.Z() - s * v.X());
    m.c[8] = 1.0f - 2 * (v.X() * v.X() + v.Y() * v.Y());
}

template <class T_vecmath> void quatt<T_vecmath>::make_matrix(
        mtx4t<T_vecmath>& m) const {
    m.c[0] = 1.0f - 2 * (v.Y() * v.Y() + v.Z() * v.Z());
    m.c[1] = 2 * (v.X() * v.Y() + s * v.Z());
    m.c[2] = 2 * (v.X() * v.Z() - s * v.Y());
    m.c[3] = 0.0f;

    m.c[4] = 2 * (v.X() * v.Y() - s * v.Z());
    m.c[5] = 1.0f - 2 * (v.X() * v.X() + v.Z() * v.Z());
    m.c[6] = 2 * (v.Y() * v.Z() + s * v.X());
    m.c[7] = 0.0f;

    m.c[8] = 2 * (v.X() * v.Z() + s * v.Y());
    m.c[9] = 2 * (v.Y() * v.Z() - s * v.X());
    m.c[10] = 1.0f - 2 * (v.X() * v.X() + v.Y() * v.Y());
    m.c[11] = 0.0f;

    m.c[12] = 0.0f;
    m.c[13] = 0.0f;
    m.c[14] = 0.0f;
    m.c[15] = 1.0f;
}
#endif

