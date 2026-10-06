// TriangleSet.h : RT TriangleSet rep.

#ifndef _RT_TRIANGLESET_H_
#define _RT_TRIANGLESET_H_

#include "array.h"

#include "rt.h"
#include "R3.h"
#include "Vec3.h"
#include "Transform.h"

namespace rt {

  // Auxiliary class used while building a triangle set. Building 
  // a TriangleSet requires a lot of temporary data, grouped in
  // this class, and thrown away after the TriangleSet is complete.
  struct TriangleSetCreator {
    array<Triangle*> *triangles;   // smart arrays
    array<Vec3> *vcoord, *vcol, *vnorm, *vtco;
    array<ClientData> *vdata;

    // have_xxx is true if xxx vertex attrib is available, 
    // and false if not: determined by what is available at
    // the moment the first vertex of the triangle set is defined.
    bool have_vnorm, have_vcol, have_vtco, have_vdata; 

    // Temporary buffers for holding vertex normal, color, texture
    // coordinate, vertex client data and triangle client data.
    // These values are copied to the vertex arrays when a vertex
    // coordinate arrives.
    Vec3 vcoord_buf, vnorm_buf, vcol_buf, vtco_buf;
    ClientData vdata_buf;
    ClientData tridata_buf;

    // current vertex index
    int vidx;

    // Surface being defined
    Surface* surface;

    TriangleSetCreator()
    {
      triangles = 0;
      vcoord = 0;
      vcol = 0;
      vnorm = 0;
      vtco = 0;
      vdata = 0;
    }

    ~TriangleSetCreator()
    {
      // do not delete the triangles pointed to in the array
      delete triangles;
      delete vcoord;
      delete vnorm;
      delete vcol;
      delete vtco;
      delete vdata;
    }

    void init(Surface* surface)
    {
      triangles = new array<Triangle*>;
      vcoord = new array<Vec3>;
      vcol = new array<Vec3>;
      vnorm = new array<Vec3>;
      vtco = new array<Vec3>;
      vdata = new array<ClientData>;

      have_vnorm = have_vcol = have_vtco = have_vdata = false;
      tridata_buf = 0;
      vidx = 0;

      TriangleSetCreator::surface = surface;
    }

    void close_triangle(bool reversed)
    {
      Triangle* tri = new Triangle;
      tri->data = tridata_buf;
      tri->surface = surface;
      tri->vidx = vidx-2;    // also works for triangle strips this way
      tri->set_reversed(reversed); // will be closed later
      triangles->append(tri);
    }

    void vertex(const Vec3& coord)
    {
      vidx = vcoord->append(coord);
      if (have_vcol)  vcol->append(vcol_buf);
      if (have_vnorm) vnorm->append(vnorm_buf);
      if (have_vtco)  vtco->append(vtco_buf);
      if (have_vdata) vdata->append(vdata_buf);

      // for triangle strips, we should close a triangle for every
      // vertex after the first 2, with alternating reversed flag.
      if (vidx%3 == 2) close_triangle(false);
    }

    void normal(const Vec3& norm)
    {
      if (vidx==0) have_vnorm = true;
      vnorm_buf = norm;
    }

    void color(float* rgb)
    {
      if (vidx==0) have_vcol = true;
      vcol_buf.set(rgb);
    }

    void texCoord(float* uvw)
    {
      if (vidx==0) have_vtco = true;
      vtco_buf.set(uvw);
    }

    void vertexData(void* data)
    {
      if (vidx==0) have_vdata = true;
      vdata_buf = data;
    }

    void triangleData(void* data)
    {
      tridata_buf = data;
    }
  };

  struct TriangleSet: public Surface {
  	static TriangleSetCreator *c;  // TODO: not thread safe

    TriangleSet()
    {
      triangles = 0;   // all inherited from Surface
      vcoord = 0;
      vcol = 0;
      vnorm = 0;
      vtco = 0;
      vdata = 0;
  	  texture = 0;
	    material = 0;
	    transform = 0;
	    data = 0;
      nrtris = nrverts = 0;
    }

    void init(Material* mat, Texture* texture, Transform *xform, void* client_data)
    {
      TriangleSet::material = mat;
      TriangleSet::texture = texture;
      TriangleSet::transform = xform;
	    TriangleSet::data = client_data;

      c = new TriangleSetCreator;
      c->init(this);
    }

    void close(void)
    {
      if (c->triangles->size > 0) {
        // copy data from creator and make sure data will not be
        // deleted when deleting the creator below
        nrtris = c->triangles->size; nrverts = c->vcoord->size;
        vcoord    = (R3*)c->vcoord->s   ; c->vcoord->s = 0;
        if (c->have_vcol ) { vcol  = (R3*)c->vcol->s ; c->vcol->s  = 0; }
        if (c->have_vnorm) { vnorm = (R3*)c->vnorm->s; c->vnorm->s = 0; }
        if (c->have_vtco ) { vtco  = (R3*)c->vtco->s ; c->vtco->s  = 0; }
        if (c->have_vdata) { vdata = c->vdata->s; c->vdata->s = 0; }

        triangles = c->triangles->s; c->triangles->s = 0;
        for (int i=0; i<nrtris; i++)
          triangles[i]->close();
      }
      delete c; c=0;
    }

    void vertex(float* xyz)
    {
      Vec3 vcoord_buf;
      transform->apply_affine_to_point(xyz, vcoord_buf.v);
      c->vertex(vcoord_buf);
    }

    void normal(float* xyz)
    {
      Vec3 vnorm_buf;
      transform->apply_affine_to_normal(xyz, vnorm_buf.v);
      c->normal(vnorm_buf);
    }

    void color(float* rgb)
    {
      c->color(rgb);
    }

    void texCoord(float* uvw)
    {
      c->texCoord(uvw);
    }

    void vertexData(void* data)
    {
      c->vertexData(data);
    }

    void triangleData(void* data)
    {
      c->triangleData(data);
    }

    ~TriangleSet()
    {
      for (int i=0; i<nrtris; i++) delete triangles[i];
      delete [] triangles;
      delete [] vcoord;
      delete [] vcol;
      delete [] vnorm;
      delete [] vtco;
      delete [] vdata;
    }
  };

}  // namespace rt

#endif /* _RT_TRIANGLESET_H_ */
