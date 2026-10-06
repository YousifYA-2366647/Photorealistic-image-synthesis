// rtIndexedFaceSet.h: loads VRML-style IndexedFaceSet into SRTK

#ifndef _RT_INDEXEDFACESET_H_
#define _RT_INDEXEDFACESET_H_

#include "srtk/array.h"
#include "vecmath.h"

namespace rt {

    struct IndexedFaceSet {
        array<vec3>*        color;
        array<vec3>*        coord;
        array<vec3>*        normal;
        array<vec2>*          texCoord;
        bool                ccw;
        array<int>          colorIndex;
        bool                colorPerVertex;
        bool                convex;
        array<int>          coordIndex;
        float               creaseAngle;
        array<int>          normalIndex;
        bool                normalPerVertex;
        bool                solid;
        array<int>          texCoordIndex;
        const char* id;   // id string

        IndexedFaceSet();
        const char* name(void) const;
    };

    class ifs_renderer {
        IndexedFaceSet* f;
    public:
        void begin_faces(IndexedFaceSet*);
        void begin_face(int id, int nverts);
        void face_normal(int id, const vec3&);
        void face_color(int id, const vec3&);
        void vertex_normal(int id, const vec3&);
        void vertex_color(int id, const vec3&);
        void vertex_texCoord(int id, const vec2&);
        void vertex_coord(int id, const vec3&);
        void end_face(int id, int nverts);
        void end_faces(IndexedFaceSet*);
        bool triangles_required;
        bool convexity_required;
        bool normals_required;
        bool texcoords_required;
        bool colors_required;

        ifs_renderer() {
            triangles_required = true;
            convexity_required = true;
            normals_required = true;
            texcoords_required = true;
            colors_required = true;
        }
    };

    class ifs_tesselator {
    protected:
        struct DVec3 {
            double x, y, z;
            DVec3(double xx = 0., double yy = 0., double zz = 0.) {
                x = xx;
                y = yy;
                z = zz;
            }
        };

        // All subsequent routines process the geometry pointed to by 'f'.
        // 'f' is set in render().
        const IndexedFaceSet* f;

        // count the number of faces in the (current) geometry
        int count_faces(void);

        // Skips to past next end-of-face marker from position k in the
        // coordIndex array onwards
        int next_face(int k);

        // Face coordinate index array: describes "current face". Used by
        // prepare_face_norm() and do_face().
        array<int> current_face;

        // Checks face described starting at index k in the coordIndex array of
        // f.
        // Fills in current_face index array. Returns false + complains if
        // . face has too few vertices
        // . some coordinate index in the face is out of bound
        // . automatically generated normal for the face (if any) is
        // not normalized (degenerate face).
        // Returns true if all is OK.
        bool check_face(const int k, const int facenr);

        // automatic (vertex) normal generation
        struct vertshare {   // vertex sharing information
            array<int> faces; // indices of faces sharing the vertex
        };
        array<vertshare>* vert;   // sharing info for all vertices
        array<vec3>* fnorm;                  // face normals
        float cosCreaseAngle;
        // computes face normal of current face. Fills in result in 'normal'.
        // Returns true if all is allright and false is something is wrong.
        // Does not complain if something is wrong.
        bool compute_face_normal(vec3* normal);
        // Prepares for automatic normal generation: computes 'vert'
        // and 'fnorm' for the current geometry. If no normals are required,
        // or there are normals given, this routine immediately returns.
        void prepare_normals(void);
        void prepare_face_normal(const int k, const int facenr);
        // computes vertex normal given face normals and vertex sharing
        // information
        const vec3 gen_normal(const int vertnr, const int facenr);

        // automatic texture coordinate generation
        vec3 min;            // geometry bounding box corner
        int sidx, tidx;      // coordinate index for S and T texture coordinate
        float side;          // longest edge of geometry bounding box
        // Prepare for automatic texture coordinate generation: computes
        // min, sidx, tidx and side for given geometry. Returns immediately
        // if no texture coordinates are required or texture coordinates are
        // given. Note: we need non-const Geometry* here unfortunately, because
        // prepare_texCoords needs to compute the Geometry's bounding box.
        void prepare_texCoords(IndexedFaceSet* f);
        // computes texture coordinate based on the above information
        const vec2 gen_texCoord(const int vertnr);

        // current face vertices projected to 2D by project_face(), used by
        // face_is_convex() and triangulate().
        array<vec2> v2d;
        void project_face(const vec3& face_normal);
        void project_face_X(void);    // projects on YZ plane (perp. X)
        void project_face_Y(void);
        void project_face_Z(void);

        // Checks whether or not current face, projected to 2D is convex
        bool face_is_convex(void);

        // Triangulates the current face, projected to 2D by project_face().
        // (Generates renderer calls as an alternative for do_face_simple()).
        void triangulate(const int k, const int facenr);

        // Triangulates current face, knowing that it is convex. This is
        // much cheaper than for non-convex polygons.
        void triangulate_convex(const int k, const int facenr);

        // retrieves normal, color, texCoord with index i from
        // f->norm, f->col, f->texco. If idx is not null, uses
        // idx[i] as the index instead of i.
        int get_normal(const array<int>* idx, const int i);
        int get_color(const array<int>* idx, const int i);
        int get_texCoord(const array<int>* idx, const int i);

        // 'k' is index in f->coordIndex array, facenr is nr of current face.
        void do_face_normal(const int facenr);
        void do_face_color(const int facenr);
        void do_vertex_normal(const int k, const int facenr);
        void do_vertex_color(const int k);
        void do_texCoord(const int k);
        void do_coord(const int k);
        void do_vertex(const int k, const int facenr);

        // Generates renderer handlers calls for automatically generated
        // triangle (by triangulate() or triangulate_convex()). facenr
        // is needed to find the face normal, but the facenr passed to
        // the renderer handlers is -1, indicating an automatically generated
        // primitive.
        void do_triangle(const int k1, const int k2, const int k3,
                const int facenr);

        void do_face_simple(const int k, const int facenr);
        void do_face(const int k, const int facenr);

        class ifs_renderer* renderer;

    public:
        ifs_tesselator(ifs_renderer* r) {
            ifs_tesselator::renderer = r;
            f = 0;
            current_face.init(0, 0);
            v2d.init(0, 0);
            vert = 0; fnorm = 0; cosCreaseAngle = -1.;
            min = vec3(0, 0, 0); sidx = tidx = -1; side = 0.;
        }

        void render(IndexedFaceSet* object);
    };

}   // namespace rt

#endif /*_RT_INDEXEDFACESET_H_*/

