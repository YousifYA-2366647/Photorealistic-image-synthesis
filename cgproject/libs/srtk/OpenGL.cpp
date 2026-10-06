// OpenGL.h
#ifdef RT_OPENGL

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

#include "rt.h"
#include "State.h"
#include <iostream>

namespace rt {

    void SetMaterial(Material* mat) {
        GLfloat a[4] = {0.2, 0.2, 0.2, 1.0}, d[4], s[4];
        d[0] = mat->diffuse[0]; d[1] = mat->diffuse[1]; d[2] = mat->diffuse[2]; d[3] = 1.;
        s[0] = mat->specular[0]; s[1] = mat->specular[1]; s[2] = mat->specular[2]; s[3] = 1.;
        glMaterialfv(GL_FRONT, GL_AMBIENT, a);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
        glMaterialfv(GL_FRONT, GL_SPECULAR, s);
        glMaterialf(GL_FRONT, GL_SHININESS, mat->shininess);
    }

    void SetTexture(Texture* texture) {
        std::cout << "wut" << std::endl;
        if(!texture) {
            glDisable(GL_TEXTURE_2D);
            return;
        }

        if(texture->opengl_id == -1) {
            // not yet loaded before
            GLuint texid;

            GLint formats[5] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glGenTextures(1, &texid);
            glBindTexture(GL_TEXTURE_2D, texid);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            gluBuild2DMipmaps(GL_TEXTURE_2D, texture->channels, texture->width, texture->height, formats[texture->channels], GL_UNSIGNED_BYTE, texture->map);

            texture->opengl_id = texid;
        }

        GLuint texid = texture->opengl_id;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texid);
    }

    void RenderTriangle(Triangle* t) {
        for(int v = 0; v < 3; v++) {
            if(t->vnorm) glNormal3fv((GLfloat*) t->vnorm[v]);

            if(t->vtco)  glTexCoord3fv((GLfloat*) t->vtco[v]);

            glVertex3fv((GLfloat*) t->vcoord[v]);
        }
    }

    void RenderTriangleArray(array<Triangle*>* triangles) {
        glBegin(GL_TRIANGLES);

        for(int i = 0; i < triangles->size; i++) {
            Triangle* t = (*triangles)[i];
            RenderTriangle(t);
        }

        glEnd();
    }

    void TriangleSet::renderWithOpenGL(void) {
        RenderTriangleArray(triangles);
    }

    void RenderTriangleList(list<Triangle*>* triList) {
        list<Triangle*>* tri = triList;
        glBegin(GL_TRIANGLES);

        while(tri) {
            RenderTriangle(tri->data);
            tri = tri->next;
        }

        glEnd();
    }

    // for debugging cluster hierarchy:
    static int cdepth = 0;
    void RenderCluster(Cluster* clus) {
        //    RenderTriangleList(clus->triangles);

        if(cdepth == 0)
            glDisable(GL_LIGHTING);

        glColor3f(0.5, 1, 0.5);
        float* b = clus->bounds.b;
        glBegin(GL_LINE_LOOP);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MIN_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MIN_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MAX_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MAX_Y], b[Bounds::MIN_Z]);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MIN_Y], b[Bounds::MAX_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MIN_Y], b[Bounds::MAX_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MAX_Y], b[Bounds::MAX_Z]);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MAX_Y], b[Bounds::MAX_Z]);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MIN_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MIN_Y], b[Bounds::MAX_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MIN_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MIN_Y], b[Bounds::MAX_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MAX_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MAX_X], b[Bounds::MAX_Y], b[Bounds::MAX_Z]);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MAX_Y], b[Bounds::MIN_Z]);
        glVertex3f(b[Bounds::MIN_X], b[Bounds::MAX_Y], b[Bounds::MAX_Z]);
        glEnd();

        cdepth++;

        for(int i = 0; i < 8; i++)
            if(clus->children[i]) RenderCluster(clus->children[i]);

        cdepth--;

        if(cdepth == 0)
            glEnable(GL_LIGHTING);
    }

    void State::renderWithOpenGL(void) {
        Material* mat = &default_material;
        Texture* texture = 0;
        SetMaterial(mat);
        SetTexture(texture);

        for(int i = 0; i < trianglesets->size; i++) {
            TriangleSet* triset = (*trianglesets)[i];

            if(triset->texture != texture) {
                texture = triset->texture;
                SetTexture(texture);
            }

            if(triset->mat != mat) {
                mat = triset->mat;
                SetMaterial(mat);
            }

            triset->renderWithOpenGL();
        }

        RenderCluster(topCluster);
    }

} // namespace rt
#endif /*RT_OPENGL*/
