// Grid.h: Ray Tracing acceleration with uniform grid

#ifndef _RT_GRID_H_
#define _RT_GRID_H_

#include "rt.h"
#include "R3.h"
#include "Cluster.h"

namespace rt {

  // pointers to cell items are kept in a linked linear list rather than in
  // an array. Arrays would be more compact, but it is more involved to
  // remove items (dyanmic scenes!)
  typedef list<GridItem*> GridItemList;

  struct Grid: public GridItem {
    short xsize, ysize, zsize;  // number of grid cells in each dimension
    short level;                // to make this struct 48 bytes
    R3 voxsize;                 // grid cell size
    Bounds bounds;              // bounding box
    GridItemList** items;       // 3D array of pointers to voxel item lists

    Grid()
    {
      xsize = ysize = zsize = level = 0;
      R3SET(voxsize,0,0,0);
      bounds.init();
      items = 0;
      set_last_ray_id(0);
      set_is_triangle(false);
    }

    // engrid cluster of triangles
    void engrid(Cluster* clus, int level =0);

    // converts voxel position to index in items array
    inline int cell_index(int a, int b, int c) const
    {
      return ((a * ysize + b) * zsize + c);
    }

    inline bool valid_index(int cindex)
    {
      return (cindex>=0 && cindex<xsize*ysize*zsize);
    }

    // converts coordinate to voxel position along X,Y,Z
    inline int x2voxel(const float x) const
    {
      return (int)((voxsize[X]<EPSILON) ? 0 : (x - bounds[MIN_X]) / voxsize[X]);
    }
    inline int y2voxel(const float y) const
    {
      return (int)((voxsize[Y]<EPSILON) ? 0 : (y - bounds[MIN_Y]) / voxsize[Y]);
    }
    inline int z2voxel(const float z) const
    {
      return (int)((voxsize[Z]<EPSILON) ? 0 : (z - bounds[MIN_Z]) / voxsize[Z]);
    }

    // least X|Y|Z coordinate in voxel at given position along X|Y|Z
    inline float voxel2x(const int px) const
    {
      return px * voxsize[X] + bounds[MIN_X];
    }
    inline float voxel2y(const int py) const
    {
      return py * voxsize[Y] + bounds[MIN_Y];
    }
    inline float voxel2z(const int pz) const
    {
      return pz * voxsize[Z] + bounds[MIN_Z];
    }

    // Returns true if bounds are small w.r.t. grid voxel size
    bool is_small(const Bounds& bounds) const;

    // Computes cells intersected by the itembounds (fills in 6 shorts:
    // min|max X|Y|Z
    void get_cells(const Bounds& itembounds, short* cells);

    // Adds the item with given bounds
    void add_item(GridItem* item, const Bounds& itembounds);
    void add_triangle(Triangle* tri);
    void add_triangles(list<Triangle*> *trilist);
    void add_cluster(Cluster* clus);
    void add_grid(Grid* grid);

    ~Grid()
    {
      for (int i=0; i<xsize*ysize*zsize; i++) {
	GridItemList* p=items[i];
	while (p) {
	  GridItemList* next = p->next;
	  delete p;
	  p = next;
	}
      }
      delete [] items;
    }

    Hit* intersect(Ray& ray, Hit* hit);

    typedef int I3[3];
    bool intersect_bounds(Ray& ray, float& t0, R3& P);
    void intersect_setup(const Ray& ray, const float t0, const R3& P,
                         I3& g, R3& tDelta, R3& tNext, I3& step, I3& out);
    bool next_voxel(float& t0, I3& g, R3& tNext, 
                    const R3& tDelta, const I3& step, const I3& out);
    Hit* intersect_voxel(GridItemList* items, Ray& ray, Hit* h);

    // resets all ray ids of items in the grid to zero.
    void reset_ray_ids(void);

    // traverse all items overlapping the given box. 'id' plays the role of the
    // ray id for ray tracing: it prevents traversing the same item multiple times.
    void traverseBox(const Handler& handler, const Bounds& box, const unsigned id);
  };

} // namespace rt

#endif /* _RT_GRID_H_ */
