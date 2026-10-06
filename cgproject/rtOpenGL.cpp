// rtOpenGL.cpp: Renders RT scene graph using OpenGL

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>    // <GL/gl.h> needs this first on Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "srtk/rt.h"
#include "rtOpenGL.h"
#include <iostream>

using namespace rt;

// loads rt texture into OpenGL
// uses rt::Texture.data member to store OpenGL texture id
static void LoadTexture(Texture* texture) {
    GLuint texid;

    GLint formats[5] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);

    gluBuild2DMipmaps(GL_TEXTURE_2D, texture->channels, texture->width,
            texture->height, formats[texture->channels], GL_UNSIGNED_BYTE,
            texture->map);

    uintptr_t tex = texid;

    texture->data = (ClientData)tex;
}

static void SetTexture(Texture* texture) {
    static Texture* current_texture = 0;

    if(texture == current_texture)
        return; // avoid unnecessary state changes

    glEnd();  // close glBegin(GL_TRIANGLES);

    if(!texture)
        glDisable(GL_TEXTURE_2D);
    else {
        // The id was stored as a value, not as a pointer to one.
        GLuint texid = (GLuint)(uintptr_t)texture->data;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texid);
    }

    current_texture = texture;

    glBegin(GL_TRIANGLES);
}

static void SetMaterial(Material* mat) {
    static Material* current_material = 0;

    if(mat == current_material)
        return;   // avoid unnecessary state changes

    glEnd();  // close glBegin(GL_TRIANGLES);

    GLfloat a[4] = {0.2, 0.2, 0.2, 1.0}, d[4], s[4], e[4];
    d[0] = mat->diffuse[0];
    d[1] = mat->diffuse[1];
    d[2] = mat->diffuse[2];
    d[3] = 1.;
    s[0] = mat->specular[0];
    s[1] = mat->specular[1];
    s[2] = mat->specular[2];
    s[3] = 1.;
    e[0] = mat->emissivity[0];
    e[1] = mat->emissivity[1];
    e[2] = mat->emissivity[2];
    e[3] = 1.;
    float maxe = (e[0] > e[1] && e[0] > e[2] && e[0] > e[3]) ? e[0] :
        ((e[1] > e[2] && e[1] > e[3]) ? e[1] : ((e[2] > e[3]) ? e[2] : e[3]));

    if(maxe > 0.) {
        e[0] /= maxe;
        e[1] /= maxe;
        e[2] /= maxe;
        e[3] /= maxe;
    }

    glMaterialfv(GL_FRONT, GL_AMBIENT, a);
    glMaterialfv(GL_FRONT, GL_EMISSION, e);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
    glMaterialfv(GL_FRONT, GL_SPECULAR, s);
    glMaterialf(GL_FRONT, GL_SHININESS, mat->shininess);

    glBegin(GL_TRIANGLES);
}

static void RenderSurface(Surface* surf) {
    SetTexture(surf->texture);
    SetMaterial(surf->material);
}

// Renders a rt::Triangle
static void RenderTriangle(Triangle* t) {
    for(int v = 0; v < 3; v++) {
        R3* vcoord = t->getCoords();
        R3* vnorm = t->getNormals();
        R3* vtco = t->getTexCoords();

        if(vnorm) glNormal3fv((GLfloat*) vnorm[v]);

        if(vtco)  glTexCoord3fv((GLfloat*) vtco[v]);

        glVertex3fv((GLfloat*) vcoord[v]);
    }
}

// Renders rt scene graph with OpenGL
void rtRenderWithOpenGL(void) {
    // save texture client data and load textures into OpenGL
    // We use texture client data to store OpenGL texture id.
    int i, nrtextures;
    Texture** textures = rtGetTextures(&nrtextures);
    ClientData* saved_data = new ClientData [nrtextures];

    for(i = 0; i < nrtextures; i++) {
        saved_data[i] = textures[i]->data;
        LoadTexture(textures[i]);
    }

    Handler action;
    action.triangle = RenderTriangle;
    action.surface = RenderSurface;

    std::cout << std::endl;
    glDisable(GL_TEXTURE_2D);   // SetTexture assumes textures are initially off
    glBegin(GL_TRIANGLES);
    rtTraverse(action); // Crash in here.
    glEnd();

    // restore texture client data
    for(i = 0; i < nrtextures; i++)
        textures[i]->data = saved_data[i];

    delete [] saved_data;
}

