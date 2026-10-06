// ScreenBuffer.h: screen buffer class
//
// This Screen class contains two sets of pixel data:
// . a RGBA set, for image display (with the OpenGL glDrawPixels()
// function for instance), and
// . a HDR set (high dynamic range, the raw colors from a stochastic
// ray tracing calculation for instance)
// RGBA pixels are stored in packed format (4 bytes per pixel)
// HDR pixels are stored as 3 floats (RGB) per pixel
//
// Conversion from HDR to RGBA pixels is usually done using a tone
// mapping operator. The tonemap() member function implements such
// an operator.
//
// Member functions 'pack' and 'unpack' convert vec3 RGB triplets
// into RGBA quadruples and back. There's a pack() member function
// for converting HDR to RGBA pixels without tone mapping, by
// simply applying a scale factor.
//
// Storing/retrieving pixels can be done with the getPixel() and
// setPixel() member functions.
//
// This Screen class also contains the current view point in
// order to facilitate conversion between screen coordinates and
// world space virtual screen points or directions. The setView()
// member function is similar to gluLookAt() in OpenGL.
//
// Note: this Screen class assumes 32-bit unsigned integers.
//
// Credits: much of the inspiration for this class comes from
// Frank Suykens' ScreenBuffer class in RenderPark.
//
// Philippe.Bekaert@luc.ac.be - February 2002, May 2003
//
//
// Changelog
// 20040417 tmertens
// * added gamma correction, it happens during packing


#ifndef _SCREENBUFFER_H_
#define _SCREENBUFFER_H_

#include <string.h>
#include <float.h>
#include <math.h>

#ifndef SCRN_DONT_TEST_IF_FINITE
#include <stdio.h>
#endif

#include "vecmath.h"

class ScreenBuffer {
protected:
    bool view_initialized;  // initially false, set to true by setView()

public:
    unsigned* rgba;       // display RGBA pixels
    vec3* hdr;            // high dynamic range pixels
    int width, height;    // image width and height
    float rcpgamma;       // for gamma correction, contains 1/gamma

    // Current view, as set with setView() member function below
    vec3 eye, center, up;  // eye point, focus point and up dir.
    float fov;                   // field of view angle in radians

    // Variables derived from the above, by setView()
    // Camera frame: Z=reverse viewing dir, X=right, Y=up in screen
    vec3 X, Y, Z;
    float hsize, vsize;  // virtual screen width and height
    float h, v;          // pixel width and height (h = hsize/width)
    vec3 H, V;     // translates to next pixel (H = h*X, V = v*Y)
    vec3 O;        // 3D coord. of lower left corner of screen
    vec3 OE;       // O - eye

    // Allocates storage for RGBA and HDR pixels.
    // The image is not cleared.
    inline void init(const int w, const int h) {
        delete [] rgba;     // allocated with new[], so delete[]
        delete [] hdr;
        width = w;
        height = h;

        if(width * height > 0) {
            rgba = new unsigned [width * height];
            hdr = new vec3 [width * height];
        }

        view_initialized = false;
    }

    // Re-allocates HDR and RGBA pixel arrays and recomputes
    // viewing parameters if set. Does not clear.
    inline void resize(const int w, const int h) {
        if(w != width || h != height) {
            init(w, h);

            if(view_initialized)
                setView(eye, center, up, fov);
        }
    }

    // Constructors: image is not cleared!
    ScreenBuffer() {
        width = height = 0;
        rgba = 0;
        hdr = 0;
        view_initialized = false;
        rcpgamma = 1.f;
    }

    ScreenBuffer(const int width, const int height) {
        rgba = 0;
        hdr = 0;
        init(width, height);
        view_initialized = false;
        rcpgamma = 1.f;
    }

    inline void copy(const ScreenBuffer& src) {
        if(&src != this) {
            resize(src.width, src.height);

            if(width * height > 0) {
                memcpy(rgba, src.rgba, width * height * sizeof(unsigned));
                memcpy(hdr, src.hdr, width * height * sizeof(vec3));
            }

            view_initialized = src.view_initialized;

            if(src.view_initialized)
                setView(src.eye, src.center, src.up, src.fov);
        }
    }

    ScreenBuffer(const ScreenBuffer& src) {
        copy(src);
    }

    ScreenBuffer& operator=(const ScreenBuffer& src) {
        copy(src); return *this;
    }

    ~ScreenBuffer() {
        delete [] rgba; delete [] hdr;
    }

    // clears HDR pixels
    inline void clearHDR(void) {
        memset(hdr, 0, sizeof(vec3)*width * height);
    }

    // clears RGBA pixels
    inline void clearRGBA(void) {
        memset(rgba, 0, sizeof(unsigned)*width * height);
    }

    // clears RGBA and HDR pixels
    inline void clear(void) {
        clearRGBA();
        clearHDR();
    }

    // Set current view for image, returns false if eye and focus
    // point coincide, or up-direction and viewing direction are
    // the same or opposite.
    // fov is in RADIANS, not DEGREES
    bool setView(const vec3& eye, const vec3& center, const vec3& up,
                 const float fov);

    // Convert pixel coordinates i in [0,width-1), j in [0,height-1)
    // to 3D coordinate of corresponding point on the virtual screen.
    inline const vec3 getPoint(const float i, const float j) const {
        return (O + H * i + V * j);
    }

    // Convert pixel coordinates to direction from eye point to
    // the corresponding point on the virtual screen in 3D
    // (primary ray direction for ray tracing).
    inline const vec3 getDirection(const float i, const float j) const {
        return (OE + H * i + V * j).normalized();
    }

    // Converts primary ray direction to pixel coordinates.
    // Returns false if the direction does not point to the virtual
    // screen.
    inline bool getPixelCoord(const vec3& dir, float* i, float* j) const {
        float d = -(dir & Z);

        if(d < 1e-10)     // dir points behind the viewer or perp. screen
            return false;

        vec3 p = dir / d - OE;
        *i = (p & X) / h;
        *j = (p & Y) / v;
        return (*i >= 0. && *i < width && *j >= 0 && *j <= height);
    }

    // Returns pointer to packed RGBA color of pixel (i,j)
    inline unsigned* getPixel(const int i, const int j) const {
        return &rgba[j * width + i];
    }

    // Sets packed RGBA color of pixel (i,j)
    inline void setPixel(const int i, const int j, const unsigned c) {
        rgba[j * width + i] = c;
    }

    // Converts vec3 to packed RGBA
    inline unsigned pack(const vec3& c) const {
        union {
            unsigned char c[4];
            unsigned i;
        } cvt;
        cvt.c[0] = (unsigned char)(c[0] <0. ? 0. : c[0]> 1. ? 255. :
                powf(c[0], rcpgamma) * 255.);
        cvt.c[1] = (unsigned char)(c[1] <0. ? 0. : c[1]> 1. ? 255. :
                powf(c[1], rcpgamma) * 255.);
        cvt.c[2] = (unsigned char)(c[2] <0. ? 0. : c[2]> 1. ? 255. :
                powf(c[2], rcpgamma) * 255.);
        cvt.c[3] = 0xff;
        return cvt.i;
    }

    // packs all HDR pixels into the RGBA set, after applying optional
    // scale factor.
    void pack(float scale = 1.);

    // Converts packed RGBA to vec3 (RGB)
    inline const vec3 unpack(const unsigned rgba) const {
        union {
            unsigned char c[4];
            unsigned i;
        } cvt;
        cvt.i = rgba;
        return vec3((float)cvt.c[0] / 255., (float)cvt.c[1] / 255.,
                (float)cvt.c[2] / 255.);
    }

    // Sets RGBA color of pixel (vec3 parameter, range [0,1])
    inline void setPixel(const int i, const int j, const vec3& c) {
        setPixel(i, j, pack(c));
    }

    // Returns pointer to HDR value of pixel (i,j)
    inline vec3* getHDRPixel(const int i, const int j) const {
        return &hdr[j * width + i];
    }

    inline vec3& getHDRPixelRef(const int i, const int j) const {
        return hdr[j * width + i];
    }

    // Sets HDR value of pixel (i,j) (vec3 parameter)
    inline void setHDRPixel(const int i, const int j, const vec3& r) {
#ifndef SCRN_DONT_TEST_IF_FINITE
        if(isfinite(r[0]) && isfinite(r[1]) && isfinite(r[2]))
#endif
            hdr[j * width + i] = r;

#ifndef SCRN_DONT_TEST_IF_FINITE
        else
            fprintf(stderr, "ScreenBuffer::setHDRPixel: nan contrib to pixel "
                    "(%d,%d)\n", i, j);

#endif
    }

    // Adds HDR value to pixel (i,j)
    inline void addHDRPixel(const int i, const int j, const vec3& r) {
#ifndef SCRN_DONT_TEST_IF_FINITE
        if(isfinite(r[0]) && isfinite(r[1]) && isfinite(r[2]))
#endif
            hdr[j * width + i] += r;

#ifndef SCRN_DONT_TEST_IF_FINITE
        else
            fprintf(stderr, "ScreenBuffer::addHDRPixel: nan contrib to pixel "
                    "(%d,%d)\n", i, j);

#endif
    }

    // Adds constant term 'bias' to all HDR pixels.
    inline void biasHDR(const vec3& bias) {
        vec3* r = hdr;

        for(int i = 0; i < width * height; i++)
            *r++ += bias;
    }

    // Multiplies all HDR pixels with constant factor 'scale'
    inline void scaleHDR(const float scale) {
        vec3* r = hdr;

        for(int i = 0; i < width * height; i++)
            *r++ *= scale;
    }

    // Calculates adaptation level (kind of average
    // intensity) of HDR image for
    // tone mapping. The optional scale factor is applied
    // for computing the adaptation level only. It does
    // not alter the HDR pixel values (needed for tonemap()).
    double adaptation_level(float scale = 1.) const;

    // Tone maps single RGB triplet (needed by tonemap() below)
    const vec3 tonemap(const vec3& rad, float refwhite, float scale = 1.);

    // Derives RGBA pixel values from HDR set
    // The optional scale factor is applied for conversion only,
    // the HDR pixels remain unchanged.
    // It can be used to divide by the number of paths per pixels
    // for instance, in ray tracing (if not done in another way).
    void tonemap(float scale = 1.);

    // Save current RGBA image to file (PPM file format)
    void saveRGBAImage(const char* filename);

    //sets gamma correction
    void set_gamma(const float gamma) {
        if(gamma != 1.0f)
            fprintf(stderr, "gamma correction set to %1.1f\n", gamma);
        else
            fprintf(stderr, "gamma correction disabled\n");

        rcpgamma = 1.f / gamma;
    }

#ifdef NEVER
    // Save current HDR image to file, using specified adaptation_luminance
    // (needed by certain formats) and optional scaling.
    void saveHDRImage(const char* filename, double adaptation_luminance,
            double scale = 1.);

    // Same, but uses lwa and scale, previously set when doing tone mapping,
    // or directly.
    inline void saveHDRImage(const char* filename) {
        saveHDRImage(filename, lwa, scale);
    }
#endif
};

#endif /* _SCREENBUFFER_H_ */

