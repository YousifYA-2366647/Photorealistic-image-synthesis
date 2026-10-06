// State.h: ray tracing kernel state - private header file

#ifndef RT_STATE_H
#define RT_STATE_H

#include <stdlib.h>
#ifdef OLD_OSTREAM
#include <ostream.h>
#else
#include <iostream>
#endif
#include <math.h>

// seems not always to be defined under windows ...
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "array.h"
#include "stack.h"

#include "rt.h"
#include "R3.h"
#include "Transform.h"
#include "TriangleSet.h"

#include "Cluster.h"
#include "Grid.h"
#include "EmbreeScene.h"

namespace rt {

  struct State {
    // world data structures
    array<Material*> *materials;
    array<Texture*> *textures;
    array<TriangleSet*> *trianglesets;

    Material default_material;
    Material *current_material;
    Texture *current_texture;
    TriangleSet *current_triangleset;
    
    stack<Transform> *xf;

    Cluster* topCluster;
    Grid* grid;

#ifdef SRTK_USE_EMBREE
    EmbreeScene* embree;
#endif
    // World bounding box, filled in by endWorld() whichever backend is used.
    Bounds world_bounds;

    unsigned ray_id;

    State()
    {
      materials = 0;
      textures = 0;
      trianglesets = 0;

      current_material = 0;
      current_texture = 0;
      current_triangleset = 0;

      xf = 0;
      
      topCluster = 0;   // init'ed by endWorld()
      grid = 0;         // also init'ed by endWorld()
#ifdef SRTK_USE_EMBREE
      embree = 0;       // also init'ed by endWorld()
#endif
      world_bounds.init();

      ray_id = 0;
    }

    void beginWorld(void)
    {
      materials = new array<Material*>;
      textures = new array<Texture*>;
      trianglesets = new array<TriangleSet*>;

      R3 emissivity = {0,0,0};
      R3 diffuse = {0.8,0.8,0.8};
      R3 specular = {0,0,0};
      R3 transmissivity = {0,0,0};
      default_material.set(emissivity, diffuse, specular, 0., transmissivity, 1.);

      materials->append(&default_material); // becomes current
      current_material = &default_material;
      current_texture = 0;
      current_triangleset = 0;

      // create transform stack and push identity transform to it
      xf = new stack<Transform>;
      Transform id; id.set_identity();
      xf->push(id);
    }

    void loadIdentity(void)
    {
      xf->top().set_identity();
    }

    void loadMatrix(const float* m4x4)
    {
      xf->top().set_transpose(m4x4);
    }

    void multMatrix(const float* m4x4)
    {
      Transform f; f.set_transpose(m4x4);
      xf->top() = xf->top().compose(f);
    }

    void pushMatrix(void)
    {
      xf->push(xf->top());
    }

    void popMatrix(void)
    {
      xf->pop();
    }

    void scale(float *xyz)
    {
      xf->top().scale(xyz);
    }

    void translate(float *xyz)
    {
      xf->top().translate(xyz);
    }

    void rotate(float degrees, float *xyz)
    {
      xf->top().rotate(degrees*M_PI/180., xyz);
    }

    int material(const float *emissivity, // (diffuse) light source intensity
		const float *diffuse,  // diffuse reflectivity
		const float *specular, // specular reflectivity
		const float shininess, // Phong exponent (not divided by 128 like in OpenGL), if larger than 128, the material is to be considered perfectly specular
		const float *transmissivity,  // (specular) transmittivity
		const float indexOfRefraction,
		void *client_data)
    {
      current_material = new Material;
      current_material->set(emissivity, diffuse, specular, shininess, transmissivity, indexOfRefraction, client_data);
      return materials->append(current_material);
    }

    void bindMaterial(int material_id)
    {
      current_material = (*materials)[material_id];
    }

    int texture(int width, int height, int channels, unsigned char* map, void* client_data)
    {
      current_texture = new Texture;
      current_texture->set(width, height, channels, map, client_data);
      return textures->append(current_texture);
    }

    void bindTexture(int texture_id)
    {
      current_texture = (*textures)[texture_id];
    }

    void unbindTexture(void)
    {
      current_texture = 0;
    }

    int beginTriangleSet(void* client_data)
    {
      current_triangleset = new TriangleSet;
      current_triangleset->init(current_material, current_texture, &xf->top(), client_data);
      return trianglesets->append(current_triangleset);      
    }

    void endTriangleSet(void)
    {
      if (!current_triangleset) return;
      current_triangleset->close();
      current_triangleset = 0;
    }

    void endWorld(void);

    unsigned increment_rayid(void);
    struct Hit* shootRay(struct Ray& ray, struct Hit* hit);
    void traverse(const Handler& handler);
    void traverseBox(const Handler& handler, const Bounds& box);

    ~State()
    {
      int i;
      for (i=0; i<materials->size; i++) delete (*materials)[i];
      delete materials;
      for (i=0; i<textures->size; i++) delete (*textures)[i];
      delete textures;
      for (i=0; i<trianglesets->size; i++) delete (*trianglesets)[i];
      delete trianglesets;
      delete xf;
#ifdef SRTK_USE_EMBREE
      delete embree;
#endif
    }

  };

  extern State* rts;

} //namespace rt

#endif /*RT_STATE_H*/
