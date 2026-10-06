// material.c

#include "rt.h"
#include "R3.h"

namespace rt {

#define R3ISZERO(col) (col[0]==0. && col[1]==0. && col[2]==0.)

  void Material::set(const float* emissivity,
	     const float* diffuse,
	     const float* specular, const float shininess,
	     const float* transmissivity, const float indexOfRefraction,
       void* client_data)
  {
    R3COPY(emissivity, Material::emissivity);
    R3COPY(diffuse, Material::diffuse);
    R3COPY(specular, Material::specular);
    R3COPY(transmissivity, Material::transmissivity);
    Material::shininess = shininess;
    Material::indexOfRefraction = indexOfRefraction;
    Material::data = client_data;

    flags.is_light = !R3ISZERO(emissivity);
    flags.is_diffuse_reflector = !R3ISZERO(diffuse);
    flags.is_specular_reflector = !R3ISZERO(specular);
    flags.is_mirror = shininess>128.;
    flags.is_transparent = !R3ISZERO(transmissivity);
  }

}  // namespace rt
