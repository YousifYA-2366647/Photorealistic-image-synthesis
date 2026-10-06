// EmbreeScene.cpp: Intel Embree ray traversal backend for srtk.

#ifdef SRTK_USE_EMBREE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "EmbreeScene.h"
#include "R3.h"
#include "TriangleSet.h"

// Embree 4 moved the filter callback out of the query context into a
// separate arguments struct and renamed the context. Paper over that here.
#if RTC_VERSION_MAJOR >= 4
typedef RTCRayQueryContext EmbreeQueryContext;
#define embreeInitQueryContext rtcInitRayQueryContext
#define EMBREE_SCENE_FLAG_QUERY_FILTER RTC_SCENE_FLAG_FILTER_FUNCTION_IN_ARGUMENTS
#else
typedef RTCIntersectContext EmbreeQueryContext;
#define embreeInitQueryContext rtcInitIntersectContext
#define EMBREE_SCENE_FLAG_QUERY_FILTER RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION
#endif

namespace rt {

    //-------------------------------------------------------------------------
    // Per-query payload.
    //
    // srtk rays carry two things Embree knows nothing about: a list of
    // triangles to skip (Ray::exclude, the framework's cure for shadow acne)
    // and a front/back face mask (RT_FRONT / RT_BACK). Both are enforced in an
    // intersection filter, which Embree calls for every candidate hit before
    // accepting it. We hang the originating srtk ray off the query context so
    // the filter can see them.
    //-------------------------------------------------------------------------
    struct QueryContext {
        EmbreeQueryContext base;   // must be first
        const Ray* ray;
        const EmbreeScene* self;
    };

    static void filterFunction(const RTCFilterFunctionNArguments* args) {
        // We only ever issue single rays, so N == 1 throughout.
        if(args->N != 1 || args->valid[0] != -1)
            return;

        const QueryContext* ctx = (const QueryContext*)args->context;
        const Ray* ray = ctx->ray;

        RTCHitN* hits = args->hit;
        const unsigned geomID = RTCHitN_geomID(hits, 1, 0);
        const unsigned primID = RTCHitN_primID(hits, 1, 0);

        Triangle* tri = ctx->self->triangle(geomID, primID);
        if(!tri) return;

        // Same rejections, in the same order, as Triangle::intersect().
        if(tri->is_degenerate() ||
           ray->exclude[0] == tri || ray->exclude[1] == tri) {
            args->valid[0] = 0;
            return;
        }

        const double d = R3DOTPRODUCT(tri->normal, ray->dir);
        if(d > EPSILON) {           // back facing
            if(!(ray->flags & RT_BACK)) args->valid[0] = 0;
        } else if(d < -EPSILON) {   // front facing
            if(!(ray->flags & RT_FRONT)) args->valid[0] = 0;
        } else {                    // edge on
            args->valid[0] = 0;
        }
    }

    // The filter costs a callback per candidate hit, so only ask for it when
    // the ray actually needs one. A primary ray in a scene without degenerate
    // triangles needs nothing.
    inline bool EmbreeScene::needs_filter(const Ray& ray) const {
        return has_degenerate ||
               ray.exclude[0] != 0 || ray.exclude[1] != 0 ||
               (ray.flags & RT_FRONT_AND_BACK) != RT_FRONT_AND_BACK;
    }

    //-------------------------------------------------------------------------

    static void errorFunction(void*, enum RTCError code, const char* str) {
        fprintf(stderr, "Embree error %d: %s\n", (int)code, str ? str : "");
    }

    EmbreeScene::EmbreeScene() {
        device = 0;
        scene = 0;
        sets = 0;
        nrsets = 0;
        has_degenerate = false;
        world_bounds.init();
    }

    EmbreeScene::~EmbreeScene() {
        if(scene)  rtcReleaseScene(scene);
        if(device) rtcReleaseDevice(device);
        delete [] sets;
    }

    Triangle* EmbreeScene::triangle(unsigned geomID, unsigned primID) const {
        if(geomID >= nrsets || !sets[geomID])
            return 0;
        TriangleSet* set = sets[geomID];
        if((int)primID >= set->nrtris)
            return 0;
        return set->triangles[primID];
    }

    void EmbreeScene::build(array<TriangleSet*>* trianglesets) {
        has_degenerate = false;

        device = rtcNewDevice(NULL);
        if(!device) {
            fprintf(stderr, "Could not create Embree device (error %d)\n",
                    (int)rtcGetDeviceError(NULL));
            exit(1);
        }
        rtcSetDeviceErrorFunction(device, errorFunction, NULL);

        scene = rtcNewScene(device);

        // Static scene, traced many times: spend the extra build time.
        rtcSetSceneBuildQuality(scene, RTC_BUILD_QUALITY_HIGH);

        // Ray::exclude and the front/back mask are implemented with an
        // intersection filter passed per query, which Embree only honours if
        // the scene opts in.
        rtcSetSceneFlags(scene, EMBREE_SCENE_FLAG_QUERY_FILTER);

        const int nrtrianglesets = trianglesets ? trianglesets->size : 0;

        // geomID is assigned by Embree; size the lookup table generously and
        // index it by the id we are handed back.
        nrsets = (unsigned)(nrtrianglesets > 0 ? nrtrianglesets : 1);
        sets = new TriangleSet* [nrsets];
        for(unsigned i = 0; i < nrsets; i++) sets[i] = 0;

        int totaltris = 0;

        for(int i = 0; i < nrtrianglesets; i++) {
            TriangleSet* set = trianglesets->elem(i);
            if(!set || set->nrtris <= 0)
                continue;   // Embree rejects empty geometries

            RTCGeometry geom =
                rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

            // srtk stores an unshared triangle soup: triangle j owns vertices
            // vidx..vidx+2 of the surface's vcoord array. Copy it into an
            // Embree vertex buffer with the 16-byte stride Embree prefers.
            float* verts = (float*)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
                4 * sizeof(float), (size_t)set->nrverts);

            unsigned* idx = (unsigned*)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
                3 * sizeof(unsigned), (size_t)set->nrtris);

            if(!verts || !idx) {
                fprintf(stderr, "Embree: could not allocate geometry buffers\n");
                exit(1);
            }

            for(int v = 0; v < set->nrverts; v++) {
                verts[4 * v + 0] = set->vcoord[v][0];
                verts[4 * v + 1] = set->vcoord[v][1];
                verts[4 * v + 2] = set->vcoord[v][2];
                verts[4 * v + 3] = 0.0f;   // padding

                world_bounds.enlarge(set->vcoord[v]);
            }

            for(int t = 0; t < set->nrtris; t++) {
                const unsigned v0 = (unsigned)set->triangles[t]->vidx;
                idx[3 * t + 0] = v0;
                idx[3 * t + 1] = v0 + 1;
                idx[3 * t + 2] = v0 + 2;
            }

            for(int t = 0; !has_degenerate && t < set->nrtris; t++)
                if(set->triangles[t]->is_degenerate())
                    has_degenerate = true;

            rtcCommitGeometry(geom);
            const unsigned geomID = rtcAttachGeometry(scene, geom);
            rtcReleaseGeometry(geom);   // the scene holds a reference now

            if(geomID >= nrsets) {
                // Should not happen with sequential attachment, but grow
                // rather than scribble past the end.
                TriangleSet** bigger = new TriangleSet* [geomID + 1];
                for(unsigned k = 0; k < geomID + 1; k++) bigger[k] = 0;
                for(unsigned k = 0; k < nrsets; k++) bigger[k] = sets[k];
                delete [] sets;
                sets = bigger;
                nrsets = geomID + 1;
            }
            sets[geomID] = set;
            totaltris += set->nrtris;
        }

        rtcCommitScene(scene);

        // NOTE: world_bounds is accumulated above from the vertices rather
        // than taken from rtcGetSceneBounds(). Embree reports the BVH root
        // box, which it pads slightly; home() derives the initial camera
        // from these bounds, so using the padded box would frame the scene
        // differently than the uniform-grid backend does and the two
        // backends would not produce comparable images.

        printf("Embree: built BVH over %d triangles in %d geometries%s.\n",
               totaltris, nrtrianglesets,
               has_degenerate ? " (scene contains degenerate triangles)" : "");
    }

    Hit* EmbreeScene::intersect(Ray& ray, Hit* hit) {
        alignas(16) RTCRayHit rh;
        rh.ray.org_x = ray.org[0];
        rh.ray.org_y = ray.org[1];
        rh.ray.org_z = ray.org[2];
        rh.ray.dir_x = ray.dir[0];
        rh.ray.dir_y = ray.dir[1];
        rh.ray.dir_z = ray.dir[2];
        rh.ray.tnear = ray.mindist;
        rh.ray.tfar  = ray.maxdist;
        rh.ray.mask  = 0xFFFFFFFF;
        rh.ray.flags = 0;
        rh.ray.time  = 0.0f;
        rh.ray.id    = ray.id;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rh.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        QueryContext ctx;
        embreeInitQueryContext(&ctx.base);
        ctx.ray = &ray;
        ctx.self = this;

#if RTC_VERSION_MAJOR >= 4
        RTCIntersectArguments args;
        rtcInitIntersectArguments(&args);
        args.context = &ctx.base;
        if(needs_filter(ray))
            args.filter = filterFunction;

        rtcIntersect1(scene, &rh, &args);
#else
        if(needs_filter(ray))
            ctx.base.filter = filterFunction;

        rtcIntersect1(scene, &ctx.base, &rh);
#endif

        if(rh.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            return 0;

        Triangle* tri = triangle(rh.hit.geomID, rh.hit.primID);
        if(!tri)
            return 0;

        hit->dist = rh.ray.tfar;
        // Embree's (u,v) weight vertices 1 and 2, exactly like srtk's (s,t).
        hit->s = rh.hit.u;
        hit->t = rh.hit.v;
        hit->triangle = tri;
        R3ADDSCALED(ray.org, (double)rh.ray.tfar, ray.dir, hit->point);

        // Callers of rtShootRay1() read these back for statistics.
        ray.maxdist = rh.ray.tfar;

        return hit;
    }

    bool EmbreeScene::occluded(Ray& ray) {
        alignas(16) RTCRay r;
        r.org_x = ray.org[0];
        r.org_y = ray.org[1];
        r.org_z = ray.org[2];
        r.dir_x = ray.dir[0];
        r.dir_y = ray.dir[1];
        r.dir_z = ray.dir[2];
        r.tnear = ray.mindist;
        r.tfar  = ray.maxdist;
        r.mask  = 0xFFFFFFFF;
        r.flags = 0;
        r.time  = 0.0f;
        r.id    = ray.id;

        QueryContext ctx;
        embreeInitQueryContext(&ctx.base);
        ctx.ray = &ray;
        ctx.self = this;

#if RTC_VERSION_MAJOR >= 4
        RTCOccludedArguments args;
        rtcInitOccludedArguments(&args);
        args.context = &ctx.base;
        if(needs_filter(ray))
            args.filter = filterFunction;

        rtcOccluded1(scene, &r, &args);
#else
        if(needs_filter(ray))
            ctx.base.filter = filterFunction;

        rtcOccluded1(scene, &ctx.base, &r);
#endif

        // Embree signals "occluded" by setting tfar to -inf.
        return r.tfar < 0.0f;
    }

} // namespace rt

#endif /* SRTK_USE_EMBREE */
