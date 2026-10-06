#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <cmath>   // std::isfinite (the portable spelling of BSD finite())

#include "rtIndexedFaceSet.h"
#include "srtk/rt.h"

#ifndef EPSILON
#define EPSILON 1e-6
#endif

namespace rt {

    IndexedFaceSet::IndexedFaceSet() {
        color = 0;
        coord = 0;
        normal = 0;
        texCoord = 0;
        ccw = true;
        colorIndex.init(0, 0);
        colorPerVertex = true;
        convex = true;
        coordIndex.init(0, 0);
        creaseAngle = 0.;
        normalIndex.init(0, 0);
        normalPerVertex = true;
        solid = true;
        texCoordIndex.init(0, 0);
        id = "IndexedFaceSet";
    }

    const char* IndexedFaceSet::name(void) const {
        return id;
    }

    void ifs_renderer::begin_faces(IndexedFaceSet* f) {
        ifs_renderer::f = f;
        rtBeginTriangleSet();
    }

    void ifs_renderer::begin_face(int id, int nverts) {
        (void) id;
        (void) nverts;
    }

    void ifs_renderer::face_normal(int id, const vec3& n) {
        (void) id;
        if(f->normal)
            rtNormal3(n.ptr());
    }

    void ifs_renderer::face_color(int id, const vec3& c) {
        (void) id;
        if(f->color)
            rtColor3(c.ptr());
    }

    void ifs_renderer::vertex_normal(int id, const vec3& n) {
        (void) id;
        if(f->normal)
            rtNormal3(n.ptr());
    }

    void ifs_renderer::vertex_color(int id, const vec3& c) {
        (void) id;
        if(f->color)
            rtColor3(c.ptr());
    }

    void ifs_renderer::vertex_texCoord(int id, const vec2& t) {
        (void) id;
        if(f->texCoord)
            rtTexCoord2(t[0], t[1]);
    }

    void ifs_renderer::vertex_coord(int id, const vec3& p) {
        (void) id;
        rtVertex3(p.ptr());
    }

    void ifs_renderer::end_face(int id, int nverts) {
        (void) id;
        (void) nverts;
    }

    void ifs_renderer::end_faces(IndexedFaceSet*) {
        rtEndTriangleSet();
    }

    // Count the number of faces in the current geometry
    int ifs_tesselator::count_faces(void) {
        int nrfaces = 0;

        for(int k = 0; k < f->coordIndex.size; k = next_face(k))
            nrfaces++;

        return nrfaces;
    }

    // Returns first coordinate index of next face, or an out of bound index if
    // there is no next face.
    int ifs_tesselator::next_face(int k) {
        while(k < f->coordIndex.size && f->coordIndex[k] >= 0)
            k++;

        return (k + 1);  // skip end-of-face marker
    }

    // Computes normal of current face.
    // Newells method (see e.g. Tampieri, Graphics Gems III p.231) with one
    // modifications to make it more robust: polygon is translated to the
    // origin. This method works for both convex and
    // non-convex polygons with arbitrary number of vertices. If the
    // polygon is non-planar, a "best-fit" normal is returned. (There
    // should be no non-planar polygons in VRML geometries!)
    // Pre-requisite: vertex indices need to be valid.
    // Return true is normal is OK, false if something is wrong.
    bool ifs_tesselator::compute_face_normal(vec3* normal) {
        if(current_face.size < 3)   // error message is issued in check_face()
            return false;

        DVec3 n(0, 0, 0);
        //  vec3& o = (*f->coord)[0];     // origin
        vec3& o = (*f->coord)[current_face[0]];     // origin
        vec3& p = (*f->coord)[current_face[current_face.size - 1]];
        DVec3 prev, cur(p.X() - o.X(), p.Y() - o.Y(), p.Z() - o.Z());

        for(int j = 0; j < current_face.size; j++) {
            prev = cur;
            vec3& p = (*f->coord)[current_face[j]];
            cur = DVec3(p.X() - o.X(), p.Y() - o.Y(), p.Z() - o.Z());
            n.x += (prev.y - cur.y) * (prev.z + cur.z);
            n.y += (prev.z - cur.z) * (prev.x + cur.x);
            n.z += (prev.x - cur.x) * (prev.y + cur.y);
        }

        double norm = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        n.x /= norm; n.y /= norm; n.z /= norm;

        if(!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
            *normal = vec3(0, 0, 0);
            return false;
        }

        *normal = vec3(n.x, n.y, n.z);
        return true;
    }

    void ifs_tesselator::prepare_face_normal(const int k, const int facenr) {
        current_face.size = 0;

        for(int l = k; l < f->coordIndex.size; l++) {
            int i = f->coordIndex[l];

            if(i < 0) break;

            if(i >= f->coord->size) continue;

            current_face.append(i);

            if(f->creaseAngle > 0.)(*vert)[i].faces.append(facenr);
        }

        compute_face_normal(&(*fnorm)[facenr]);
    }

    // Prepares for automatic normal generation: computes vertex sharing
    // information in 'vert' and face normals for all faces in 'fnorm'.
    void ifs_tesselator::prepare_normals(void) {
        fnorm = 0;
        vert = 0;
        cosCreaseAngle = -1;

        if(f->normal || !renderer->normals_required)
            // we have normals or no normals are required
            return;

        int nrfaces = count_faces();
        fnorm = new array<vec3>(nrfaces);
        vert = new array<vertshare>(f->coord->size);
        cosCreaseAngle = cos(f->creaseAngle);

        int facenr = 0;

        for(int k = 0; k < f->coordIndex.size; k = next_face(k)) {
            prepare_face_normal(k, facenr);
            facenr++;
        }
    }

    // Vertex normals cannot be simply precomputed since the normal to be used
    // may be different in different faces sharing the vertex!
    const vec3 ifs_tesselator::gen_normal(const int vertnr, const int facenr) {
        if(!fnorm || !vert) {
            fprintf(stderr, "ifs_tesselator::gen_normal: automatic normal gene"
                    "ration improperly prepared");
            exit(-1);
        }

        vec3& nf = (*fnorm)[facenr];
        struct vertshare& v = (*vert)[vertnr];

        if(f->creaseAngle <= 0.)
            // no vertex sharing
            return nf;

        vec3 vnorm = nf;
        int ncnt = 1;

        for(int j = 0; j < v.faces.size; j++) {
            int k = v.faces[j];
            vec3 n = (*fnorm)[k];

            if(k != facenr && cosCreaseAngle <= (n & (vec3)nf)) {
                vnorm += n;
                ncnt++;
            }
        }

        if(ncnt > 1) vnorm.normalize();

        return vnorm;
    }

    // Prepare for automatic texture coordinate generation:
    // compute minimum x,y,z, longest side of bounding box and indices sidx and
    // tidx relating texture coordinates s and t to 3D world coordinates x, y
    // or z.
    void ifs_tesselator::prepare_texCoords(IndexedFaceSet* f) {
        min = vec3(0, 0, 0);
        sidx = tidx = -1;
        side = 0.;

        if(f->texCoord || !renderer->texcoords_required)
            return;

        fprintf(stderr, "ifs_tesselator::prepare_texCoords: not yet implemente"
                "d.\n");
#ifdef NEVER
        class BoundingBox bounds = f->get_bounds();
        min = bounds.min;
        vec3 size = bounds.size();

        side = size.X(); sidx = 1;

        if(size.Y() > side) {
            side = size.Y();
            sidx = 2;
        }

        if(size.Z() > side) {
            side = size.Z();
            sidx = 3;
        }

        float mmin = size.X(); tidx = 1;

        if(size.Y() <= mmin) {
            mmin = size.Y();
            tidx = 2;
        }

        if(size.Z() <= mmin) {
            mmin = size.Z();
            tidx = 3;
        }

        tidx = 6 - tidx - sidx;
#endif
    }

    const vec2 ifs_tesselator::gen_texCoord(const int vertnr) {
        (void) vertnr;
#ifdef NEVER

        if(sidx <= 0 || tidx <= 0) {
            fprintf(stderr, "ifs_tesselator::gen_texCoord: automatic texture c"
                    "oordinate generation improperly prepared\n");
            exit(-1);
        }

        vec3 d = vec3((*f->coord)[vertnr]) - min;
        vec2 t;
        t.s = (sidx == 1 ? d.X() : (sidx == 2 ? d.Y() : d.Z())) / side;
        t.t = (tidx == 1 ? d.X() : (tidx == 2 ? d.Y() : d.Z())) / side;

        if(!std::isfinite(t.s) || !std::isfinite(t.t))
            t = vec2(0, 0);

        return t;
#endif
        return vec2(0, 0);
    }

    int ifs_tesselator::get_normal(const array<int>* idx, const int i) {
        int k = i;

        if(idx) {
            if(idx->size <= i) {
                fprintf(stderr, "%s: normalIndex array too short (no %dth ind"
                        "ex)\n", f->name(), i);
                return -1;
            }

            k = (*idx)[i];
        }

        if(k < 0 || f->normal->size <= k) {
            fprintf(stderr, "%s: normal index %d out of range", f->name(), k);
            return -1;
        }

        return k;
    }

    int ifs_tesselator::get_color(const array<int>* idx, const int i) {
        int k = i;

        if(idx) {
            if(idx->size <= i) {
                fprintf(stderr, "%s: colorIndex array too short (no %dth ind"
                        "ex)\n", f->name(), i);
                return -1;
            }

            k = (*idx)[i];
        }

        if(k < 0 || f->coord->size <= k) {
            fprintf(stderr, "%s: color index %d out of range", f->name(), k);
            return -1;
        }

        return k;
    }

    int ifs_tesselator::get_texCoord(const array<int>* idx, const int i) {
        int k = i;

        if(idx) {
            if(idx->size <= i) {
                fprintf(stderr, "%s: texCoordIndex array too short (no %dth i"
                        "ndex)\n", f->name(), i);
                return -1;
            }

            k = (*idx)[i];
        }

        if(k < 0 || f->texCoord->size <= k) {
            fprintf(stderr, "%s: texCoord index %d out of range", f->name(), k);
            return -1;
        }

        return k;
    }

    void ifs_tesselator::do_face_normal(const int facenr) {
        if(!renderer->normals_required)
            return;

        if(f->normal && !f->normalPerVertex) {    // per face normals given
            int idx;

            if(f->normalIndex.size > 0)
                idx = get_normal(&f->normalIndex, facenr);
            else
                idx = get_normal(0, facenr);

            if(idx >= 0) renderer->face_normal(idx, (*f->normal)[idx]);
        }
    }

    void ifs_tesselator::do_face_color(const int facenr) {
        if(!renderer->colors_required)
            return;

        if(f->color && !f->colorPerVertex) {  // per face colors given
            int idx;

            if(f->colorIndex.size > 0)
                idx = get_color(&f->colorIndex, facenr);
            else
                idx = get_color(0, facenr);

            if(idx >= 0) renderer->face_color(idx, (*f->color)[idx]);
        }
    }

    void ifs_tesselator::do_vertex_normal(const int k, const int facenr) {
        if(!renderer->normals_required)
            return;

        if(!f->normal) {         // automatically generate vertex normal
            int i = f->coordIndex[k];
            vec3 n = gen_normal(i, facenr);
            renderer->vertex_normal(-1, n);
        } else if(f->normalPerVertex) {   // vertex normals given
            int idx;

            if(f->normalIndex.size > 0)
                idx = get_normal(&f->normalIndex, k);
            else
                idx = get_normal(&f->coordIndex, k);

            if(idx >= 0) renderer->vertex_normal(idx, (*f->normal)[idx]);
        }
    }

    void ifs_tesselator::do_vertex_color(const int k) {
        if(!renderer->colors_required)
            return;

        if(f->color && f->colorPerVertex) {   // set vertex color
            int idx;

            if(f->colorIndex.size > 0)
                idx = get_color(&f->colorIndex, k);
            else
                idx = get_color(&f->coordIndex, k);

            if(idx >= 0) renderer->vertex_color(idx, (*f->color)[idx]);
        }
    }

    void ifs_tesselator::do_texCoord(const int k) {
        if(!renderer->texcoords_required)
            return;

        if(!f->texCoord) {     // automatically generate texture coordinate
            int i = f->coordIndex[k];
            vec2 t = gen_texCoord(i);
            renderer->vertex_texCoord(-1, t);
        } else {            // texture coordinates given
            int idx;

            if(f->texCoordIndex.size > 0)
                idx = get_texCoord(&f->texCoordIndex, k);
            else
                idx = get_texCoord(&f->coordIndex, k);

            if(idx >= 0) renderer->vertex_texCoord(idx, (*f->texCoord)[idx]);
        }
    }

    void ifs_tesselator::do_coord(const int k) {
        int i = f->coordIndex[k];
        renderer->vertex_coord(i, (*f->coord)[i]);
    }

    void ifs_tesselator::do_vertex(const int k, const int facenr) {
        do_vertex_normal(k, facenr);
        do_vertex_color(k);
        do_texCoord(k);
        do_coord(k);
    }

#ifdef DEBUG
    static void print_coord(const Geometry* f, const int k) {
        int i = f->coordIndex[k];
        vec3 coord = (*f->coord)[i];
        cerr << "vertex " << k << ": i=" << i << ", coord=" << coord << "\n";
    }

    static void print_face(const Geometry* f, const int k, const int facenr) {
        cerr << "face " << facenr << " of " << f->name() << ";\n";

        for(int l = k; l < f->coordIndex.size && f->coordIndex[l] >= 0; l++)
            print_coord(f, l);
    }
#endif

    // Checks validity of face.
    bool ifs_tesselator::check_face(const int k, const int facenr) {
        current_face.size = 0;

        for(int l = k; l < f->coordIndex.size; l++) {
            int i = f->coordIndex[l];

            if(i < 0) break;

            if(i >= f->coord->size) {
                fprintf(stderr, "%s: face %d: %d-th coordinate index (%d) is "
                        "out of bound\n",
                        f->name(), facenr, l, i);
                return false;
            }

            current_face.append(i);
        }

        if(current_face.size < 3) {
            fprintf(stderr, "%s: face %d has too few vertices (only %d)",
                    f->name(), facenr, current_face.size);
            return false;
        }

        if(fnorm && vec3((*fnorm)[facenr]).square_length() < 0.99) {
            return false;
        }

        return true;
    }

    void ifs_tesselator::project_face_X(void) {
        for(int l = 0; l < current_face.size; l++) {
            int i = current_face[l];   // coordinate index (valid, we know)
            vec3& v = (*f->coord)[i];
            v2d[l] = vec2(v.Y(), v.Z());
        }
    }

    void ifs_tesselator::project_face_Y(void) {
        for(int l = 0; l < current_face.size; l++) {
            int i = current_face[l];   // coordinate index (valid, we know)
            vec3& v = (*f->coord)[i];
            v2d[l] = vec2(v.Z(), v.X());
        }
    }

    void ifs_tesselator::project_face_Z(void) {
        for(int l = 0; l < current_face.size; l++) {
            int i = current_face[l];   // coordinate index (valid, we know)
            vec3& v = (*f->coord)[i];
            v2d[l] = vec2(v.X(), v.Y());
        }
    }

    void ifs_tesselator::project_face(const vec3& face_normal) {
        v2d.size = 0;
        v2d.grow(current_face.size);   // make sufficient space

        // dominant component of normal, determines projection direction
        vec3 fn = vec3(face_normal).abs();

        if(fn.X() > fn.Y() && fn.X() > fn.Z())
            project_face_X();
        else if(fn.Y() > fn.Z())
            project_face_Y();
        else
            project_face_Z();
    }

    // Checks whether current face, projected to 2D with project_face()
    // is convex, by checking whether edges "turn" in the same
    // direction at all vertices.
    bool ifs_tesselator::face_is_convex(void) {
        if(current_face.size <= 3)
            return true;    // triangles are always convex

        bool cw = false, ccw = false;

        vec2 p = v2d[v2d.size - 1] - v2d[v2d.size - 2];
        vec2 c = v2d[0] - v2d[v2d.size - 1];
        double sense = p ^ c;
        cw  |= (sense >  EPSILON);   // clockwise turn
        ccw |= (sense < -EPSILON);   // counter clockwise turn

        for(int i = 1; i < v2d.size; i++) {
            p = c;
            c = v2d[i] - v2d[i - 1];
            sense = p ^ c;
            cw  |= (sense >  EPSILON);
            ccw |= (sense < -EPSILON);

            if(cw && ccw) return false;
        }

        return true;
    }

    static bool point_in_triangle(const vec2& p, const vec2& p1, const vec2& p2,
            const vec2& p3) {
        double u0, v0, u1, v1, u2, v2, a, b;

        u0 = p.U() - p1.U();
        v0 = p.V() - p1.V();
        u1 = p2.U() - p1.U();
        v1 = p2.V() - p1.V();
        u2 = p3.U() - p1.U();
        v2 = p3.V() - p1.V();

        a = 10.; b = 10.; // values large enough so the result would be false

        if(fabs(u1) < EPSILON) {
            if(fabs(u2) > EPSILON && fabs(v1) > EPSILON) {
                b = u0 / u2;

                if(b < EPSILON || b > 1. - EPSILON)
                    return false;
                else
                    a = (v0 - b * v2) / v1;
            }
        } else {
            b = v2 * u1 - u2 * v1;

            if(fabs(b) > EPSILON) {
                b = (v0 * u1 - u0 * v1) / b;

                if(b < EPSILON || b > 1. - EPSILON)
                    return false;
                else
                    a = (u0 - b * u2) / u1;
            }
        }

        return (a >= EPSILON && a <= 1. - EPSILON && (a + b) <= 1. - EPSILON);
    }

    // Checks whether the 2D line segments p1,p2 and p3,p4 intersect
    // with each other.
    // From Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7
    static bool segments_intersect(const vec2& p1, const vec2& p2,
            const vec2& p3, const vec2& p4) {
        double a, b, c, du, dv, r1, r2, r3, r4;
        bool colinear = false;

        du = fabs(p2.U() - p1.U());
        dv = fabs(p2.V() - p1.V());

        if(du > EPSILON || dv > EPSILON) {
            if(dv > du) {
                a = 1.0;
                b = - (p2.U() - p1.U()) / (p2.V() - p1.V());
                c = - (p1.U() + b * p1.V());
            } else {
                a = - (p2.V() - p1.V()) / (p2.U() - p1.U());
                b = 1.0;
                c = - (a * p1.U() + p1.V());
            }

            r3 = a * p3.U() + b * p3.V() + c;
            r4 = a * p4.U() + b * p4.V() + c;

            if(fabs(r3) < EPSILON && fabs(r4) < EPSILON)
                colinear = true;
            else if((r3 > -EPSILON && r4 > -EPSILON) || (r3 < EPSILON && r4 <
                        EPSILON))
                return false;
        }

        if(!colinear) {
            du = fabs(p4.U() - p3.U());
            dv = fabs(p4.V() - p3.V());

            if(du > EPSILON || dv > EPSILON) {
                if(dv > du) {
                    a = 1.0;
                    b = - (p4.U() - p3.U()) / (p4.V() - p3.V());
                    c = - (p3.U() + b * p3.V());
                } else {
                    a = - (p4.V() - p3.V()) / (p4.U() - p3.U());
                    b = 1.0;
                    c = - (a * p3.U() + p3.V());
                }

                r1 = a * p1.U() + b * p1.V() + c;
                r2 = a * p2.U() + b * p2.V() + c;

                if(fabs(r1) < EPSILON && fabs(r2) < EPSILON)
                    colinear = true;
                else if((r1 > -EPSILON && r2 > -EPSILON) || (r1 < EPSILON &&
                            r2 < EPSILON))
                    return false;
            }
        }

        if(!colinear)
            return true;

        // colinear segments never intersect: do as if they are always
        // a little bit apart from each other.
        return false;
    }

    // renderer callbacks for an automatically generated triangle
    void ifs_tesselator::do_triangle(const int k1, const int k2, const int k3,
            const int facenr) {
        renderer->begin_face(-1, 3);
        do_face_normal(facenr);
        do_face_color(facenr);
        do_vertex(k1, facenr);
        do_vertex(k2, facenr);
        do_vertex(k3, facenr);
        renderer->end_face(-1, 3);
    }

    // Triangulates current face, projected to 2D using project_face().
    // This routine started as an ANSI-C version of face2tri.C in
    // the MGF library. I changed it a lot to make it more robust in the
    // context of our RenderPark system for global illumination.
    // Inspiration comes from Burger and Gillis, Interactive Computer
    // Graphics and the (indispensable) Graphics Gems books.
    // Eventually, the code was re-converted to C++, and changed to
    // fit in the XRML library + to do all computations in 2D.
    void ifs_tesselator::triangulate(const int k, const int facenr) {
        int i;
        int n = current_face.size;

        // v is an array containing vertex indices or a special index
        // OUT (-2) to indicate that the corresponding vertex has been
        // processed.
        int vidx_buf[10];  // small buffer often avoids dynamic allocation
        int* v = vidx_buf;

        if(n > 10)
            v = new int[n];   // dynamically allocate larger buffer

        for(i = 0; i < n; i++)
            v[i] = current_face[i];  // copy vertex index

#define OUT (-2)

        // find vertex furthest away from the center of the polygon
        // first compute the center (everything in 2D).
        vec2 center = v2d[0];

        for(i = 1; i < n; i++)
            center += v2d[i];

        center /= (float)n;

        // now find the vertex furthest away from the center
        int furthest = 0; double maxd2 = 0;

        for(i = 1; i < n; i++) {
            double d2 = v2d[i] & v2d[i];

            if(d2 > maxd2) {
                maxd2 = d2;
                furthest = i;
            }
        }

        // find out in what sense the face contour turns at the furthest
        // vertex. If sense is counterclockwise, 'refsense' will be positive.
        int p0, p1, p2;
        p1 = furthest;

        p0 = p1 - 1; if(p0 < 0) p0 = n - 1;  // previous

        p2 = (p1 + 1) % n;                   // next
        float refsense = (v2d[p2] - v2d[p1]) ^ (v2d[p0] - v2d[p1]);

        int vertices_left = n;
        p0 = -1;

        while(vertices_left >= 3) {
            float sense = 0.;
            bool good = false;
            int start = p0;

            // Find next good triangle.
            // A triangle is good if
            // . it is formed by three consecutive non-yet-processed vertices
            // . its vertex order is consistent with the vertex order of the
            // polygon, as determined at the vertex furthest away from the
            // center
            // . no other not-yet-processed vertices of the polygon lay
            // inside the triangle
            // . no line segment formed by other consecutive not-yet-processed
            // vertices intersects the triangle
            do {
                p0 = (p0 + 1) % n;

                while(v[p0] == OUT)
                    p0 = (p0 + 1) % n;

                p1 = (p0 + 1) % n;

                while(v[p1] == OUT)
                    p1 = (p1 + 1) % n;

                p2 = (p1 + 1) % n;

                while(v[p2] == OUT)
                    p2 = (p2 + 1) % n;

                if(p0 == start)
                    break;

                sense = (v2d[p2] - v2d[p1]) ^ (v2d[p0] - v2d[p1]);
                good = sense * refsense >= 0.;   // same sense

                for(int i = 0; i < n && good; i++) {
                    if(v[i] == OUT || v[i] == v[p0] || v[i] == v[p1] || v[i] ==
                            v[p2])
                        continue;

                    if(point_in_triangle(v2d[i], v2d[p0], v2d[p1], v2d[p2])) {
                        good = false;
                        break;
                    }

                    int j = (i + 1) % n;

                    if(v[j] == OUT || v[j] == v[p0])
                        continue;

                    if(segments_intersect(v2d[p2], v2d[p0], v2d[i], v2d[j]))
                        good = false;
                }
            } while(!good);

            if(p0 == start) {
                // more than 3 vertices left but no good triangles anymore
                fprintf(stderr, "%s: face %d is misbuilt", f->name(), facenr);
                break;
            }

            if(fabs(sense) > EPSILON)       // avoid degenerate faces
                do_triangle(k + p0, k + p1, k + p2, facenr);

            v[p1] = OUT;
            vertices_left--;
        }

        if(n > 10) delete [] v;
    }

    void ifs_tesselator::triangulate_convex(const int k, const int facenr) {
        if(current_face.size < 3)
            return;

        for(int l = k + 1; l + 1 < f->coordIndex.size && f->coordIndex[l + 1]
                >= 0; l++) {
            do_triangle(k, l, l + 1, facenr);
        }
    }

    void ifs_tesselator::do_face_simple(const int k, const int facenr) {
        if(current_face.size > 3 && renderer->triangles_required) {
            triangulate_convex(k, facenr);
            return;
        }

        renderer->begin_face(facenr, current_face.size);
        do_face_normal(facenr);
        do_face_color(facenr);

        for(int l = k; l < f->coordIndex.size && f->coordIndex[l] >= 0; l++)
            do_vertex(l, facenr);

        renderer->end_face(facenr, current_face.size);
    }

    void ifs_tesselator::do_face(const int k, const int facenr) {
        if(!check_face(k, facenr)) {
            return;   // bad face
        }

        if(current_face.size == 3 || !renderer->convexity_required ||
                f->convex) {
            do_face_simple(k, facenr);
            return;
        }

        // compute face normal
        vec3 face_normal;

        if(fnorm)
            face_normal = (*fnorm)[facenr];
        else if(!compute_face_normal(&face_normal)) {
            return;
        }

        // project face vertices to 2D
        project_face(face_normal);

        if(face_is_convex())
            do_face_simple(k, facenr);
        else
            triangulate(k, facenr);
    }

    void ifs_tesselator::render(IndexedFaceSet* f) {
        ifs_tesselator::f = f;

        if(renderer->triangles_required) renderer->convexity_required = true;

        if(!f->coord || f->coordIndex.size <= 0) {
            fprintf(stderr, "%s: no coordinates or coordinate indices",
                    f->name());
            return;
        }

        prepare_normals();
        prepare_texCoords(f);

        renderer->begin_faces(f);
        int facenr = 0;

        for(int k = 0; k < f->coordIndex.size; k = next_face(k)) {
            do_face(k, facenr);
            facenr++;
        }

        renderer->end_faces(f);

        delete fnorm;
        delete vert;
    }

}   // namspace rt

