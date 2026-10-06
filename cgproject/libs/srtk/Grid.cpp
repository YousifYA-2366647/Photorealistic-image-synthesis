// Grid.cpp: RayTracing acceleration using a uniform grid

#include <stdio.h>
#include <stdlib.h>

#include "Grid.h"

#include "rtio.h"

namespace rt {

  // Could be done with inheritance and virtual functions, but
  // we don't want virtual functions. They can't be inlined and
  // make our structs larger.
  inline Hit* GridItem::intersect(Ray& ray, Hit* h)
  {
    if (last_ray_id() == ray.id)
      return 0;   // not a new hit
    set_last_ray_id(ray.id);

    Hit *hit =0;
    if (is_triangle()) {
      hit = ((Triangle*)this)->intersect(ray, h);
      if (hit)
        ray.maxdist = hit->dist;
    } else {
      hit = ((Grid*)this)->intersect(ray, h);
    }

    return hit;
  }

  // Returns true if bounds are small w.r.t. grid voxel size
  inline bool Grid::is_small(const Bounds& ibounds) const
  {
    return ((ibounds[MAX_X] - ibounds[MIN_X]) <= voxsize[X] &&
      (ibounds[MAX_Y] - ibounds[MIN_Y]) <= voxsize[Y] &&
      (ibounds[MAX_Z] - ibounds[MIN_Z]) <= voxsize[Z]);
  }

  inline void Grid::get_cells(const Bounds& itembounds, short* c)
  {
    /* enlarge the bounds by a small amount in all directions */
    Bounds ibounds = itembounds;
    float xext = (bounds[MAX_X] - bounds[MIN_X]) * 1e-4, 
      yext = (bounds[MAX_Y] - bounds[MIN_Y]) * 1e-4, 
      zext = (bounds[MAX_Z] - bounds[MIN_Z]) * 1e-4;
    ibounds[MIN_X] -= xext; ibounds[MAX_X] += xext;
    ibounds[MIN_Y] -= yext; ibounds[MAX_Y] += yext;
    ibounds[MIN_Z] -= zext; ibounds[MAX_Z] += zext;

    c[MIN_X] = x2voxel(ibounds[MIN_X]);
    if (c[MIN_X] >= xsize) c[MIN_X] = xsize-1;
    if (c[MIN_X] < 0) c[MIN_X] = 0;
    c[MAX_X] = x2voxel(ibounds[MAX_X]);
    if (c[MAX_X] >= xsize) c[MAX_X] = xsize-1;
    if (c[MAX_X] < 0) c[MAX_X] = 0;

    c[MIN_Y] = y2voxel(ibounds[MIN_Y]);
    if (c[MIN_Y] >= ysize) c[MIN_Y] = ysize-1;
    if (c[MIN_Y] < 0) c[MIN_Y] = 0;
    c[MAX_Y] = y2voxel(ibounds[MAX_Y]);
    if (c[MAX_Y] >= ysize) c[MAX_Y] = ysize-1;
    if (c[MAX_Y] < 0) c[MAX_Y] = 0;

    c[MIN_Z] = z2voxel(ibounds[MIN_Z]);
    if (c[MIN_Z] >= zsize) c[MIN_Z] = zsize-1;
    if (c[MIN_Z] < 0) c[MIN_Z] = 0;
    c[MAX_Z] = z2voxel(ibounds[MAX_Z]);
    if (c[MAX_Z] >= zsize) c[MAX_Z] = zsize-1;
    if (c[MAX_Z] < 0) c[MAX_Z] = 0;
  }

  // Adds the item with given bounds
  void Grid::add_item(struct GridItem* item, const Bounds& itembounds)
  {
    short cells[6];
    get_cells(itembounds, cells);

    for (short a=cells[MIN_X]; a<=cells[MAX_X]; a++) {
      for (short b=cells[MIN_Y]; b<=cells[MAX_Y]; b++) {
        for (short c=cells[MIN_Z]; c<=cells[MAX_Z]; c++) {
          GridItemList* *cell_items = (GridItemList**)&items[cell_index(a,b,c)];
          (*cell_items) = (*cell_items)->prepend(item);
        }
      }
    }
  }

  inline void Grid::add_triangle(Triangle* tri)
  {
    Bounds tribounds(tri);
    add_item(tri, tribounds);
  }

  inline void Grid::add_triangles(list<Triangle*> *tril)
  {
    while (tril) {
      add_triangle(tril->data);
      tril = tril->next;
    }
  }

  inline void Grid::add_grid(Grid* grid)
  {
    add_item(grid, grid->bounds);
  }

  void Grid::add_cluster(Cluster* clus)
  {
    // TODO: use more sophisticated heuristics.
    if (is_small(clus->bounds) && clus->nrtriangles > 4) {
      // Small cluster with more than a few triangles.
      // Create a new grid for the triangles (includes bounding box)
      // Chances are good that the overhead of bounding box testing
      // and grid tracing setup pay off.
      Grid *subgrid = new Grid;
      subgrid->engrid(clus, level+1);
      add_grid(subgrid);
    } else {
      add_triangles(clus->triangles);  // add triangles in the cluster
      for (int i=0; i<8; i++)          // add children clusters
        if (clus->children[i])
          add_cluster(clus->children[i]);
    }
  }

  void Grid::engrid(Cluster* clus, int level)
  {
    // copy bounds and enlarge a tiny bit, compute grid dimensions from it
    bounds = clus->bounds;
    double xext = (bounds[MAX_X] - bounds[MIN_X]);
    double yext = (bounds[MAX_Y] - bounds[MIN_Y]); 
    double zext = (bounds[MAX_Z] - bounds[MIN_Z]);

    double s = xext + yext + zext;
    if (s < EPSILON) {
      xsize = ysize = zsize = (short)pow((double)clus->nrtriangles, 0.33333333) + 1;
    } else {
      double a = xext / s;
      double b = yext / s;
      double c = zext / s;
      bool xflat=false, yflat=false, zflat=false;
      double root = 3.;
      if (a < 0.01) { xflat = true; a = 1.; root--; }
      if (b < 0.01) { yflat = true; b = 1.; root--; }
      if (c < 0.01) { zflat = true; c = 1.; root--; }
      double t = pow((double)clus->nrtriangles / (a*b*c), 1./root);
      xsize = xflat ? 1 : (short)ceil(a * t);
      ysize = yflat ? 1 : (short)ceil(b * t);
      zsize = zflat ? 1 : (short)ceil(c * t);
    }

    xext *= 1e-4; yext *= 1e-4; zext *= 1e-4;
    bounds[MIN_X] -= xext; bounds[MAX_X] += xext;
    bounds[MIN_Y] -= yext; bounds[MAX_Y] += yext;
    bounds[MIN_Z] -= zext; bounds[MAX_Z] += zext;

    // compute voxel size
    voxsize[X] = (bounds[MAX_X] - bounds[MIN_X]) / (float)xsize;
    voxsize[Y] = (bounds[MAX_Y] - bounds[MIN_Y]) / (float)ysize;
    voxsize[Z] = (bounds[MAX_Z] - bounds[MIN_Z]) / (float)zsize;

    // allocate and initialize grid item list array
    int numcells = xsize*ysize*zsize;
    items = new GridItemList* [numcells];
    for (int i=0; i<numcells; i++)
      items[i] = 0;

    Grid::level = level;
//    fprintf(stderr, "Engridding %d triangles in level %d grid of size %dx%dx%d ... ", 
//      clus->nrtriangles, level, (int)xsize, (int)ysize, (int)zsize);
    add_cluster(clus);
  }
  
  void Grid::reset_ray_ids(void)
  {
    set_last_ray_id(0);
    for (int i=0; i<xsize*ysize*zsize; i++) {
      GridItemList* p = items[i];
      while (p) {
	GridItem *item = p->data;
        item->set_last_ray_id(0);
        if (!item->is_triangle())
          ((Grid*)item)->reset_ray_ids();
	p = p->next;
      }
    }
  }

  /* Compute t0, ray's minimal intersection with the whole grid and
   * position P of this intersection. Returns TRUE if the grid bounds are 
   * intersected and FALSE if the ray passes along the voxel grid. */
  inline bool Grid::intersect_bounds(Ray& ray, float& t0, R3& P)
  {
    t0 = ray.mindist;
    R3ADDSCALED(ray.org, t0, ray.dir, P);
    if (bounds.is_out(P)) {
      float tmax;
      if (!bounds.intersect(ray, t0, tmax))
        return false;
      R3ADDSCALED(ray.org, t0, ray.dir, P);
    }
    return true;
  }

  /* initializes grid tracing */
  inline void Grid::intersect_setup(const Ray& ray, const float t0, const R3& P,
                             I3& g, R3& tDelta, R3& tNext, I3& step, I3& out)
  {
    /* Compute the grid cell g where this intersection occurs */
    g[X] = x2voxel(P[X]); if (g[X] >= xsize) g[X] = xsize-1;
    g[Y] = y2voxel(P[Y]); if (g[Y] >= ysize) g[Y] = ysize-1;
    g[Z] = z2voxel(P[Z]); if (g[Z] >= zsize) g[Z] = zsize-1;

    /* Setup X: */
    /* tDelta[X] is the distance increment along the ray to the adjacent 
    * voxel in X direction.
    * tNext[X] is the total distance from the ray origin to the next voxel
    * in X direction.
    * step[X] is either +1 or -1 accroding to the ray X direction. 
    * out[X] is -1 or grid[X]size: the first x grid cell index outside the 
    * grid. */
    if (ray.dir[X] > EPSILON) {
      tDelta[X] = voxsize[X] / ray.dir[X];
      tNext[X] = t0 + (voxel2x(g[X]+1) - P[X]) / ray.dir[X];
      step[X] = 1; out[X] = xsize;
    } else if (ray.dir[X] < -EPSILON) {
      tDelta[X] = voxsize[X] / -ray.dir[X];
      tNext[X] = t0 + (voxel2x(g[X]) - P[X]) / ray.dir[X];
      step[X] = out[X] = -1;
    } else {
      tDelta[X] = 0.;
      tNext[X] = HUGE;
    }

    /* Setup Y: */
    if (ray.dir[Y] > EPSILON) {
      tDelta[Y] = voxsize[Y] / ray.dir[Y];
      tNext[Y] = t0 + (voxel2y(g[Y]+1) - P[Y]) / ray.dir[Y];
      step[Y] = 1; out[Y] = ysize;
    } else if (ray.dir[Y] < -EPSILON) {
      tDelta[Y] = voxsize[Y] / -ray.dir[Y];
      tNext[Y] = t0 + (voxel2y(g[Y]) - P[Y]) / ray.dir[Y];
      step[Y] = out[Y] = -1;
    } else {
      tDelta[Y] = 0.;
      tNext[Y] = HUGE;
    }

    /* Setup Z: */
    if (ray.dir[Z] > EPSILON) {
      tDelta[Z] = voxsize[Z] / ray.dir[Z];
      tNext[Z] = t0 + (voxel2z(g[Z]+1) - P[Z]) / ray.dir[Z];
      step[Z] = 1; out[Z] = zsize;
    } else if (ray.dir[Z] < -EPSILON) {
      tDelta[Z] = voxsize[Z] / -ray.dir[Z];
      tNext[Z] = t0 + (voxel2z(g[Z]) - P[Z]) / ray.dir[Z];
      step[Z] = out[Z] = -1;
    } else {
      tDelta[Z] = 0.;
      tNext[Z] = HUGE;
    }
  }

  /* Advances to the next grid cell. Assumes setup with GridTraceSetup(). 
  * returns FALSE if the current voxel was the last voxel in the grid intersected 
  * by the ray */
  inline bool Grid::next_voxel(float& t0, I3& g, R3& tNext, 
                        const R3& tDelta, const I3& step, const I3& out)
  {
    bool ingrid = true;
    if (tNext[X] <= tNext[Y] && tNext[X] <= tNext[Z]) { /* tNext[X] is smallest */
      g[X] += step[X];
      t0 = tNext[X];
      tNext[X] += tDelta[X];
      ingrid = (g[X] != out[X]);
    } else if (tNext[Y] <= tNext[Z]) {		         /* tNext[Y] is smallest */
      g[Y] += step[Y];
      t0 = tNext[Y];
      tNext[Y] += tDelta[Y];
      ingrid = (g[Y] != out[Y]);
    } else {					         /* tNext[Z] is smallest */
      g[Z] += step[Z];
      t0 = tNext[Z];
      tNext[Z] += tDelta[Z];
      ingrid = (g[Z] != out[Z]);
    }
    return ingrid;
  }

  /* finds the nearest intersection of the ray with an item (GEOM or PATCH) in
  * a voxel's item list. If there is an intersection, maxdist will contain
  * the distance to the intersection point measured from the ray origin
  * as usual. If there is no intersection, maxdist remains unmodified. */
  inline Hit* Grid::intersect_voxel(GridItemList *items, Ray& ray, Hit* h)
  {
    Hit *hit = 0;    
    while (items) {
      if (items->data->intersect(ray, h))
        hit = h;
      items = items->next;
    }
    return hit;
  }

  /* traces a ray through a voxel grid. Returns nearest intersection or null */
  Hit* Grid::intersect(Ray& ray, Hit* h)
  {
    R3 tNext, tDelta, P;
    I3 step, out, g;
    float t;

    if (!intersect_bounds(ray, t, P))
      return 0;

    intersect_setup(ray, t, P, g, tDelta, tNext, step, out);

    Hit *hit = 0;
    do {
      int cidx = cell_index(g[X],g[Y],g[Z]);
      if (!valid_index(cidx)) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": invalid cell index.\n";
        return 0;
      }
      if (intersect_voxel(items[cidx], ray, h))
        hit = h;
    } while (next_voxel(t, g, tNext, tDelta, step, out) && t <= ray.maxdist);
    return hit;
  }

  inline void GridItem::traverseBox(const Handler& handler, const Bounds& box, const unsigned id)
  {
    if (last_ray_id() != id) {
      set_last_ray_id(id);
      
      if (is_triangle())
	handler.triangle((Triangle*)this);
      else
	((Grid*)this)->traverseBox(handler, box, id);
    }
  }

  void Grid::traverseBox(const Handler& handler, const Bounds& box, const unsigned id)
  {
    if (bounds.is_disjunct(box))   // Grid bounding box and query box don't overlap
      return;

    short cells[6];
    get_cells(box, cells);         // determine grid cells overlapping the query box
    
    for (short a=cells[MIN_X]; a<=cells[MAX_X]; a++) {
      for (short b=cells[MIN_Y]; b<=cells[MAX_Y]; b++) {
        for (short c=cells[MIN_Z]; c<=cells[MAX_Z]; c++) {
          GridItemList* p = items[cell_index(a,b,c)];  // traverse item list of cell
          while (p) {
	    p->data->traverseBox(handler, box, id);
	    p = p->next;
	  }
	}
      }
    }
  }

}
