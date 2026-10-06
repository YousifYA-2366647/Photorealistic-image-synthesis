// ScreenBuffer.cpp : screen buffer class

#include <stdio.h>
#include <float.h>
#include <math.h>

#include "ScreenBuffer.h"

#ifndef EPSILON
#define EPSILON 1e-6
#endif

bool ScreenBuffer::setView(const vec3& eye, const vec3& center, const vec3& up,
        float fov) {
    ScreenBuffer::eye = eye;
    ScreenBuffer::center = center;
    ScreenBuffer::up = up;
    ScreenBuffer::fov = fov;

    float viewdist;
    Z = (eye - center).normalized(&viewdist);

    if(viewdist < EPSILON) {
        fprintf(stderr,
                "ScreenBuffer::setView: eye point and focus point coincide\n");
        view_initialized = false;
        return false;
    }

    float n;
    X = (up ^ Z).normalized(&n);

    if(n < EPSILON) {
        fprintf(stderr,
                "ScreenBuffer::setView: up-direction and viewing direction coi"
                "ncide\n");
        view_initialized = false;
        return false;
    }

    Y = (Z ^ X).normalized();

    if(width > height) {
        float vfov = fov;
        vsize = tan(vfov / 2.) * 2.;
        hsize = vsize * (float)width / (float)height;
    } else {
        float hfov = fov;
        hsize = tan(hfov / 2.) * 2.;
        vsize = hsize * (float)height / (float)width;
    }

    h = hsize / (float)width;
    v = vsize / (float)height;

    H = X * h; // translates to next pixel to the right
    V = Y * v; // translates to next pixel above
    O = eye - Z - X * (hsize / 2.) - Y * (vsize / 2.);
    OE = O - eye;

    view_initialized = true;
    return true;
}

double ScreenBuffer::adaptation_level(float scale) const {
    double sumloglum = 0.;
    int count = 0;
    vec3* r = hdr;

    for(int i = 0; i < width * height; i++, r++) {
        // Same units as tonemap(), which compares raw max_comp() against
        // the reference white derived from this value (no factor PI).
        double lum = scale * r->max_comp();

        if(lum > 1e-20) {
            sumloglum += log(lum);
            count++;
        }
    }

    return (count > 0) ? exp(sumloglum / (double)count) : 0.;
}

const vec3 ScreenBuffer::tonemap(const vec3& rad, float refwhite, float scale) {
    float max = rad.max_comp();

    if(max < 1e-32 || refwhite < 1e-32) return rad;

    float rel = max / refwhite;
    float s = (rel > 0.008856)
              ? (1.16 * powf(rel, 0.33) - 0.16)
              : (9.033 * rel);

    return rad * (scale * s / max);
}

void ScreenBuffer::tonemap(float scale) {
    double refwhite  = adaptation_level() * 5.42;

    for(int i = 0; i < width * height; i++)
        rgba[i] = pack(tonemap(hdr[i], refwhite, scale));
}

void ScreenBuffer::pack(float scale) {
    for(int i = 0; i < width * height; i++)
        rgba[i] = pack(hdr[i] * scale);
}

void ScreenBuffer::saveRGBAImage(const char* filename) {
    fprintf(stderr, "Saving PPM image to '%s' ... ", filename);
    FILE* f = fopen(filename, "wb");

    if(!f) {
        perror("fopen");
        return;
    }

    fprintf(f, "P6\n%d %d\n255\n", width, height);

    for(int j = height - 1; j >= 0; j--) {
        unsigned char* p = (unsigned char*)(rgba + j * width);

        for(int i = 0; i < width; i++, p += 4) {
            fprintf(f, "%c%c%c", p[0], p[1], p[2]);
        }
    }

    fclose(f);
    fprintf(stderr, "done.\n");
}

#ifdef NEVER

// TODO: saves PIC. Should at least inspect file name extension.
void ScreenBuffer::saveHDRImage(const char* filename, double lwa,
        double scale) {
    class file f;

    if(!f.open(filename, "w")) {
        Error("ScreenBuffer:saveHDRImage", "Can't open file '%s' for writing",
                filename);
        return;
    }

    WritePIC(&f, width, height, 3, (float*)hdr, true /*reverse*/);

    f.close();
}
#endif

