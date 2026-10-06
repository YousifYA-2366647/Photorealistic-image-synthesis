/* Cluster.cpp: cluster hierarchy: sorts triangles by size and
 * location.
 *
 * References: Per Christensen, PhD Thesis "Hierarchical Techniques for Glossy
 *	Global Illumination", Univ. of Washington, 1995, p116-117 */

// Could be made faster by
// - precomputing and storing triangle bounding box size and center
// - precomputing and storing cluster bounding box size

#include "Cluster.h"

namespace rt {

/* No clusters are created with less than this number of patches. */
#define MIN_TRIANGLES 3

  Cluster::Cluster()
  {
    R3SET(mid, 0., 0., 0.);
    triangles = 0;
    nrtriangles = 0;
    for (int i=0; i<8; i++)
      children[i] = 0;
  }

  /* Adds a patch to a cluster. The bounding box is enlarged if necessary, but
   * the midpoint is not updated (it's more efficient to do that once, after all
   * patches have been added to the cluster and the bounding box is fully 
   * determined). */
  void Cluster::addTriangle(TriangleList *tri)
  {
    triangles = triangles->prepend(tri);
    nrtriangles++;
    bounds.enlarge(tri->data);
  }

  // remove triangle (list element) prevfl = predecessor of fl. fl is to be removed
  void Cluster::removeTriangle(TriangleList *fl, TriangleList *prevfl)
  {
    if (fl == triangles)
      triangles = triangles->next;
    else
      prevfl->next = fl->next;
    //    nrtriangles--;
  }

  /* Checks the size of fl->patch w.r.t. the bounding box of the cluster clus. If
   * the size of the patch is more than half the size of the cluster, the routine
   * returns. If the patch is smaller than half the size of the cluster, the position of
   * its centroid is tested w.r.t. the centroid of the cluster. If the centroids 
   * coincide, the routine returns. If not, fl is moved to the patch list of a subcluster
   * of clus. Which subcluster depends on the position of the patch w.r.t. the centroid
   * of clus. Returns TRUE if the patch pointed to by fl was moved to the subcluster.
   * Returns FALSE if the patch was not moved. prevfl is the PATCHLIST element preceeding
   * fl (chasing pointers!), needed to be able to efficiently remove fl from the
   * patchlist of clus. */
  bool Cluster::checkMoveTriangle(TriangleList *fl, TriangleList *prevfl)
  {
    R3 csz, fsz, fc;         // cluster size, face size and face center
    bounds.get_size(csz);    // cluster size
    Bounds fbx(fl->data);    // triangle bounding box
    fbx.get_size(fsz);       // triangle size
    fbx.get_center(fc);      // triangle (bounds) center
    
    /* if the patch is larger than an octant, return */
    if ((fsz[0] > 10*EPSILON && fsz[0] > csz[0] * 0.5) ||
	(fsz[1] > 10*EPSILON && fsz[1] > csz[1] * 0.5) ||
	(fsz[2] > 10*EPSILON && fsz[2] > csz[2] * 0.5))
      return false;

    /* check the position of the centroid of the boundingbox of the patch w.r.t. the
     * centroid of the cluster. If the centroid coincides with
     * the center of the current cell, the %8 makes that the
     * triangle will be moved to subcluster with index 0. */
    int idx = R3Compare(mid, fc, 0.) % 8;
    
    /* move the patch to the subcluster with index subi */
    removeTriangle(fl, prevfl);
    children[idx]->addTriangle(fl);
    
    return true;		/* fl was moved to the subcluster */
  }

  /* Splits a cluster into subclusters: it is assumed that clus is a cluster without
   * subclusters yet. If clus is NULL or there are no more than MIN_PATCHES_IN_CLUSTER 
   * patches in the cluster, the routine simply returns. If there are more patches, 8 
   * subclusters are created for the cluster. Then for each patch, if the patch
   * is smaller than an octant, the patch is moved to the octant containing its
   * centroid. Otherwise, the patch remains a direct child of the cluster. Finally,
   * subclusters with zero patches are disposed off and the procedure recursively repeated
   * for each subcluster. */
  void Cluster::split(void)
  {
    /* don't split the cluster if it contains too few patches. */
    if (nrtriangles < MIN_TRIANGLES)
      return;

    /* Create eight subclusters for the cluster with initialized bounding box. */
    int i;
    for (i=0; i<8; i++)
      children[i] = new Cluster;
    
    /* check and possibly move each of the patches in the cluster to a subcluster */
    TriangleList *fl, *next, *prev;
    fl = triangles; prev = 0;
    while (fl) {
      next = fl->next;	/* fl can be moved to a subcluster, after which fl->next will
			 * point to the patches already present in the subcluster. */
      if (!checkMoveTriangle(fl, prev))
	prev = fl;	/* fl was not moved to a subcluster. If fl is moved,
			 * prev remains the same. */
      fl = next;
    }

    /* dispose of subclusters containing no patches, call SplitCluster recursively for
     * non empty subclusters */
    for (i=0; i<8; i++) {
      if (!children[i]->triangles) {
	delete children[i];
	children[i] = 0;
      } else {
	children[i]->bounds.get_center(children[i]->mid);
	children[i]->split();
      }
    }
  }

  /* Initialises top-level cluster, containing all the triangles. */
  void Cluster::init(TriangleList *tri)
  {
    triangles = tri;
    while (tri) {
      nrtriangles++;
      bounds.enlarge(tri->data);
      tri = tri->next;
    }
    bounds.get_center(mid);
  }

  /* Creates a hierarchical model of the discretised scene (the patches 
   * in the scene) using the simple algorithm described in
   * - Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
   *   PhD Thesis, University of Washington, 1995, p 116 
   * This hierarchy is often much more efficient for tracing rays and
   * clustering radiosity algorithms than the given hierarchy of
   * bounding boxes. A pointer to the toplevel "cluster" is returned. */
  Cluster* CreateClusterHierarchy(TriangleList *trilist)
  {
    Cluster *top = new Cluster;
    top->init(trilist);
    top->split();
    return top;
  }

  Hit* Cluster::intersect(Ray& ray, Hit* hit)
  {
    R3 norg;
    R3ADDSCALED(ray.org, ray.mindist, ray.dir, norg);
    if (bounds.is_out(norg) && !bounds.intersect(ray))
      return 0;

    bool got_hit = false;

    // intersect with the triangles in this cell
    TriangleList *tri = triangles;
    while (tri) {
      if (tri->data->intersect(ray, hit)) {
		ray.maxdist = hit->dist;   // reduce max distance
		if (ray.flags & RT_ANY_HIT)
		return hit;
		got_hit	= true;
      }
      tri = tri->next;
    }

    // check children cluster cells
    // TODO: traverse children cells in better order
    for (int i=0; i<8; i++) {
      got_hit |= (children[i] && children[i]->intersect(ray, hit));
    }

    return got_hit ? hit : 0;
  }

} // namespace rt
