// srtk: Simple Ray Tracing Kernel - public header file (the only one)
// Philippe Bekaert - May/June 2003
// Philippe.Bekaert@luc.ac.be

#ifndef SRTK_RT_H
#define SRTK_RT_H

// HUGE is an old SVID macro: MSVC's <math.h> still has it, glibc >= 2.27
// dropped it. Fall back to the value glibc used (FLT_MAX).
#include <math.h>
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

namespace rt {

    extern void rtInit(void);    // initialize
    extern void rtExit(void);    // terminate

    // Start/end declaration of a virtual world. There can be
    // only a single one at this time. (TODO: multiple worlds that can include
    // each other)
    extern void rtBeginWorld(void);   // start definition of world
    extern void rtEndWorld(void);     // end definition of world, build ray tracing acceleration data structure

    // Transforms, stack corresponds to OpenGL MODELVIEW stack
    extern void rtLoadIdentity(void);
    extern void rtPushMatrix(void);
    extern void rtPopMatrix(void);
    extern void rtLoadMatrix(const float* m4x4);
    extern void rtMultMatrix(const float* m4x4);
    extern void rtTranslate(float* xyz);
    extern void rtScale(float* xyz);
    extern void rtRotate(float degrees, float* xyz);

    // Define material (also makes it current)
    extern int rtMaterial(const float* emissivity, // (diffuse) light source intensity
                          const float* diffuse,  // diffuse reflectivity
                          const float* specular, // specular reflectivity
                          const float shininess, // Phong exponent, if larger than 128, the material is considered a perfect mirror
                          const float* transmissivity,  // (specular) transmittivity
                          const float indexOfRefraction,
                          void* client_data = 0); // pointer to additional data of your own yuo can associate with a material

    // Sets current material;
    extern void rtBindMaterial(int material_id);

    // Define texture with 'width' columns, 'height' rows and 'channels'
    // channels (3 for RGB, 4 for RGBA type).
    // 'map' is the image data: 3/4 bytes per RGB/RGBA texel,
    // enumerated bottom-to-top and left-to-right (first bytes
    // correspond with texture coordinates (0,0)).
    // only pointer to 'map' is copied (not map itself).
    // Returns texture ID.
    // Also makes texture current.
    // client_data points to (optional) additional data of your own you
    // want to associate with the texture object.
    extern int rtTexture(int width, int height, int channels, const unsigned char* map, void* client_data = 0);

    // Sets current texture
    extern void rtBindTexture(int texture_id);
    extern void rtUnbindTexture(void);   // removes texture

    // Begin/end specification of a triangle set.
    // Returns Surface ID for the created triangle set.
    extern int rtBeginTriangleSet(void* client_data = 0);
    extern void rtEndTriangleSet(void);  // end a series of triangles

    // Specify vertex coordinates, normals, colors, texture coords,:
    // like in OpenGL: vertex is "recorded" by rtVertex3f[v] (specify
    // normal, color, texture coord before coordinate). Unlike OpenGL,
    // the first vertex determines what vertex attributed will be kept
    // however. I.o.w. all vertices in a triangle set shall have the
    // same attributes.
    extern void rtVertex3(float* xyz);
    extern void rtColor3(float* rgb);
    extern void rtNormal3(float* xyz);
    extern void rtTexCoord3(float* uvw);

    // You can associate (a pointer to) your own data with vertices
    // and triangles.
    extern void rtVertexData(void* data);   // "client" data to be associated with subsequent vertices
    extern void rtTriangleData(void* data); // "client" data to be associated with subsequent triangles

    /////////////////////////////////////////////////////////////////
    // Essential data structures.

    // Simplest possible representation of 3D coordinates, texture
    // coordinates and RGB colors.
    typedef float R3[3];

    // Client data: a void*
    typedef void* ClientData;

    struct Material {  // Material data structure
        R3 emissivity;   // (diffuse) light source intensity
        R3 diffuse;      // diffuse reflectivity
        R3 specular;     // specular reflectivity
        float shininess; // Phong exponent
        R3 transmissivity;  // (perfect specular) transmissivity
        float indexOfRefraction;  // index of refraction
        ClientData data; // can be used freely by client application

        // Flags set by rtMaterial.
        struct {
            bool is_light : 1;    // if emissivity is non-zero
            bool is_diffuse_reflector : 1;  // if diffuse is non-zero
            bool is_specular_reflector : 1; // if specular is non-zero
            bool is_mirror : 1;   // if shininess is larger than 128
            bool is_transparent : 1;        // if transmissivity is non-zero
        } flags;
        // Find out whether material is for a light source, etc...
        bool is_light(void) {
            return flags.is_light;
        }
        bool is_diffuse_reflector(void) {
            return flags.is_diffuse_reflector;
        }
        bool is_specular_reflector(void) {
            return flags.is_specular_reflector;
        }
        bool is_mirror(void) {
            return flags.is_mirror;
        }
        bool is_transparent(void) {
            return flags.is_transparent;
        }

        // assigns value to members
        void set(const float* emissivity,
                 const float* diffuse,
                 const float* specular, const float shininess,
                 const float* transmissitivy, const float indexOfRefraction,
                 void* client_data = 0);
    };

    struct Texture {    // Texture map data structure
        int width, height;   // texture map width and height
        unsigned char* map;  // left-to-right, bottom-to-top
        int channels;        // 3=RGB, 4=RGBA
        ClientData data;

        // assigns value to members (copies only pointer to 'map')
        void set(int width, int height, int channels, unsigned char* map, void* client_data = 0);

        // locate texel RGB/RGBA color values (unsigned char triple/quad)
        // col and row count from 0 to width/height minus 1.
        inline unsigned char* lookup_raw(int col, int row) {
            return map + channels * (width * row + col);
        }

        // look up texture pixel value at texture coordinates (u,v).
        // Texture image corresponds with [0,1)x[0,1) square.
        // Uses periodic tiling (clamp yourself if needed).
        // Stores result in 'color' and 'value'. Returns pointer to color.
        // The looked-up values. NOTE: lookup() returns a pointer to
        // thread-local scratch, which is what you should use. These two
        // members are also updated, for compatibility, but they are shared
        // between threads -- so in a build with USE_OPENMP=ON, read the
        // returned pointer rather than these.
        R3 color;         // looked up color values ([0,1] range)
        float alpha;      // alpha value
        float* lookup(const float u, const float v);
        inline float* lookup(const float* texcoord) {
            return lookup(texcoord[0], texcoord[1]);
        }
    };

    struct Surface {   // Group of triangles sharing common material etc...
        struct Material* material;
        struct Texture* texture;
        struct Transform* transform;  // not relevant yet
        ClientData data;   // client data as set with rtBeginTriangleSet()

        struct Triangle** triangles;  // array of pointers to the triangles
        R3* vcoord;        // vertex coordinates
        R3* vcol;          // vertex colors (null if no vertex colors defined)
        R3* vnorm;         // vertex normals (or null if no vertex normals)
        R3* vtco;          // vertex texture coordinates (or null if no texture coordinates)
        ClientData* vdata; // vertex client data (or null if not given
        int nrtris, nrverts; // number of triangles and vertices in set
    };

    struct GridItem {        // This struct is not directly used in a program
        unsigned flags;        // everything in a single 4-byte quantity
        unsigned last_ray_id(void) const; // last ray id (internal use)
        void set_last_ray_id(const unsigned);
        bool is_triangle(void) const;     // returns true if the item is a triangle
        void set_is_triangle(const bool);
        struct Hit* intersect(struct Ray& ray, struct Hit* hit);
        void traverseBox(const struct Handler& handler, const struct Bounds& box, const unsigned id);
    };

    struct Triangle: public GridItem {   // triangle data structure
        Surface* surface; // material, texture, vertex coord... arrays etc...
        ClientData data;  // triangle client data
        float plane_constant;  // triangle plane constant
        R3 normal;             // triangle normal

        // Access to vertex coordinates, normals, colors, texture coordinates
        // and vertex client data. Returns pointer to 3 items.
        int vidx;         // index of first vertex in vertex coord... arrays
        inline R3* getCoords(void) const {
            return &surface->vcoord[vidx];
        }
        inline R3* getNormals(void) const {
            return surface->vnorm ? &surface->vnorm[vidx] : 0;
        }
        inline R3* getColors(void) const {
            return surface->vcol ? &surface->vcol[vidx] : 0;
        }
        inline R3* getTexCoords(void) const {
            return surface->vtco ? &surface->vtco[vidx] : 0;
        }
        inline ClientData* getVertexData(void) const {
            return surface->vdata ? &surface->vdata[vidx] : 0;
        }

        // Ray-triangle intersection. Returns 0 if no intersection.
        // If there is an intersection, fills in the provided hit
        // record (see below) and returns pointer to it.
        struct Hit* intersect(struct Ray& ray, struct Hit* hit);

        unsigned orientation(void) const; // dominant normal index (internal use)
        bool is_degenerate(void) const;   // true if triangle is degenerate
        bool is_reversed(void) const;     // true if vertices are in reverse order
        // sets flags (internal use only)
        void set_orientation(const unsigned);
        void set_degenerate(const bool);
        void set_reversed(const bool);

        // Calculates normal and plane constant after reading
        // a triangle and initializes flags.
        // Requires that reversed flag be set first.
        // Returns false if triangle is degenerate. (for internal use only)
        bool close(void);
    };

    // struct Ray and Hit: rays and ray-object intersection data
    // default intersection distance bounds
#define RT_VERY_NEAR 1e-5
#define RT_FAR_AWAY 1e30

    // allowed intersected triangle orientation w.r.t. ray direction
#define RT_FRONT 1
#define RT_BACK  2
#define RT_FRONT_AND_BACK (RT_FRONT|RT_BACK)

    // whether to search for just any hit (shadow rays). If this
    // flag is not set, we will search for the nearest hit (default).
    //
    // NOTE: with this flag set, only *whether* rtShootRay() returned a hit is
    // meaningful. With the Embree backend the query goes through
    // rtcOccluded1(), which is much faster than finding the nearest hit but
    // does not report which triangle blocked the ray -- so Hit::triangle is
    // null and Hit::s/t are zero. Use rtTestVisibility() below, or drop the
    // flag if you need the hit details.
#define RT_ANY_HIT 4

    struct Ray {      // Ray data structure
        R3 org, dir;    // origin and direction
        float mindist;  // min. allowed distance to hit point
        float maxdist;  // max. allowed distance to hit point
        Triangle* exclude[2];  // triangles to exclude from intersection
        // testing (to remedy ray tracing "acne", e.g. due to immediate
        // self-intersections)
        unsigned flags;   // intersection options

        // Set ray origin, direction, minimum/maximum intersection
        // distance and intersection options.
        // Also clears list of triangles to exclude from testing.
        void set(const float* org, const float* dir,
                 const float mindist = RT_VERY_NEAR,
                 const float maxdist = RT_FAR_AWAY,
                 const unsigned flags = RT_FRONT_AND_BACK);

        inline Ray(const float* org, const float* dir,
                   const float mindist = RT_VERY_NEAR,
                   const float maxdist = RT_FAR_AWAY,
                   const unsigned flags = RT_FRONT_AND_BACK) {
            set(org, dir, mindist, maxdist, flags);
        }

        // Used internally, or for statistics.
        unsigned id;
        int nr_bounding_box_tests, nr_triangle_tests;
    };

    // Filled in and returned by rtShootRay()
    struct Hit {      // Ray-surface hit data structure
        float dist;     // ray origin to surface hit distance
        float s, t;     // barycentric coordinates in hit triangle
        Triangle* triangle;  // pointer to hit triangle;
        R3 point;       // coordinates of intersection point

        // the data below is not automatically computed by
        // rtShootRay or Triangle::intersect.
        R3 normal;      // shading normal at intersection point
        // calculate shading normal at intersection (stores result
        // in 'normal' and returns pointer)
        float* getShadingNormal(void);

        R3 texcoord;    // texture coordinate at intersection point
        // calculate texture coordinate at intersection (stores
        // result in 'texcoord' and returns pointer)
        float* getTexCoord(void);
    };

    ///////////////////////////////////////////////////////////////
    // The following routines can only be used after rtEndWorld().

    // Retrieve world bounding box: fills in min x,y and z coordinate
    // in min and maximum x,y and z coordinate in max. min and max
    // shall point to 3 floats.
    extern void rtGetBounds(float* min, float* max);

    // Retrieve (pointer to internal) array of textures.
    // nr of textures is filled in 'nrtextures'.
    extern Texture** rtGetTextures(int* nrtextures);

    // Same, but for the materials
    extern Material** rtGetMaterials(int* nrmaterials);

    // Same, but for Surfaces (rtBeginTriangleSet returns index in this array)
    extern Surface** rtGetSurfaces(int* nrsurfaces);

    struct Handler {   // entity handlers for rtTraverse()
        void (*triangle)(Triangle* tri); // called for every Triangle
        void (*surface)(Surface* surf);  // called when starting each Surface
        static void default_triangle(Triangle*) {
            /*ignore*/
        }
        static void default_surface(Surface*) {
            /*ignore*/
        }
        Handler() {
            triangle = default_triangle;
            surface = default_surface;
        }
    };

    // Traverses world, calls appropriate handler member function
    // for each material, texture and triangle.
    extern void rtTraverse(const Handler& handler);

    // Traverses items enclosed in the given box. box points to 6 floats:
    // min_x, min_y, min_z, max_x, max_y, max_z. At this time (TODO), only
    // calls the triangle callback.
    extern void rtTraverseBox(const Handler& handler, const float* const box);

    // Returns bounding box for read world. Returns 6 floats
    // meaning: min_x, min_y, min_z, max_x, max_y, max_z.
    extern float* rtGetBounds(void);

    // Shoot ray. Returns null if nothing gets hit.
    // If there is an intersection, intersection data
    // are stored into the hit record pointed to by 'hit'
    // and 'hit' is returned.
    extern Hit* rtShootRay(const Ray& ray, Hit* hit);
    extern Hit* rtShootRay1(Ray& ray, Hit* hit);  // returns stats in ray struct

    // Same, but uses static hit record (contents lost on next
    // call to this function)
    // thread_local, not plain static: with the Embree backend the pixel
    // loops may be parallel, and a shared scratch hit record would be a data
    // race. Note the contents are still clobbered by the next call on the
    // same thread -- copy anything you need to keep.
    inline Hit* rtShootRay(Ray& ray) {
        static thread_local Hit hit;
        return rtShootRay(ray, &hit);
    }

    // Test visibility: returns true if ray hits nothing, and false if it does hit something
    inline bool rtTestVisibility(Ray& ray) {
        ray.flags |= RT_ANY_HIT;   // don't care about finding nearest
        return rtShootRay(ray) == 0;
    }

    ////////////////////////////////////////////////////
    // Alternatives for functions declared above:
    inline void rtTranslate(float x, float y, float z) {
        float v[3]; v[0] = x; v[1] = y; v[2] = z;
        rtTranslate(v);
    }

    inline void rtScale(float x, float y, float z) {
        float v[3]; v[0] = x; v[1] = y; v[2] = z;
        rtScale(v);
    }

    inline void rtRotate(float degrees, float x, float y, float z) {
        float v[3]; v[0] = x; v[1] = y; v[2] = z;
        rtRotate(degrees, v);
    }

    inline void rtVertex3(float x, float y, float z) {
        float v[3]; v[0] = x; v[1] = y; v[2] = z;
        rtVertex3(v);
    }

    inline void rtColor3(float r, float g, float b) {
        float v[3]; v[0] = r; v[1] = g; v[2] = b;
        rtColor3(v);
    }

    inline void rtNormal3(float x, float y, float z) {
        float v[3]; v[0] = x; v[1] = y; v[2] = z;
        rtNormal3(v);
    }

    inline void rtTexCoord3(float u, float v, float w) {
        float x[3]; x[0] = u; x[1] = v; x[2] = w;
        rtTexCoord3(x);
    }

    inline void rtTexCoord2(float u, float v) {
        rtTexCoord3(u, v, 0.0);
    }

#define RT_GETFLAGS(mask,off) ((flags&mask)>>off)
#define RT_SETFLAGS(mask,off,val) {flags=(flags&~mask)|(((unsigned)val<<off)&mask);}
#define RT_MAX_RAY_ID ((1u<<27)-1)   // max ray id (27 bits)
#define RT_ITM_MRID 0x07ffffff
#define RT_ITM_ORID 0
#define RT_ITM_MTRI 0x08000000
#define RT_ITM_OTRI 27
#define RT_TRI_MORI 0x30000000
#define RT_TRI_OORI 28
#define RT_TRI_MDEG 0x40000000
#define RT_TRI_ODEG 30
#define RT_TRI_MREV 0x80000000
#define RT_TRI_OREV 31
    inline unsigned GridItem::last_ray_id(void) const {
        return RT_GETFLAGS(RT_ITM_MRID, RT_ITM_ORID);
    }
    inline bool GridItem::is_triangle(void) const {
        return RT_GETFLAGS(RT_ITM_MTRI, RT_ITM_OTRI) != 0;
    }
    inline unsigned Triangle::orientation(void) const {
        return RT_GETFLAGS(RT_TRI_MORI, RT_TRI_OORI);
    }
    inline bool Triangle::is_degenerate(void) const {
        return RT_GETFLAGS(RT_TRI_MDEG, RT_TRI_ODEG) != 0;
    }
    inline bool Triangle::is_reversed(void) const {
        return RT_GETFLAGS(RT_TRI_MREV, RT_TRI_OREV) != 0;
    }
    inline void GridItem::set_last_ray_id(const unsigned val) {
        RT_SETFLAGS(RT_ITM_MRID, RT_ITM_ORID, val);
    }
    inline void GridItem::set_is_triangle(const bool val) {
        RT_SETFLAGS(RT_ITM_MTRI, RT_ITM_OTRI, val);
    }
    inline void Triangle::set_orientation(const unsigned val) {
        RT_SETFLAGS(RT_TRI_MORI, RT_TRI_OORI, val);
    }
    inline void Triangle::set_degenerate(const bool val) {
        RT_SETFLAGS(RT_TRI_MDEG, RT_TRI_ODEG, val);
    }
    inline void Triangle::set_reversed(const bool val) {
        RT_SETFLAGS(RT_TRI_MREV, RT_TRI_OREV, val);
    }

} // namespace rt

#endif /*SRTK_RT_H*/
