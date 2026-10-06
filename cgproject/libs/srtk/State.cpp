// state.cpp: ray tracing kernel state

#include <time.h>

#include <stdio.h>
#include "State.h"
/* #include "Grid.h" */

#ifndef OLD_OSTREAM
using namespace std;
#endif

namespace rt {

TriangleSetCreator* TriangleSet::c = 0;
State* rts = 0;

void rtInit(void)
{
  rts = new State;
}

void rtExit(void)
{
  delete rts;
}

void rtBeginWorld(void)
{
  rts->beginWorld();
}

void rtEndWorld(void)
{
  rts->endWorld();
}

void rtLoadIdentity(void)
{
  rts->loadIdentity();
}

void rtPushMatrix(void)
{
  rts->pushMatrix();
}

void rtPopMatrix(void)
{
  rts->popMatrix();
}

void rtLoadMatrix(const float* m4x4)
{
  rts->loadMatrix(m4x4);
}

void rtMultMatrix(const float* m4x4)
{
  rts->multMatrix(m4x4);
}

void rtTranslate(float *xyz)
{
  rts->translate(xyz);
}

void rtScale(float* xyz)
{
  rts->translate(xyz);
}

void rtRotate(float degrees, float* xyz)
{
  rts->rotate(degrees, xyz);
}

int rtMaterial(const float *emissivity, // (diffuse) light source intensity
		const float *diffuse,  // diffuse reflectivity
		const float *specular, // specular reflectivity
		const float shininess, // Phong exponent (not divided by 128 like in OpenGL), if larger than 128, the material is to be considered perfectly specular
		const float *transmissivity,  // (specular) transmittivity
		const float indexOfRefraction,
    void* client_data)
{
  return rts->material(emissivity, diffuse, specular, shininess, transmissivity, indexOfRefraction, client_data);
}

void rtBindMaterial(int material_id)
{
  rts->bindMaterial(material_id);
}

int rtTexture(int width, int height, int channels, const unsigned char* map, void* client_data)
{
  return rts->texture(width, height, channels, (unsigned char*)map, client_data);
}

void rtBindTexture(int texture_id)
{
  rts->bindTexture(texture_id);
}

void rtUnbindTexture(void)
{
  rts->unbindTexture();
}

int rtBeginTriangleSet(void* client_data)
{
  return rts->beginTriangleSet(client_data);
}

void rtEndTriangleSet(void)
{
  rts->endTriangleSet();
}

void rtVertex3(float *xyz)
{
  rts->current_triangleset->vertex(xyz);
}

void rtColor3(float *rgb)
{
  rts->current_triangleset->color(rgb);
}

void rtNormal3(float *xyz)
{
  rts->current_triangleset->normal(xyz);
}

void rtTexCoord3(float *xyz)
{
  rts->current_triangleset->texCoord(xyz);
}

void rtVertexData(void *data)
{
  rts->current_triangleset->vertexData(data);
}

void rtTriangleData(void *data)
{
  rts->current_triangleset->triangleData(data);
}

void Ray::set(const float* org, const float *dir, const float mindist, const float maxdist, unsigned flags)
{
  R3COPY(org, Ray::org);
  R3COPY(dir, Ray::dir);
  Ray::mindist = mindist;
  Ray::maxdist = maxdist;
  exclude[0] = exclude[1] = 0;   // don't exclude anything
  Ray::flags = flags;
}

void State::endWorld(void)
{
  if (current_triangleset)
    endTriangleSet();
  
#ifdef SRTK_USE_EMBREE

  // Embree backend: hand the finished triangle sets to Embree and let it
  // build the BVH. The cluster/grid hierarchy below is not built at all.
  embree = new EmbreeScene;
  embree->build(trianglesets);
  world_bounds = embree->bounds();

#else

  // count objects and triangles, make flat triangles linked list
  int objCount = 0, triCount = 0;
  list<Triangle*> *triList =0;  // chopped up by CreateClusterHierarchy
  for (int i=0; i<trianglesets->size; i++) {
    TriangleSet* triset = trianglesets->elem(i);;
    for (int j=0; j<triset->nrtris; j++)
      triList = triList->prepend(triset->triangles[j]);
    objCount++;
    triCount += triset->nrtris;
  }
//  cerr << "Got " << triCount << " triangles in " << objCount << " triangle sets.\n";
  if (!triList) {
    topCluster = new Cluster;
    grid = new Grid;
    world_bounds.init();
    return;
  }

  // build cluster hierarchy
//  cerr << "Building cluster hierarchy ... ";
  topCluster = CreateClusterHierarchy(triList);
//  cerr << "done.\n";
  
  // engrid clusters in hierarchical grid
//  cerr << "Building grid ... ";
  grid = new Grid;
  grid->engrid(topCluster);
//  cerr << "done.\n";
  world_bounds = topCluster->bounds;

#endif /* SRTK_USE_EMBREE */
}

void rtGetBounds(float* min, float* max)
{
  R3 m, M;
  rts->world_bounds.get_min(m);
  rts->world_bounds.get_max(M);
  R3COPY(m, min);
  R3COPY(M, max);
}

Texture** rtGetTextures(int *nrtextures)
{
  *nrtextures = rts->textures->size;
  return &rts->textures->elem(0);
}

Material** rtGetMaterials(int *nrmaterials)
{
  *nrmaterials = rts->materials->size;
  return &rts->materials->elem(0);
}

Surface** rtGetSurfaces(int *nrsurfaces)
{
  *nrsurfaces = rts->trianglesets->size;
  return (Surface**)&rts->trianglesets->elem(0);
}

float* rtGetBounds(void)
{
  return rts->topCluster->bounds.b;
}

void State::traverse(const Handler& handler)
{
  for (int i=0; i<trianglesets->size; i++) {
    TriangleSet* triset = trianglesets->elem(i);
    handler.surface(triset);
    for (int j=0; j<triset->nrtris; j++)
      handler.triangle(triset->triangles[j]);
  }
}

void rtTraverse(const Handler& handler)
{
  rts->traverse(handler);
}

unsigned State::increment_rayid(void)
{
  // TODO: get rid of this occasionally executed, but relatively expensive
  // resetting of ray IDs (executed every RT_MAX_RAY_ID, currently 128M,
  // queries).
  if ((++ray_id) >= RT_MAX_RAY_ID) {  // avoid errors due to overflow
    grid->reset_ray_ids();
    ray_id = 1;  // start over
  }
  return ray_id;
}

Hit* State::shootRay(Ray& ray, Hit* hit)
{
  ray.nr_bounding_box_tests = ray.nr_triangle_tests = 0;

#ifdef SRTK_USE_EMBREE

  // Embree does not need the mailboxing ray IDs, and deliberately does not
  // touch them: rtcIntersect1 / rtcOccluded1 leave the scene untouched. That
  // is what makes this backend safe to call from several threads at once,
  // and the uniform grid below not.
  if (ray.flags & RT_ANY_HIT) {
    // Shadow ray: any blocker will do, so take Embree's cheaper occlusion
    // path. Callers reach this through rtTestVisibility(), which only ever
    // compares the result against 0, so a minimal hit record is enough.
    if (!embree->occluded(ray))
      return 0;
    hit->dist = ray.maxdist;
    hit->s = hit->t = 0.f;
    hit->triangle = 0;
    R3COPY(ray.org, hit->point);
    return hit;
  }
  return embree->intersect(ray, hit);

#else

//  return topCluster->intersect(ray, hit);
  ray.id = increment_rayid();
  return grid->intersect(ray, hit);

#endif /* SRTK_USE_EMBREE */
}

void State::traverseBox(const Handler& handler, const Bounds& box)
{
#ifdef SRTK_USE_EMBREE
  // A grid-traversal convenience that nothing in the framework calls. There
  // is no Embree equivalent: use rtTraverse() to walk every triangle and
  // test it against the box yourself.
  (void)handler; (void)box;
  fprintf(stderr, "rtTraverseBox() is not available with the Embree backend; "
                  "use rtTraverse() instead.\n");
#else
  grid->traverseBox(handler, box, increment_rayid());
#endif
}

Hit *rtShootRay1(Ray& ray, Hit* hit)
{
  return rts->shootRay(ray, hit);
}

Hit* rtShootRay(const Ray& ray, Hit* hit)
{
  Ray myray = ray;   // copy ray (we change it during intersection testing)
  return rts->shootRay(myray, hit);
}

void rtTraverseBox(const Handler& handler, const float* const box)
{
  Bounds bbox(box);
  rts->traverseBox(handler, bbox);
}

}  // namespace rt

