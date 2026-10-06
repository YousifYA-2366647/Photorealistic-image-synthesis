// Texture.cpp

#include <math.h>
#include "rt.h"
#include "R3.h"

namespace rt {

  void Texture::set(int width, int height, int channels, unsigned char* map, void* client_data)
  {
    Texture::width = width;
    Texture::height = height;
    Texture::channels = channels;
    Texture::map = map;
    Texture::data = client_data;
  }
  
  // TODO: implement bilinear interpolation.
  float* Texture::lookup(const float u, const float v)
  {
    float x = (u-floorf(u))*width;
    float y = (v-floorf(v))*height;
    float i = floorf(x);
    float j = floorf(y);
    const unsigned char* pix = lookup_raw((int)i, (int)j);

    // The looked-up value goes in thread-local scratch rather than in the
    // Texture object: several threads may be shading against the same
    // texture at once. (Texture::color / Texture::alpha are kept in sync
    // for any code that reads those members directly.)
    static thread_local R3 result;
    R3SET(result, (float)pix[0] * (1./255.),
                  (float)pix[1] * (1./255.),
                  (float)pix[2] * (1./255.));
    R3COPY(result, color);
    alpha = (channels == 4) ? (float)pix[3] * (1./255) : 1.;
    return &result[0];
  }

} // namespace rt
