// EmbreeScene.h: Intel Embree ray traversal backend for srtk.
//
// This replaces Cluster.cpp + Grid.cpp + Triangle::intersect() as the thing
// that answers "what does this ray hit?". It is deliberately hidden behind
// the same rtShootRay() / rtTestVisibility() functions declared in rt.h, so
// that nothing above the kernel -- neither main.cpp nor the model loaders --
// can tell which backend is in use. Build with -DUSE_EMBREE=OFF to go back to
// the original uniform grid, or to plug in an acceleration structure of your
// own at exactly this seam.
//
// Two details worth knowing if you read the implementation:
//
//  * srtk and Embree use the same barycentric convention. srtk's hit->s and
//    hit->t weight vertices 1 and 2 (see PINT in R3.h); Embree's hit.u and
//    hit.v do the same. They map across one-to-one, no conversion.
//
//  * srtk does not normalise Ray::dir, and expresses hit distances in units
//    of |dir| -- shadow rays rely on this, using maxdist = 1 to mean "at the
//    light". Embree's tnear/tfar are in the same parametrisation, so this
//    also carries over untouched.

#ifndef SRTK_EMBREESCENE_H
#define SRTK_EMBREESCENE_H

#ifdef SRTK_USE_EMBREE

// Embree 4 is preferred; Embree 3 (e.g. Ubuntu 22.04's libembree-dev 3.12)
// is also supported. EmbreeScene.cpp handles the API differences.
#if __has_include(<embree4/rtcore.h>)
#include <embree4/rtcore.h>
#else
#include <embree3/rtcore.h>
#endif

#include "rt.h"
#include "array.h"
#include "Bounds.h"

namespace rt {

    struct TriangleSet;

    class EmbreeScene {
    public:
        EmbreeScene();
        ~EmbreeScene();

        // Builds the BVH from the finished triangle sets. Called once from
        // State::endWorld().
        void build(array<TriangleSet*>* trianglesets);

        // Nearest hit. Returns 'hit' (filled in) or 0.
        Hit* intersect(Ray& ray, Hit* hit);

        // Any hit -- used for shadow rays. Returns true if something blocks
        // the ray. This is Embree's dedicated occlusion path and is markedly
        // cheaper than asking for the nearest hit and throwing it away.
        bool occluded(Ray& ray);

        // World bounding box of the built scene.
        const Bounds& bounds(void) const {
            return world_bounds;
        }

        bool is_built(void) const {
            return scene != 0;
        }

        // (geomID, primID) -> the srtk Triangle that Embree just hit.
        Triangle* triangle(unsigned geomID, unsigned primID) const;

    private:
        RTCDevice device;
        RTCScene scene;

        // Indexed by Embree geomID.
        TriangleSet** sets;
        unsigned nrsets;

        Bounds world_bounds;

        // True if any triangle in the scene is degenerate. srtk skips those
        // when intersecting, which we can only reproduce inside the filter
        // function, so their presence forces the filter on for every ray.
        bool has_degenerate;

        // Does this ray need the intersection filter at all?
        inline bool needs_filter(const Ray& ray) const;

        EmbreeScene(const EmbreeScene&);
        EmbreeScene& operator=(const EmbreeScene&);
    };

} // namespace rt

#endif /* SRTK_USE_EMBREE */

#endif /* SRTK_EMBREESCENE_H */
