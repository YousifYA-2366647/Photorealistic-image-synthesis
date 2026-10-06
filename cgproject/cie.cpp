/* cie.c */

#include <stdio.h>
#include <math.h>
#include "cie.h"

#ifndef EPSILON
#define EPSILON 1e-6
#endif

/* This code is a modified version of the CIE XYZ<->RGB code
 * in the MGF documentation */

#ifdef  NTSC
static float  CIE_x_r =                0.670;    /* standard NTSC primaries */
static float  CIE_y_r =                0.330;
static float  CIE_x_g =                0.210;
static float  CIE_y_g =                0.710;
static float  CIE_x_b =                0.140;
static float  CIE_y_b =                0.080;
/* monitor white point */
static float  CIE_x_w =                0.3333333333;
static float  CIE_y_w =                0.3333333333;
#else
static float  CIE_x_r =                0.640;    /* nominal CRT primaries */
static float  CIE_y_r =                0.330;
static float  CIE_x_g =                0.290;
static float  CIE_y_g =                0.600;
static float  CIE_x_b =                0.150;
static float  CIE_y_b =                0.060;
/* monitor white point */
static float  CIE_x_w =                0.3333333333;
static float  CIE_y_w =                0.3333333333;
#endif

#define CIE_D           (       CIE_x_r*(CIE_y_g - CIE_y_b) + \
                                CIE_x_g*(CIE_y_b - CIE_y_r) + \
                                CIE_x_b*(CIE_y_r - CIE_y_g)     )
#define CIE_C_rD        ( (1./CIE_y_w) * \
                                ( CIE_x_w*(CIE_y_g - CIE_y_b) - \
                                  CIE_y_w*(CIE_x_g - CIE_x_b) + \
                                  CIE_x_g*CIE_y_b - CIE_x_b*CIE_y_g     ) )
#define CIE_C_gD        ( (1./CIE_y_w) * \
                                ( CIE_x_w*(CIE_y_b - CIE_y_r) - \
                                  CIE_y_w*(CIE_x_b - CIE_x_r) - \
                                  CIE_x_r*CIE_y_b + CIE_x_b*CIE_y_r     ) )
#define CIE_C_bD        ( (1./CIE_y_w) * \
                                ( CIE_x_w*(CIE_y_r - CIE_y_g) - \
                                  CIE_y_w*(CIE_x_r - CIE_x_g) + \
                                  CIE_x_r*CIE_y_g - CIE_x_g*CIE_y_r     ) )

#define CIE_rf          (CIE_y_r*CIE_C_rD/CIE_D)
#define CIE_gf          (CIE_y_g*CIE_C_gD/CIE_D)
#define CIE_bf          (CIE_y_b*CIE_C_bD/CIE_D)

#define CIE_WHITE_U (4.0*CIE_WHITE_X / \
              (CIE_WHITE_X+15.0*CIE_WHITE_Y+3.0*CIE_WHITE_Z))
#define CIE_WHITE_V (9.0*CIE_WHITE_Y / \
              (CIE_WHITE_X+15.0*CIE_WHITE_Y+3.0*CIE_WHITE_Z))

/* ---------------------------------------------------------------------------
  `_luminousEfficacy'

  Luminous efficacy currently in use.
  ------------------------------------------------------------------------- */
static float _luminousEfficacy = WHITE_EFFICACY;

/* ---------------------------------------------------------------------------
  `_xyz2rgbmat'
  `_rgb2xyzmat'

  Conversion matrices for CIEXYZ<->RGB conversions. The actual values are
  set up according to specified monitor primaries.
  ------------------------------------------------------------------------- */
static float _xyz2rgbmat[3][3], _rgb2xyzmat[3][3];

/* ---------------------------------------------------------------------------
                                                             PRIVATE FUNCTIONS
  ------------------------------------------------------------------------- */
static float _gray(const float* spec) {
    return CIE_rf * spec[0] + CIE_gf * spec[1] + CIE_bf * spec[2];
}

static float _luminance(const float* spec) {
    return _luminousEfficacy * _gray(spec);
}

static void _setcolortrans(float mat[3][3],
                           float a, float b, float c,
                           float d, float e, float f,
                           float g, float h, float i) {
    mat[0][0] = a; mat[0][1] = b; mat[0][2] = c;
    mat[1][0] = d; mat[1][1] = e; mat[1][2] = f;
    mat[2][0] = g; mat[2][1] = h; mat[2][2] = i;
}

/* ---------------------------------------------------------------------------
                                                              PUBLIC FUNCTIONS
  ----------------------------------------------------------------------------
  `GetLuminousEfficacy'
  `SetLuminousEfficacy'

  Set/return the value usef for tristimulus white efficacy.
  ------------------------------------------------------------------------- */
void GetLuminousEfficacy(float* e) {
    *e = _luminousEfficacy;
}

void SetLuminousEfficacy(float e) {
    _luminousEfficacy = e;
}

/* ---------------------------------------------------------------------------
  `SpectrumGray'

  Returns an achromatic value representing the spectral quantity.
  ------------------------------------------------------------------------- */
float SpectrumGray(float* spec) {
    return _gray(spec);
}

/* ---------------------------------------------------------------------------
  `SpectrumLuminance'

  Returns the luminance, photometric quantity correspoinding to the
  radiance of the given spectrum.
  ------------------------------------------------------------------------- */
float SpectrumLuminance(float* spec) {
    return _luminance(spec);
}

/* ---------------------------------------------------------------------------
  `ComputeColorConversionTransforms'

  Computes RGB <-> XYZ color transforms based on the given monitor primary
  colors and whitepoint.
  ------------------------------------------------------------------------- */
void ComputeColorConversionTransforms(float xr, float yr,
                                      float xg, float yg,
                                      float xb, float yb,
                                      float xw, float yw) {
    CIE_x_r = xr; CIE_y_r = yr;
    CIE_x_g = xg; CIE_y_g = yg;
    CIE_x_b = xb; CIE_y_b = yb;
    CIE_x_w = xw; CIE_y_w = yw;

    _setcolortrans(_xyz2rgbmat,       /* XYZ to RGB */
                   (CIE_y_g - CIE_y_b - CIE_x_b * CIE_y_g + CIE_y_b * CIE_x_g) /
                   CIE_C_rD,
                   (CIE_x_b - CIE_x_g - CIE_x_b * CIE_y_g + CIE_x_g * CIE_y_b) /
                   CIE_C_rD,
                   (CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g) / CIE_C_rD,
                   (CIE_y_b - CIE_y_r - CIE_y_b * CIE_x_r + CIE_y_r * CIE_x_b) /
                   CIE_C_gD,
                   (CIE_x_r - CIE_x_b - CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r) /
                   CIE_C_gD,
                   (CIE_x_b * CIE_y_r - CIE_x_r * CIE_y_b) / CIE_C_gD,
                   (CIE_y_r - CIE_y_g - CIE_y_r * CIE_x_g + CIE_y_g * CIE_x_r) /
                   CIE_C_bD,
                   (CIE_x_g - CIE_x_r - CIE_x_g * CIE_y_r + CIE_x_r * CIE_y_g) /
                   CIE_C_bD,
                   (CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r) / CIE_C_bD);

    _setcolortrans(_rgb2xyzmat,   /* RGB to XYZ */
                   CIE_x_r * CIE_C_rD / CIE_D, CIE_x_g * CIE_C_gD / CIE_D,
                   CIE_x_b * CIE_C_bD / CIE_D,
                   CIE_y_r * CIE_C_rD / CIE_D, CIE_y_g * CIE_C_gD / CIE_D,
                   CIE_y_b * CIE_C_bD / CIE_D,
                   (1. - CIE_x_r - CIE_y_r)*CIE_C_rD / CIE_D,
                   (1. - CIE_x_g - CIE_y_g)*CIE_C_gD / CIE_D,
                   (1. - CIE_x_b - CIE_y_b)*CIE_C_bD / CIE_D);
}

void colortrans(float* col, float mat[3][3], float* res) {
    res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
    res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
    res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
}

/* ---------------------------------------------------------------------------
                                                               CIE XYZ <-> RGB
  ------------------------------------------------------------------------- */
void rgb_xyz(float* rgb, float* xyz) {
    colortrans(rgb, _rgb2xyzmat, xyz);
}

void xyz_rgb(float* xyz, float* rgb) {
    colortrans(xyz, _xyz2rgbmat, rgb);
}

/* ---------------------------------------------------------------------------
                                                           CIE XYZ <-> CIE Luv
  ------------------------------------------------------------------------- */
void xyz_luv(float* xyz, float* luv, float ymax) {
    float s, u, v;

    if(ymax <= 0.0) ymax = CIE_WHITE_Y;

    s = xyz[1] / ymax;

    if(s >= 0.008856) luv[0] = 116.0 * powf(s, 1.0 / 3.0) - 16.0;
    else               luv[0] = 903.3 * s;

    s  = xyz[0] + 15.0 * xyz[1] + 3.0 * xyz[2];

    if(s > EPSILON) {
        u  =  4.0 * xyz[0] / s;
        v  =  9.0 * xyz[1] / s;
        s  = 13.0 * luv[0];
        luv[1] = s * (u - CIE_WHITE_U);
        luv[2] = s * (v - CIE_WHITE_V);
    } else {
        luv[1] = 0.0;
        luv[2] = 0.0;
    }
}

void luv_xyz(float* luv, float* xyz, float ymax) {
    float s;

    if(ymax <= 0.0) ymax = CIE_WHITE_Y;

    if(luv[0] >= 7.9996) {
        s = (luv[0] + 16.0) / 116.0;
        xyz[1] = ymax * s * s * s;
    } else {
        xyz[1] = luv[0] * ymax / 903.3;
    }

    s = 13.0 * luv[0];

    if(s > EPSILON) {
        float q, r, a;

        q = luv[1] / s + CIE_WHITE_U;
        r = luv[2] / s + CIE_WHITE_V;
        a = 3.0 * xyz[1] * (5.0 * r - 3.0);
        xyz[2] = ((q - 4.0) * a - 15.0 * q * r * xyz[1]) / (12.0 * r);
        xyz[0] = - (a / r + 3.0 * xyz[2]);
    } else {
        xyz[0] = xyz[2] = 0.0;
    }
}

float luv_diff(float* c1, float* c2) {
    return sqrt((c1[0] - c2[0]) * (c1[0] - c2[0]) +
                (c1[1] - c2[1]) * (c1[1] - c2[1]) +
                (c1[2] - c2[2]) * (c1[2] - c2[2]));
}

/* ---------------------------------------------------------------------------
                                                           CIE xyY <-> CIE XYZ
  ------------------------------------------------------------------------- */
void xyz_xyy(float* xyz, float* xyy) {
    float denom = xyz[0] + xyz[1] + xyz[2];

    xyy[0] = xyz[0] / denom;
    xyy[1] = xyz[1] / denom;
    xyy[2] = xyz[1];
}

void xyy_xyz(float* xyy, float* xyz) {
    xyz[0] = xyy[2] * xyy[0];
    xyz[1] = xyy[2];
    xyz[2] = xyy[2] * (1.0 - xyy[1] - xyy[0] * xyy[1]) / xyy[1];
}

/* ---------------------------------------------------------------------------
                                                                          MISC
  ------------------------------------------------------------------------- */

/* Returns TRUE if the color was desaturated during gamut clipping. */
int clipgamut(float* rgb) {
    /* really SHOULD desaturate instead of just clipping! */
    int i, desaturated = 0;

    for(i = 0; i < 3; i++) {
        if(rgb[i] < 0.) {
            rgb[i] = 0.;
            desaturated = 1;
        }
    }

    return desaturated;
}

