// Cluster.h: triangle cluster hierarchy: sorts triangles by
// size and location.

#ifndef _RT_CLUSTER_H_
#define _RT_CLUSTER_H_

#include "rt.h"
#include "R3.h"
#include "Bounds.h"
#include "list.h"

namespace rt {

  typedef list<Triangle*> TriangleList;

  /* group of triangles of similar size and location */
  struct Cluster {
    Bounds bounds;		/* bounding box for the cluster */
    R3 mid;			/* midpoint of the bounding box */
    TriangleList *triangles; /* list of patches in this cluster */
    int nrtriangles;            /* nr of triangles in this cluster and its children */
    Cluster *children[8];	/* 8 subclusters: clusters form an octree */

    Cluster();

    // trilist gets chopped up by split
    void init(TriangleList *trilist);
    void split(void);

    Hit* intersect(Ray& ray, Hit* hit);

    protected:
    void addTriangle(TriangleList *tri);
    void removeTriangle(TriangleList *fl, TriangleList *prevfl);
    bool checkMoveTriangle(TriangleList *fl, TriangleList *prevfl);
  };

  extern Cluster* CreateClusterHierarchy(TriangleList *trilist);

} // namespace rt

#endif /* _RT_CLUSTER_H_ */
