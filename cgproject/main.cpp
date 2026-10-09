// main.cpp
//
// Philippe Bekaert, Tom Mertens, May/June 2003

//Changelog:
//20040416 tmertens
//* modified progress indicator
//* added F1 for help
//20040417 tmertens
//* added take screenshot using 'S'
//* added gamma correction

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <ctype.h>
#include <filesystem>
#ifdef HAVE_OPENMP
#include <omp.h>
#endif
// <windows.h> must come first: <GL/gl.h> and <GL/glu.h> both need the
// WINGDIAPI, APIENTRY and CALLBACK macros it defines. GLFW defines them
// itself when they are missing, but #undefs them again at the end of
// glfw3.h, which would leave <GL/glu.h> below without them.
#ifdef _WIN32
#include <windows.h>
#endif
#include <GLFW/glfw3.h>     // also pulls in <GL/gl.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include "vecmath.h"
#include <time.h>
#include "srtk/rt.h"
#include "rtOpenGL.h"
#ifdef HAVE_ASSIMP
#include "rtAssimp.h"
#endif
#include "rtMgf.h"
using namespace rt;

#include "ScreenBuffer.h"

#ifndef EPSILON
#define EPSILON 1e-6
#endif

//////////////////////////////////////////////////////
// Windowing.
//
// The framework used GLUT originally. It now uses GLFW, which is maintained,
// packaged everywhere and not deprecated on macOS. The event handlers below
// (keyboard, special, mouse, motion, reshape, display) kept their original
// names and signatures; a small layer at the bottom of this file forwards
// GLFW's callbacks to them, so there is only one place to look if you want
// to change how input is handled.
//
// The renderer still uses fixed-function OpenGL (glBegin/glEnd, display
// lists, gluPerspective), so we deliberately ask for a legacy-compatible
// context rather than a core profile one.

static GLFWwindow* window = 0;
static bool needs_redisplay = true;
static int current_mods = 0;    // modifier keys held at the last event

// Marks the window as needing a redraw; the main loop picks this up.
// (This is what glutPostRedisplay() used to do.)
static inline void post_redisplay(void) {
    needs_redisplay = true;
}

// Identifiers for the non-character keys, used to index 'keybuffer'.
// The values are arbitrary; they only have to be distinct and below 256.
#define KEY_F1         1
#define KEY_LEFT     100
#define KEY_UP       101
#define KEY_RIGHT    102
#define KEY_DOWN     103
#define KEY_PAGE_UP  104
#define KEY_PAGE_DOWN 105

// Mouse button state, matching GLFW's own values so no translation is needed.
#define MOUSE_LEFT_BUTTON  GLFW_MOUSE_BUTTON_LEFT
#define MOUSE_DOWN         GLFW_PRESS
#define MOUSE_UP           GLFW_RELEASE

int winx = 512;
int winy = 384;
float fov = 60.0;
float aspect = (float)winx / (float)winy;
vec3 pos(0.0f, 0.0f, 10.0f);
vec3 dir(0.0f, 0.0f, -1.0f);
vec3 up(0.0f, 1.0f, 0.0f);
bool Y_up = false;
ScreenBuffer* scrn = 0;
int nrlightsamples = 8;
int max_ray_depth = 3;
int nqcdivs = 5;
int screenshot_counter = 0;
int gamma_switch = false;
float gamma_cor;

//help screen
void print_help() {
    fprintf(stderr, "\nValid keys are:\n\n");
    fprintf(stderr, "+\tMove faster\n");
    fprintf(stderr, "-\tMove slower\n");
    fprintf(stderr, ">\tIncrease field of view\n");
    fprintf(stderr, "<\tDecrease field of view\n");
    fprintf(stderr, "U\tSwitch up-axis (Y or Z)\n");
    fprintf(stderr, "o\tRender using OpenGL\n");
    fprintf(stderr, "h\tGo back initial viewing position (Home)\n");
    fprintf(stderr, "W\tOpenGL Wireframe on/off\n");
    fprintf(stderr, "c\tRay Cast current view\n");
    fprintf(stderr, "r\tRay Trace (classical) current view\n");
    fprintf(stderr, "s\tRay trace (Stochastic) current view\n");
    fprintf(stderr, "S\tSave current image as `screenshot#.ppm`\n");
    fprintf(stderr, "p\tPhoton map first pass\n");
    fprintf(stderr, "d\tReDisplay last ray traced image\n");
    fprintf(stderr, "T\tTone map last ray traced image\n");
    fprintf(stderr, "g\tToggle gamma correction (default off)\n");
    fprintf(stderr, "P\tUndo tone mapping of last ray traced image (Pack)\n");
    fprintf(stderr, "0 ... 9\tEdit number (vi style: first type number, then c"
            "ommand using number)\n");
    fprintf(stderr, "C\tClear current number\n");
    fprintf(stderr, "D\tSet maximum ray tracing depth to current number\n");
    fprintf(stderr, "L\tSet maximum number of light samples to current number"
            "\n");
    fprintf(stderr, "Q\tQuit program\n");
}

//progress indicator
static const char progress_wheel[4] = {'-', '\\', '|', '/'};

void print_progress(int step, int total_steps) {
    printf("\b\b\b\b\b\b\b\b%c ", progress_wheel[(step >> 2) & 3]);
    printf("%3.1f%%", 100.0f * (float)step / (float)total_steps);
}

// Parallelising the pixel loops is only safe with the Embree backend: the
// srtk uniform grid records a ray id in every triangle it tests, so two
// threads tracing at once corrupt each other's traversal. Configure with
// -DUSE_OPENMP=ON to switch this on; it is off by default because a debugger
// is much more pleasant to use on a single-threaded render.
//
// Each row is independent -- every pixel writes only its own scrn entry --
// so the loop needs no synchronisation beyond the progress counter.
#ifdef HAVE_OPENMP
#  define RT_PARALLEL_FOR_ROWS _Pragma("omp parallel for schedule(dynamic, 4)")
   // NB: "omp atomic capture" is OpenMP 3.1, which MSVC rejects under its
   // default /openmp (2.0). A critical section is portable and costs
   // nothing here: it runs once per row, not once per pixel.
#  define RT_REPORT_ROW(counter, total)                     \
    {                                                       \
        int _done;                                          \
        _Pragma("omp critical (rt_progress)")               \
        { _done = ++(counter); }                            \
        if(omp_get_thread_num() == 0)                       \
            print_progress(_done, (total));                 \
    }
#else
#  define RT_PARALLEL_FOR_ROWS
#  define RT_REPORT_ROW(counter, total) print_progress(++(counter), (total))
#endif


// Rotates camera axis frame to world axis frame
const vec3 c2w(const vec3& v) {
    return Y_up ? v : vec3(v[0], -v[2], v[1]);
}

// Rotates world axis frame to camera axis frame
const vec3 w2c(const vec3& v) {
    return Y_up ? v : vec3(v[0], v[2], -v[1]);
}

//////////////////////////////////////////////////////
// Ray, Hit, Texture, Material, Surface, Triangle, rtShootRay(),
// etc... : see rt.h

// vec3 operations: (declared in vecmath.h)
// +, +=,
// -, -=,
// *, *= (component-wise multiplication and mult. with scalar)
// /, /= (division by scalar)
// & (dot product: enclose between brackets to ensure proper precedence)
// ^ (cross product: enclose between brackets)
// v.normalize() normalizes v
// v.normalized() returns normalized copy of v
// v.length(), square_length()
// and many more
// initialisation: vec3(x,y,z), vec3(float*) ...
// access: v.ptr() returns float*, v.X()|Y()|Z(), v[0]|[1]|[2] ...

//////////////////////////////////////////////////////
// Ray casting: activated when pressing 'c' on the drawing
// window.

// Phong shading model
const vec3 phong(const vec3& N,   // shading normal
                 const vec3& L,   // light direction
                 const vec3& V,   // direction to viewer
                 const vec3& Rd,  // diffuse color
                 const vec3& Rs,  // specular color
                 const float Ns) { // shininess (Phong exponent)
    // diffuse term: Rd times absolute value of cosine between light
    // direction and shading normal
    vec3 color = Rd * fabs(N & L);

    if(Ns > 0.) {   // specular term
        vec3 R = 2 * (N & V) * N - V; // ideal reflected direction
        float a = R & L;

        if(a > 0.)
            color += Rs * powf(a, Ns);
    }

    return color;
}

// headlight Phong shader for ray casting
// This means: Phong shader, with point light at the viewing
// position (= light shining in direction ray.dir).
vec3 headlight_phong(const Ray& ray, Hit* hit) {
    vec3 N(hit->getShadingNormal());
    vec3 V = -vec3(ray.dir);         // direction to viewer
    vec3 L = -vec3(ray.dir);         // direction to light (at viewer)

    Surface* surf = hit->triangle->surface; // short cut
    Texture* txt = surf->texture;
    Material* mat = surf->material;
    vec3 Rd(txt                   // diffuse color: hit surface has texture?
            ? txt->lookup(hit->getTexCoord()) // yes: use texture color
            : mat->diffuse);                  // no: use diffuse material color
    vec3 Rs(mat->specular);      // specular color
    float Ns = mat->shininess;   // shininess

    vec3 Ed(mat->emissivity);    // emissivity

    if(mat->is_light()) Ed /= Ed.max_comp();   // rescale so max comp is 1

    return Ed + phong(N, L, V, Rd, Rs, Ns);
}

// ray casts current view
void raycast(void) {
    // Set screen buffer view geometry: see ScreenBuffer.h
    scrn->setView(c2w(pos), c2w(pos + dir), c2w(up), fov * M_PI / 180.);

    // Clear screen buffer
    scrn->clear();

    // ray cast
    printf("Ray casting\n"); fflush(stdout);
    clock_t start = clock();

    int rows_done = 0;

    RT_PARALLEL_FOR_ROWS
    for(int row = 0; row < scrn->height; row++) {
        for(int col = 0; col < scrn->width; col++) {
            // center of pixel (col,row) has screen coordinates
            // (col+0.5,row+0.5).
            Ray ray(scrn->eye, scrn->getDirection(col + 0.5, row + 0.5));
            Hit* hit = rtShootRay(ray);

            if(hit) {
                vec3 color = headlight_phong(ray, hit);
                scrn->setHDRPixel(col, row, color);
            }
        }

        RT_REPORT_ROW(rows_done, scrn->height);
    }

    printf(" done (%.3f secs.)\n", ((float)clock() - (float)start) /
            (float)CLOCKS_PER_SEC);

    // Convert HDR (3 floats RGB) pixels to packed RGBA pixels
    // (1 unsigned int) for display.
    scrn->pack();        // without tone mapping
    //  scrn->tonemap();     // with tone mapping
}

//////////////////////////////////////////////////////
// classical ray tracing: activated when pressing 'r' on
// the drawing window

// computes midpoint of triangle: for pre-computing light triangle midpoint
const vec3 midpoint(const Triangle* tri) {
    R3* v = tri->getCoords();        // get vertex coordinates
    vec3 a(v[0]), b(v[1]), c(v[2]);  // convert R3 to vec3
    return 0.33333333 * (a + b + c);
}

// cmputes area of triangle
float area(const Triangle* tri) {
    R3* v = tri->getCoords();        // get vertex coordinates
    vec3 a(v[0]), b(v[1]), c(v[2]);  // convert R3 to vec3
    return 0.5 * ((b - a) ^ (c - a)).length();
}

// light source triangles in current model
struct Light {            // represents light source triangle
    Triangle* tri;          // pointer to light source triangle
    vec3 Ed_classic;        // emissivity rescaled for classic ray tracing
    vec3 Ed;                // true emissivity (Watts per square meter)
    vec3 mid;               // mid point
    vec3 power;             // light source power (Watts) = emissivity * area

    void init(Triangle* tri) {
        Light::tri = tri;
        Ed = vec3(tri->surface->material->emissivity);
        Ed_classic = Ed / Ed.max_comp();
        mid = midpoint(tri);
        power = area(tri) * Ed;
    }
};
static Light* lights = 0; // array of light source triangles
static int nrlights = 0;  // nr of light source triangles

// For counting the nr of light source triangles in the current model
// (traverses the model and counts), uses nrlights global var.
void count_triangle_if_light(Triangle* tri) {
    if(tri->surface->material->is_light()) nrlights++;
}

int count_lights(void) {
    nrlights = 0;
    Handler handler;   // see srtk/rt
    handler.triangle = count_triangle_if_light;  // call this for each triangle
    rtTraverse(handler);
    return nrlights;
}

// Add's a Light struct for the triangle to the light list if the triangle
// is on a light source (emissivity nonzero)
void get_light_triangle(Triangle* tri) {
    if(tri->surface->material->is_light())
        lights[nrlights++].init(tri);
}

// Compares two light source triangles based on their self-emitted power (for
// sorting the light sources). Uses sum of the components = sum of
// power in R, G and B color channel.
// This routine is used to sort the light source triangles according to
// descending power with qsort. It returns positive if l1 is weaker than l2,
// unlike what you would expect at first.
int cmp_lights_sort_descending(const Light* l1, const Light* l2) {
    float a = l1->power.sum_comp();
    float b = l2->power.sum_comp();
    return a < b ? 1 : (a > b ? -1 : 0);
}

// Retrieves the sorted list of all light source triangles, traverses model
// twice: once for counting how many triangles are on light sources, and once
// for adding them to the array lights, allocated to the right size.
void get_lights(void) {
    printf("Building light source triangle list ... "); fflush(stdout);

    if(lights) delete [] lights;    // delete old list

    nrlights = count_lights();      // count nr of light triangles
    lights = new Light [nrlights];  // allocate new list

    nrlights = 0;     // use global nrlights as index now in lights array
    Handler handler;  // scene traversal: see srtk/rt.h
    handler.triangle = get_light_triangle;
    rtTraverse(handler);

    // sort so the most powerful light source triangles come first
    qsort(lights, nrlights, sizeof(Light),
          (int (*)(const void*, const void*))cmp_lights_sort_descending);

    printf("done: %d light triangles\n", nrlights);
}

struct MyHit {   // contains hit data in the way we need it here
    vec3 pos,   // hit position
         normal,   // shading normal
         vdir,     // incident ray direction
         Rd,       // diffuse reflectivity
         Rs,       // specular reflectivity
         Rt,       // (specular) transmissivity
         Ed,       // true light source intensity
         Ed_classic; // light source intensity rescaled for classic ray tracing
    float Ns;   // specular reflection Phong exponent
    float nr;   // index of refraction
    Triangle* tri;  // hit triangle

    MyHit(Hit& hit, const Ray& inray) {
        normal = vec3(hit.getShadingNormal());
        vdir = -vec3(inray.dir);
        pos = vec3(hit.point);

        tri = hit.triangle;
        Surface* surf = tri->surface;   // short cut
        Material* mat = surf->material;
        Rd = vec3(surf->texture
                  ? surf->texture->lookup(hit.getTexCoord())
                  : mat->diffuse);
        Rs = vec3(mat->specular);      // specular color
        Rt = vec3(mat->transmissivity); // transmissivity
        Ns = mat->shininess;   // shininess
        nr = mat->indexOfRefraction;    // index of refraction
        Ed = vec3(mat->emissivity);     // emissivity
        Ed_classic = mat->is_light() ? Ed / Ed.max_comp() : Ed;
    }
};

// trace shadow ray. return light source contribution
const vec3 classic_trace_light(MyHit& hit, Light& light) {
    vec3 ldir = light.mid - hit.pos;
    Ray shadowray(hit.pos.ptr(), ldir.ptr(), EPSILON, 1 - EPSILON); // see rt.h
    shadowray.exclude[0] = hit.tri;   // avoid immediate self-intersections
    shadowray.exclude[1] = light.tri;

    if(!rtTestVisibility(shadowray))
        return vec3(0, 0, 0); // shadow ray occluded

    // compute shade from light
    ldir.normalize();
    return phong(hit.normal, ldir, hit.vdir, hit.Rd, hit.Rs, hit.Ns)
           * light.Ed_classic;
}

// traces shadow ray to all light sources
const vec3 classic_trace_lights(MyHit& hit) {
    vec3 color(0, 0, 0);

    for(int i = 0; i < nrlights && i < nrlightsamples; i++)
        color += classic_trace_light(hit, lights[i]);

    return color;
}

const vec3 classic_trace_recursive(const Ray& ray, int depth);

// computes ideal reflected direction
const vec3 ideal_reflected_direction(const vec3& I,  // direction of incidentce
                                     const vec3& N) { // normal
    return (2 * (N & I)) * N - I;
}

// traces reflection ray into ideal reflected direction
const vec3 classic_trace_reflection(MyHit& hit, int depth) {
    if(!hit.tri->surface->material->is_specular_reflector())
        return vec3(0, 0, 0);

    vec3 R = ideal_reflected_direction(hit.vdir, hit.normal);
    Ray ray(hit.pos, R);
    ray.exclude[0] = hit.tri;   // avoid immediate self-intersection
    return hit.Rs * classic_trace_recursive(ray, depth + 1);
}

// compute ideal refracted direction. Returns 'true' in
// total_internal_reflection if total internal reflection happens
// instead of refraction
const vec3 ideal_refracted_direction(const vec3& I,  // incident direction
                                     const vec3& _N, // normal
                                     float n,        // refraction index
                                     bool* total_internal_reflection) {
    
    vec3 N = _N;
    float c1 = _N & I;

    if (c1 > 0) {
        n = 1.0f / n;
    }
    else {
        N = -N;
        c1 = -c1;
    }

    const float c2 = 1.0f - n * n * (1.0f - c1 * c1);

    if (c2 < 0) {
        *total_internal_reflection = true;
        return ideal_reflected_direction(I, N);
    }

    *total_internal_reflection = false;
    return -n * I + (n * c1 - std::sqrt(c2)) * N;
}

// traces refraction ray into ideal refracted direction
const vec3 classic_trace_refraction(const MyHit& hit, int depth) {
    if(!hit.tri->surface->material->is_transparent())
        return vec3(0, 0, 0);

    bool total_internal_reflection;
    vec3 R = ideal_refracted_direction(hit.vdir, hit.normal, hit.nr,
                                       &total_internal_reflection);
    Ray ray(hit.pos, R);
    ray.exclude[0] = hit.tri;   // avoid immediate self-intersection
    vec3 color = classic_trace_recursive(ray, depth + 1);
    return total_internal_reflection ? color : hit.Rt * color;
}

// classic recursive ray tracing shader
const vec3 classic_trace_recursive(const Ray& ray, int depth) {
    Hit hit;

    if(depth > max_ray_depth || !rtShootRay(ray, &hit))
        return vec3(0, 0, 0); // too deep, or ray hits nothing, return black

    MyHit myhit(hit, ray);
    return vec3(myhit.Ed_classic)           // light source color
           + classic_trace_lights(myhit)         // direct light
           + classic_trace_reflection(myhit, depth)     // specular reflection
           + classic_trace_refraction(myhit, depth);    // specular refraction
}

void raytrace(void) {
    scrn->setView(c2w(pos), c2w(pos + dir), c2w(up), fov * M_PI / 180.);
    scrn->clear();

    // Get list of light sources
    get_lights();

    if(nrlights == 0) {
        fprintf(stderr, "No lights in scene. Define some first by left-clicking"
                " some surfaces while holding the ctrl key.\n");
        return;
    }

    if(nrlights > nrlightsamples)
        printf("Too many lights: using only the %d most powerful\n",
                nrlightsamples);

    printf("Max. ray recursion depth = %d\n", max_ray_depth);

#ifdef HAVE_OPENMP
    printf("Using %d threads\n", omp_get_max_threads());
#endif

    printf("Classic ray tracing\n"); fflush(stdout);
    clock_t start = clock();

    int rows_done = 0;

    RT_PARALLEL_FOR_ROWS
    for(int row = 0; row < scrn->height; row++) {
        for(int col = 0; col < scrn->width; col++) {
            Ray ray(scrn->eye, scrn->getDirection(col + 0.5, row + 0.5));
            vec3 color = classic_trace_recursive(ray, 0 /*depth*/);
            scrn->setHDRPixel(col, row, color);
        }

        RT_REPORT_ROW(rows_done, scrn->height);
    }

    printf(" done (%.3f secs.)\n", ((float)clock() - (float)start) /
            (float)CLOCKS_PER_SEC);

    scrn->pack();        // without tone mapping
    //  scrn->tonemap();     // with tone mapping
}

//////////////////////////////////////////////////////
// stochastic ray tracing: activated by pressing 's' on
// the drawing window
void stochastic_raytrace(void) {
    printf("stochastic raytrace\n");
}

//////////////////////////////////////////////////////
// photon mapping first pass (use stochastic ray tracing for
// second pass): activated by pressing 'p' on the drawing
// window
void photon_map(void) {
    printf("photon map\n");
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// You'll probably not need anything below here.

// controls whether display() redisplays ray traced
// screen or renders model using OpenGL
bool display_scrn = false;

// You don't need the following global variables
bool keybuffer[256];
GLuint displaylist;
static const char* filename = 0;
bool wire_frame = false;
bool mouse_look = false;
int mouse_x, mouse_y;

float alpha = 0.0f, beta = 0.0f; // view orientation
float stepsize = 10.0f;      // step size for moving viewpoint

// compute viewing position from where we see the whole scene
void home(void) {
    vec3 min, max;
    rtGetBounds(min.ptr(), max.ptr());
    min = w2c(min);   // rotate from world to camera axis frame
    max = w2c(max);
    float diam = (max - min).length(); // box diameter
    vec3 center = (max + min) * 0.5;  // box center
    float h = 2.*tan(0.5 * fov * M_PI / 180.);
    pos = center - (diam / h) * dir;

    stepsize = diam / 250.;
}

// get near and far clipping plane distance, depending on
// scene bounding bos, viewing position and direction
void get_near_far(float* clipnear, float* clipfar) {
    vec3 b[2];
    rtGetBounds(b[0].ptr(), b[1].ptr());
    b[0] = w2c(b[0]);
    b[1] = w2c(b[1]);

    *clipnear = 1e30;
    *clipfar = 0.;
    // iterate over the 8 corners of the bounding box to find
    // minimum and maximum distance to viewing position, parallel
    // to viewing direction
    dir.normalize();  // normalize viewing direction

    for(int i = 0; i <= 1; i++) {
        for(int j = 0; j <= 1; j++) {
            for(int k = 0; k <= 1; k++) {
                float dist = (vec3(b[i].X(), b[j].Y(), b[k].Z()) - pos) & dir;

                if(dist < *clipnear) *clipnear = dist;

                if(dist > *clipfar)  *clipfar = dist;
            }
        }
    }

    // extend range a bit
    *clipnear -= (*clipfar - *clipnear) * 0.01;
    *clipfar += (*clipfar - *clipnear) * 0.01;

    if(*clipfar < 1e-5) {   // world behind us
        *clipnear = 1; *clipfar = 100;   // doesn't really matter
    } else {
        *clipnear = *clipfar / 250.;
    }
}

// Renders bounding box (sometimes useful if you're looking at the dark outside
// of your model)
void render_bounds(void) {
    vec3 b[2];   // min and max
    rtGetBounds(b[0].ptr(), b[1].ptr());
    b[0] += (b[0] - b[1]) * 0.01;
    b[1] -= (b[0] - b[1]) * 0.01;

    vec3 c[8];   // 8 corners of the bounding box

    for(int i = 0; i < 8; i++)
        c[i] = vec3(b[i % 2].X(), b[(i / 2) % 2].Y(), b[(i / 4) % 2].Z());

    // draw the 12 edges
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 0.0); // yellow
    glBegin(GL_LINES);
    glVertex3fv(c[0]);  glVertex3fv(c[1]);
    glVertex3fv(c[1]);  glVertex3fv(c[3]);
    glVertex3fv(c[3]);  glVertex3fv(c[2]);
    glVertex3fv(c[2]);  glVertex3fv(c[0]);
    glVertex3fv(c[0]);  glVertex3fv(c[4]);
    glVertex3fv(c[1]);  glVertex3fv(c[5]);
    glVertex3fv(c[2]);  glVertex3fv(c[6]);
    glVertex3fv(c[3]);  glVertex3fv(c[7]);
    glVertex3fv(c[4]);  glVertex3fv(c[5]);
    glVertex3fv(c[5]);  glVertex3fv(c[7]);
    glVertex3fv(c[7]);  glVertex3fv(c[6]);
    glVertex3fv(c[6]);  glVertex3fv(c[4]);
    glEnd();
    glEnable(GL_LIGHTING);
}

// renders current view with OpenGL
void draw_OpenGL(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(wire_frame)
        glPolygonMode(GL_FRONT, GL_LINE);
    else
        glPolygonMode(GL_FRONT, GL_FILL);

    float clipnear, clipfar;
    get_near_far(&clipnear, &clipfar);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, aspect, clipnear, clipfar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(pos[0], pos[1], pos[2],
              pos[0] + dir[0], pos[1] + dir[1], pos[2] + dir[2],
              0.0, 1.0, 0.0);

    if(!Y_up)  //convert 3ds coordinate system (Z up) to OpenGL's system (Y-up)
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    glCallList(displaylist);
    render_bounds();
}

// draws current's screen buffer RGBA pixels
void draw_screen(void) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, scrn->width, 0, scrn->height, -1.0, 1.0);

    glDisable(GL_DEPTH_TEST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);

    if(scrn->width % 8 == 0) {
        glRasterPos2i(0, 0);
        glDrawPixels(scrn->width, scrn->height, GL_RGBA, GL_UNSIGNED_BYTE,
                scrn->rgba);
    } else {
        for(int j = 0; j < scrn->height; j++) {
            glRasterPos2i(0, j);
            glDrawPixels(scrn->width, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                    scrn->rgba + j * scrn->width);
        }
    }

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    display_scrn = true;
}

// main redisplay function
void display() {
    if(display_scrn)
        draw_screen();
    else
        draw_OpenGL();

    glfwSwapBuffers(window);
}

// processes arrow key presses, accumulated in keybuffer
bool process_buffer() {
    //process keyboard status
    if(keybuffer[KEY_UP]) {
        pos = pos + stepsize * dir;
        return true;
    }

    if(keybuffer[KEY_DOWN]) {
        pos = pos - stepsize * dir;
        return true;
    }

    if(keybuffer[KEY_PAGE_UP]) {
        pos = pos + stepsize * up;
        return true;
    }

    if(keybuffer[KEY_PAGE_DOWN]) {
        pos = pos - stepsize * up;
        return true;
    }

    if(keybuffer[KEY_LEFT]) {
        vec3 l;
        l.from_spherical(stepsize, DEGTORAD * (alpha + 180.0f),
                DEGTORAD * 90.0f);
        pos = pos + l;
        return true;
    }

    if(keybuffer[KEY_RIGHT]) {
        vec3 r;
        r.from_spherical(stepsize, DEGTORAD * alpha, DEGTORAD * 90.0f);
        pos = pos + r;
        return true;
    }

    return false;
}

// idle loop
void idle() {
    if(process_buffer()) {
        display_scrn = false;
        post_redisplay();
    }
}

// processes window resize events
void reshape(int w, int h) {
    glViewport(0, 0, winx = w, winy = h);
    aspect = double(w) / double(h);

    scrn->resize(winx, winy);

    display_scrn = false;
    post_redisplay();
}

void set_max_ray_depth(int num) {
    max_ray_depth = num;
    printf("Maximum ray depth set to %d.\n", num);
}

void set_nrlightsamples(int num) {
    nrlightsamples = num;
    printf("Number of light samples set to %d.\n", num);
}

// processes keyboard character input
void keyboard(unsigned char c, int, int) {
    static int num = 0;

    switch(c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        num = num * 10 + (c - '0'); break; // edit numbers in good old vi-style

    case 'C': num = 0; break;

    case '+': stepsize *= 2.0f; break;

    case '-': stepsize *= 0.5f; break;

    case '>': fov *= 1.1f; break;

    case '<': fov *= (1.0 / 1.1); break;

    case 'W': wire_frame = !wire_frame; break;

    case 'U': Y_up = !Y_up; break;

    case 'h': home(); break;

    case 'Q': case 27: exit(0); break;

    case 'c': raycast(); display_scrn = true; break;

    case 'T':
        scrn->set_gamma(gamma_switch ? gamma_cor : 1.0f);
        scrn->tonemap(); display_scrn = true;
        break;

    case 'g':
        gamma_switch = !gamma_switch;
        scrn->set_gamma(gamma_switch ? gamma_cor : 1.0f);
        scrn->pack();
        display_scrn = true;
        break;

    case 'P': scrn->pack(); display_scrn = true; break;

    case 'r': raytrace(); display_scrn = true; break;

    case 'D': set_max_ray_depth(num); num = 0; break;

    case 'L': set_nrlightsamples(num); num = 0; break;

    case 's': stochastic_raytrace(); display_scrn = true; break;

    case 'S': {
        char fname[64];
        sprintf(fname, "screenshot%d.ppm", screenshot_counter);
        scrn->saveRGBAImage((const char*)fname);
        screenshot_counter ++;
    }
    break;

    case 'p': photon_map(); break;

    case 'o': display_scrn = false; break;

    case 'd': display_scrn = true; break;

    default:
        fprintf(stderr, "Unrecognized key '%c' pressed\n\n", c);
        print_help();
    }

    post_redisplay();
}

// if the surface pointed to at (x,y) is not yet a light source,
// then make all the surfaces made of the same material light sources
// of unit strength. If it is a light source, make them non-emissive
// surfaces again.
void toggle_light(int x, int y) {
    y = scrn->height - y - 1; // window coords run top-down

    // set current view
    scrn->setView(c2w(pos), c2w(pos + dir), c2w(up), fov * M_PI / 180.);

    // trace ray through pixel (x,y)
    Ray ray(scrn->eye, scrn->getDirection(x + 0.5, y + 0.5));
    Hit* hit = rtShootRay(ray);

    if(!hit) {
        fprintf(stderr, "No surface there.\n");
        return;
    }

    Material* mat = hit->triangle->surface->material;

    if(!mat->is_light()) {
        vec3 Ed(1, 1, 1);
        Ed.get(mat->emissivity);
        mat->flags.is_light = true;
        printf("Defined new light(s) ... use ray casting to see which\n");
    } else {
        vec3 Ed(0, 0, 0);
        Ed.get(mat->emissivity);
        mat->flags.is_light = false;
        printf("Undefined some light(s) ... use ray casting to see which\n");
    }
}

void special(int c, int, int) {
    keybuffer[c] = true;

    //process F-keys
    switch(c) {
    case KEY_F1:
        print_help();
        break;
    }
}

void special_up(int c, int, int) {
    keybuffer[c] = false;
}

void mouse(int button, int state, int x, int y) {
    if(button == MOUSE_LEFT_BUTTON) {
        mouse_x = x;
        mouse_y = y;

        if(current_mods & GLFW_MOD_CONTROL) {
            // clicking while holding ctrl : toggle light status of surface
            if(state == MOUSE_DOWN)
                toggle_light(x, y);
        } else {
            mouse_look = (state == MOUSE_DOWN);
        }
    }
}

void motion(int x, int y) {
    //implements 'mouse look'
    if(mouse_look) {
        //adjust viewing angle according to the travelled distance of mouse
        //cursor
        alpha -= float(mouse_x - x) / 4;
        beta -= float(mouse_y - y) / 4;

        if(beta > 80.0f) beta = 80.0f;

        if(beta < -80.0f) beta = -80.0f;

        dir.from_spherical(1.0f, DEGTORAD * (alpha + 270.0f), DEGTORAD *
                (beta + 90.0f));
        mouse_x = x;
        mouse_y = y;
        display_scrn = false;
    }

    post_redisplay();
}

//////////////////////////////////////////////////////
// GLFW callback forwarding. Each of these translates one GLFW event into a
// call to the corresponding handler above.

static void glfw_error(int code, const char* description) {
    fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

// Text input: GLFW's character callback already applies shift and the
// keyboard layout, so 'S' and '>' arrive as you would expect.
static void glfw_char(GLFWwindow*, unsigned int codepoint) {
    if(codepoint < 256)
        keyboard((unsigned char)codepoint, 0, 0);
}

// Non-character keys: arrows, page up/down, F1, escape.
static void glfw_key(GLFWwindow*, int key, int, int action, int mods) {
    current_mods = mods;

    int k;
    switch(key) {
    case GLFW_KEY_F1:        k = KEY_F1;        break;
    case GLFW_KEY_LEFT:      k = KEY_LEFT;      break;
    case GLFW_KEY_UP:        k = KEY_UP;        break;
    case GLFW_KEY_RIGHT:     k = KEY_RIGHT;     break;
    case GLFW_KEY_DOWN:      k = KEY_DOWN;      break;
    case GLFW_KEY_PAGE_UP:   k = KEY_PAGE_UP;   break;
    case GLFW_KEY_PAGE_DOWN: k = KEY_PAGE_DOWN; break;

    case GLFW_KEY_ESCAPE:
        // Not delivered by the character callback, so handle it here.
        if(action == GLFW_PRESS)
            keyboard(27, 0, 0);
        return;

    default:
        return;   // a printable key: the character callback deals with it
    }

    if(action == GLFW_PRESS)
        special(k, 0, 0);
    else if(action == GLFW_RELEASE)
        special_up(k, 0, 0);
}

// The cursor is reported in window coordinates, but the screen buffer is
// sized in framebuffer pixels. On a HiDPI display those differ.
static void cursor_to_pixels(double xin, double yin, int* xout, int* yout) {
    int ww = 1, wh = 1, fw = 1, fh = 1;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fw, &fh);
    *xout = (int)(xin * (ww > 0 ? (double)fw / (double)ww : 1.0));
    *yout = (int)(yin * (wh > 0 ? (double)fh / (double)wh : 1.0));
}

static void glfw_mouse_button(GLFWwindow* w, int button, int action, int mods) {
    current_mods = mods;

    double cx, cy;
    glfwGetCursorPos(w, &cx, &cy);

    int x, y;
    cursor_to_pixels(cx, cy, &x, &y);
    mouse(button, action, x, y);
}

static void glfw_cursor_pos(GLFWwindow*, double cx, double cy) {
    int x, y;
    cursor_to_pixels(cx, cy, &x, &y);
    motion(x, y);
}

static void glfw_framebuffer_size(GLFWwindow*, int w, int h) {
    if(w > 0 && h > 0)
        reshape(w, h);
}

// True while the camera is being moved, so the main loop knows to keep
// spinning instead of blocking on the next event.
static bool movement_key_held(void) {
    return keybuffer[KEY_UP] || keybuffer[KEY_DOWN] ||
           keybuffer[KEY_LEFT] || keybuffer[KEY_RIGHT] ||
           keybuffer[KEY_PAGE_UP] || keybuffer[KEY_PAGE_DOWN];
}

// Loads the model, allocates the screen buffer and picks the initial
// viewpoint. Everything here is independent of OpenGL, so batch mode can use
// it without a window.
void init_scene() {
    // .mgf goes through the MGF parser, which understands its physically
    // specified materials. Everything else is handed to Assimp.
    std::string ext = std::filesystem::path(filename).extension().string();
    for(size_t i = 0; i < ext.size(); i++)
        ext[i] = (char)tolower((unsigned char)ext[i]);

    if(ext == ".mgf") {
        rtMgfSetNrQuartCircDivs(nqcdivs);

        if(!rtReadMgf(filename))
            exit(1);
    } else {
#ifdef HAVE_ASSIMP
        if(!rtReadAssimp(filename))
            exit(1);
#else
        fprintf(stderr, "Cannot load '%s': this build only supports .mgf "
                "files. Reconfigure with -DUSE_ASSIMP=ON for the other "
                "formats.\n", filename);
        exit(2);
#endif
    }

    // create screen buffer
    scrn = new ScreenBuffer(winx, winy);

    // load settings (now only gamma correction)
    // If file not found, load defaults.
    FILE* settings = fopen("settings.txt", "r");

    if(!settings) {
        gamma_cor = 1.2f;
    } else {
        fscanf(settings, "GAMMA = %f", &gamma_cor);
        fclose(settings);
    }

    home();
}

// initializations
void init() {
    //init opengl
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glLightfv(GL_LIGHT0, GL_AMBIENT, vec4(0.2f, 0.2f, 0.2f, 1.0f));
    glLightfv(GL_LIGHT0, GL_DIFFUSE, vec4(1.0f, 1.0f, 1.0f, 1.0f));
    glLightfv(GL_LIGHT0, GL_SPECULAR, vec4(0.2f, 0.2f, 0.2f, 1.0f));
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    init_scene();

    //create display list
    fprintf(stderr, "Building display list ... ");
    displaylist = glGenLists(1);
    glNewList(displaylist, GL_COMPILE);
    rtRenderWithOpenGL();
    glEndList();
    fprintf(stderr, "done\n");

    // initially, render using OpenGL
    display_scrn = false;
}

int main(int argc, char** argv) {
    // Optional "--render <file.ppm>" turns on batch mode (see below).
    const char* batch_output = 0;
    {
        int out = 1;
        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "--render") == 0 && i + 1 < argc) {
                batch_output = argv[++i];
            } else if(strcmp(argv[i], "--size") == 0 && i + 2 < argc) {
                winx = atoi(argv[++i]);
                winy = atoi(argv[++i]);
                if(winx < 1 || winy < 1) {
                    fprintf(stderr, "--size needs two positive integers\n");
                    exit(1);
                }
                aspect = (float)winx / (float)winy;
            } else {
                argv[out++] = argv[i];
            }
        }
        argc = out;
    }

    if(argc != 2) {
        printf("No 3d model file name specified.\n\n"
               "Usage: %s <filename> [--render out.ppm] [--size W H]\n\n"
               "  --render  ray trace one frame to out.ppm and exit, without\n"
               "            opening a window\n"
               "  --size    render resolution (default %dx%d)\n\n",
               argv[0], winx, winy);
#ifdef HAVE_ASSIMP
        printf("filename = a .mgf file, or any model Assimp can read:\n%s\n\n",
               rtAssimpSupportedExtensions());
#else
        printf("filename = a .mgf file (this build has no Assimp loader)\n\n");
#endif
        exit(1);
    }

    // Change into the model's directory so that textures and includes it
    // refers to by relative path resolve, then load it by bare name.
    // (Kept in a std::string: the previous version handed out a pointer into
    // a temporary, which dangled immediately.)
    static std::string model_name;
    {
        std::error_code ec;
        std::filesystem::path given(argv[1]);

        if(!std::filesystem::exists(given, ec)) {
            fprintf(stderr, "Cannot open '%s': no such file.\n", argv[1]);
            exit(1);
        }

        std::filesystem::path dir = given.parent_path();
        if(!dir.empty()) {
            std::filesystem::current_path(dir, ec);
            if(ec) {
                fprintf(stderr, "Cannot enter directory '%s': %s\n",
                        dir.string().c_str(), ec.message().c_str());
                exit(1);
            }
        }

        model_name = given.filename().string();
        filename = model_name.c_str();
    }

    // Batch mode: render one frame from the default viewpoint straight to a
    // .ppm and exit, without opening a window. Useful for rendering without
    // sitting in front of the machine, and for checking that two traversal
    // backends agree on the same scene.
    if(batch_output) {
        init_scene();

        printf("Batch render %dx%d -> %s\n", winx, winy, batch_output);
        raytrace();
        scrn->saveRGBAImage(batch_output);
        printf("Wrote %s\n", batch_output);
        return 0;
    }

    glfwSetErrorCallback(glfw_error);

    if(!glfwInit()) {
        fprintf(stderr, "Could not initialise GLFW.\n");
        exit(1);
    }

    // Frame buffer: 32 bit RGBA, 24 bit depth, 8 bit stencil, double
    // buffered. Ask for a legacy-compatible context: this renderer uses the
    // fixed-function pipeline.
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    window = glfwCreateWindow(winx, winy, argv[1], NULL, NULL);

    if(!window) {
        fprintf(stderr, "Could not create a window. An OpenGL driver "
                "supporting the compatibility profile is required.\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetCharCallback(window, glfw_char);
    glfwSetKeyCallback(window, glfw_key);
    glfwSetMouseButtonCallback(window, glfw_mouse_button);
    glfwSetCursorPosCallback(window, glfw_cursor_pos);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size);

    memset(keybuffer, 0, 256 * sizeof(bool));

    // The screen buffer and the OpenGL display list are sized in framebuffer
    // pixels, which are not the requested window size on a HiDPI display.
    glfwGetFramebufferSize(window, &winx, &winy);
    aspect = double(winx) / double(winy);
    glViewport(0, 0, winx, winy);

    init();

    printf("\n\nPress F1 for help\n\n");

    // Main loop. This replaces glutMainLoop(): poll while the camera is
    // being moved, block on the next event when it is not, so that an idle
    // window does not spin a core.
    while(!glfwWindowShouldClose(window)) {
        idle();

        if(needs_redisplay) {
            needs_redisplay = false;
            display();
        }

        if(movement_key_held())
            glfwPollEvents();
        else
            glfwWaitEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

