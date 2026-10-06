/* rtMGF.cpp: loads MGF file into SRTK library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtMgf.h"

#include "mgflib/parser.h"

#include "srtk/rt.h"
#include "srtk/R3.h"
#include "srtk/array.h"

#include "cie.h"
#include "vecmath.h"
#include "rtIndexedFaceSet.h"

using namespace rt;

static struct rtMgfImporter* current_importer = 0;
static int do_face(int ac, char** av);  // forward decl.
static int do_obj(int ac, char** av);

struct rtMgfImporter {
    intptr_t app;                // current RT material index
    array<vec3>* coord;     // current face set
    array<vec3>* normal;
    array<int>* coordIndex;
    int xid;                // current transform context id
    int cid;                // current shape id

    IndexedFaceSet* ifs;     // container for coord, normal and coordIndex
    ifs_tesselator* tesselator;
    ifs_renderer* renderer;  // loads IndexedFaceSet into SRTK

    static void do_error(const char* errmsg) {
        fprintf(stderr, "%s line %d: %s\n", mg_file->fname, mg_file->lineno,
                errmsg);
    }

    static void do_warning(const char* errmsg) {
        fprintf(stderr, "%s line %d: %s\n", mg_file->fname, mg_file->lineno,
                errmsg);
    }

    void beginFaceSet(void) {
        cid ++;

        if(!ifs) {
            ifs = new IndexedFaceSet;
            ifs->coord = new array<vec3>;
            ifs->normal = new array<vec3>;
            ifs->normalPerVertex = true;
            ifs->convex = false;
            ifs->solid = false;
        }

        coord = ifs->coord; coord->clear();
        normal = ifs->normal; normal->clear();
        coordIndex = &ifs->coordIndex; coordIndex->clear();

        static char buf[100];
        sprintf(buf, "face set starting at %s:%d", mg_file->fname,
                mg_file->lineno);
        ifs->id = buf;
    }

    void endFaceSet(int app) {
        if(!renderer) {
            renderer = new ifs_renderer;
            renderer->texcoords_required = false;
            renderer->colors_required = false;
        }

        if(!tesselator) tesselator = new ifs_tesselator(renderer);

        if(coord && coord->size > 0 && coordIndex->size > 0) {
            rtBindMaterial(app);
            tesselator->render(ifs);
        }

        // indicate that there is no open face set anymore.
        coord = 0;
        normal = 0;
        coordIndex = 0;
    }

    /* Translates MGF color into out color representation. */
    static void GetColor(C_COLOR* cin, double intensity, R3& rgb) {
        float xyz[3];

        c_ccvt(cin, C_CSXY);

        if(cin->cy > EPSILON) {
            xyz[0] = cin->cx / cin->cy * intensity;
            xyz[1] = 1. * intensity;
            xyz[2] = (1. - cin->cx - cin->cy) / cin->cy * intensity;
        } else {
            do_warning("invalid color specification (Y<=0) ... setting to "
                    "black");
            xyz[0] = xyz[1] = xyz[2] = 0.;
        }

        if(xyz[0] < 0. || xyz[1] < 0. || xyz[2] < 0.) {
            do_warning("invalid color specification (negative CIE XYZ "
                    "componenets) ... clipping to zero");

            if(xyz[0] < 0.) xyz[0] = 0.;

            if(xyz[1] < 1.) xyz[1] = 0.;

            if(xyz[2] < 2.) xyz[2] = 0.;
        }

        xyz_rgb(xyz, rgb);

        if(clipgamut(rgb))
            do_warning("color desaturated during gamut clipping");
    }

    static float ColorMax(const R3& col) {
        return col[0] > col[1] && col[0] > col[2] ? col[0] : (col[1] > col[2] ?
                col[1] : col[2]);
    }

    // Returns srtk index of material (as returned by rtMaterial)
    int GetAppearance(void) {
        R3 Ed, Es, Rd, Td, Rs, Ts, A;
        float Ne, Nr, Nt, a;

        /* Convert intensities and chromaticities to our color model. */
        GetColor(&c_cmaterial->ed_c, c_cmaterial->ed, Ed);
        GetColor(&c_cmaterial->rd_c, c_cmaterial->rd, Rd);
        GetColor(&c_cmaterial->td_c, c_cmaterial->td, Td);
        GetColor(&c_cmaterial->rs_c, c_cmaterial->rs, Rs);
        GetColor(&c_cmaterial->ts_c, c_cmaterial->ts, Ts);

        /* check/correct range of reflectances and transmittances */
        R3ADD(Rd, Rs, A);

        if((a = ColorMax(A)) > 1. - EPSILON) {
            do_warning("invalid material specification: total reflectance shall"
                    " be < 1");
            a = (1. - EPSILON) / a;
            R3SCALE(a, Rd, Rd);
            R3SCALE(a, Rs, Rs);
        }

        R3ADD(Td, Ts, A);

        if((a = ColorMax(A)) > 1. - EPSILON) {
            do_warning("invalid material specification: total transmittance sha"
                    "ll be < 1");
            a = (1. - EPSILON) / a;
            R3SCALE(a, Td, Td);
            R3SCALE(a, Ts, Ts);
        }

        /* convert lumen/m^2 to W/m^2 */
        R3SCALE((1. / WHITE_EFFICACY), Ed, Ed);

        R3SET(Es, 0, 0, 0);
        Ne = 0.;

        /* specular power = (0.6/roughness)^2 (see MGF docs) */
        if(c_cmaterial->rs_a != 0.0) {
            Nr = 0.6 / c_cmaterial->rs_a;
            Nr *= Nr;
        } else
            Nr = 0.0;

        if(c_cmaterial->ts_a != 0.0) {
            Nt = 0.6 / c_cmaterial->ts_a;
            Nt *= Nt;
        } else
            Nt = 0.0;

        return rtMaterial(Ed, Rd, Rs, Nr, Ts, c_cmaterial->nr,
                (void*)c_cmaterial);
    }

    // Returns current RT material index
    intptr_t GetCurrentAppearance(void) {
        if(c_cmaterial->client_data != 0  // had this material before
                && c_cmaterial->client_data == (char*)app   // same material
                && c_cmaterial->clock == 0) {   // material unchanged
            return app;
        }

        if(c_cmaterial->clock != 0 || c_cmaterial->client_data == 0) {
            // new material or material changed
            intptr_t newapp = GetAppearance();
            c_cmaterial->client_data = (char*)newapp;
            c_cmaterial->clock = 0;
            return newapp;
        }

        return (intptr_t)c_cmaterial->client_data;
    }

    static int GetCurrentXID(void) {
        return xf_context ? xf_context->xid : -10;
    }

    typedef struct VDATA {
        int index, cid, xid;
    } VDATA;

    // TODO: Either use have_normals, or remove parameter.
    int GetVertex(C_VERTEX* v, bool have_normals, FVECT& face_norm) {
        (void)have_normals;
        if(v->n[0] == 0. && v->n[1] == 0. && v->n[2] == 0.) {
            // vertex without normal: never shared
            FVECT p, n;
            xf_xfmpoint(p, v->p);
            int index = ifs->coord->append(vec3(p[0], p[1], p[2]));
            xf_xfmvect(n, face_norm);
            normal->append(vec3(n[0], n[1], n[2]));
            return index;
        }

        VDATA* vd = (VDATA*)(v->client_data);

        if(!vd
                || vd->cid != cid
                || vd->xid != GetCurrentXID()
                || v->clock > 0) {
            // new vertex, or vertex changed, or transform changed, or
            // other object than before - record vertex and make new tag
            FVECT p, n;
            xf_xfmpoint(p, v->p);
            int index = coord->append(vec3(p[0], p[1], p[2]));
            xf_xfmvect(n, v->n);
            normal->append(vec3(n[0], n[1], n[2]));

            if(!vd) vd = new VDATA;

            vd->cid = cid;
            vd->xid = GetCurrentXID();
            vd->index = index;
            v->client_data = (char*)vd;
            v->clock = 0;
        }

        return vd->index;
    }

    inline static vec3 cvtvec(double* w) {
        vec3 v(w[0], w[1], w[2]);
        return v;
    }

    static bool ComputeFaceNormal(int nverts, C_VERTEX** v, vec3* normal) {
        vec3 n(0, 0, 0);
        vec3 o = cvtvec(v[0]->p);     // origin
        vec3 p = cvtvec(v[nverts - 1]->p);
        vec3 prev, cur = p - o;

        for(int j = 0; j < nverts; j++) {
            prev = cur;
            vec3 p = cvtvec(v[j]->p);
            cur = p - o;
            n[0] += (prev[1] - cur[1]) * (prev[2] + cur[2]);
            n[1] += (prev[2] - cur[2]) * (prev[0] + cur[0]);
            n[2] += (prev[0] - cur[0]) * (prev[1] + cur[1]);
        }

        double norm = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        n[0] /= norm; n[1] /= norm; n[2] /= norm;

        if(!isfinite(n[0]) || !isfinite(n[1]) || !isfinite(n[2])) {
            *normal = vec3(0, 0, 0);
            return false;
        }

        *normal = n;
        return true;
    }

    int face(int ac, char** av) {
        int curapp = GetCurrentAppearance();

        if(curapp != app || xid != GetCurrentXID()) {
            // new appearance or new transform
            endFaceSet(app);
            beginFaceSet();
        }

        if(!coord)
            beginFaceSet();

        app = curapp;
        xid = GetCurrentXID();

        if(ac < 4)
            return MG_EARGC;

        bool have_normals = true;
        C_VERTEX** vp = (C_VERTEX**)alloca((ac - 1) * sizeof(C_VERTEX*));

        for(int i = 1; i < ac; i++) {
            vp[i - 1] = c_getvert(av[i]);

            if(!vp[i - 1])
                return MG_EUNDEF;

            FVECT& n = vp[i - 1]->n;

            if(n[0] == 0. && n[1] == 0. && n[2] == 0.)
                have_normals = false;
        }

        if(xf_context && xf_context->rev) {  // reverse vertex order
            for(int i = 0, j = ac - 2; i < j; i++, j--) {
                C_VERTEX* t = vp[i];
                vp[i] = vp[j];
                vp[j] = t;
            }
        }

        FVECT face_norm = {0, 0, 0};

        if(!have_normals) {    // compute face normal
            vec3 n;

            if(!ComputeFaceNormal(ac - 1, vp, &n)) {
                do_warning("degenerate face");
                return MG_OK;
            }

            face_norm[0] = n[0];
            face_norm[1] = n[1];
            face_norm[2] = n[2];
        }

        for(int n = 0; n < ac - 1; n++) {
            int index = GetVertex(vp[n], have_normals, face_norm);
            coordIndex->append(index);
        }

        coordIndex->append(-1);  // end-of-face marker

        return MG_OK;
    }

    int obj_level;
    int obj(int ac, char** av) {
        if(ac > 1) {
            for(int i = 0; i < obj_level * 2; i++)
                putchar(' ');

            printf("%s\n", av[1]);
            obj_level++;
        }   else {
            obj_level--;

            if(coord) {    // close   current face set
                endFaceSet(app);
            }
        }

        return obj_handler(ac, av);
    }

    // initializes MGF entity handler table
    static void init(void) {
        mg_ehand[MG_E_COLOR] = c_hcolor;
        mg_ehand[MG_E_CXY] = c_hcolor;

        mg_ehand[MG_E_MATERIAL] = c_hmaterial;
        mg_ehand[MG_E_SIDES] = c_hmaterial;
        mg_ehand[MG_E_RD] = c_hmaterial;
        mg_ehand[MG_E_TD] = c_hmaterial;
        mg_ehand[MG_E_ED] = c_hmaterial;
        mg_ehand[MG_E_RS] = c_hmaterial;
        mg_ehand[MG_E_TS] = c_hmaterial;
        mg_ehand[MG_E_IR] = c_hmaterial;

        mg_ehand[MG_E_VERTEX] = c_hvertex;
        mg_ehand[MG_E_POINT] = c_hvertex;
        mg_ehand[MG_E_NORMAL] = c_hvertex;

        mg_ehand[MG_E_XF] = xf_handler;

        mg_ehand[MG_E_FACE] = do_face;
        mg_ehand[MG_E_OBJECT] = do_obj;

        mg_init();
    }

    bool parse(const char* filename) {
        current_importer = this;

        init();

        coord = 0;
        normal = 0;
        coordIndex = 0;
        app = 0;
        xid = -10;
        cid = 0;

        obj_level = 0;

        MG_FCTXT fctxt;
        int err = mg_open(&fctxt, (char*)filename);

        if(err)
            do_error(mg_err[err]);
        else {
            while(mg_read() > 0 && !err) {
                err = mg_parse();

                if(err)
                    do_error(mg_err[err]);
            }

            mg_close();
        }

        mg_clear();

        if(coord)    // close current face set
            endFaceSet(app);

        return err == 0;
    }

    rtMgfImporter() {
        ifs = 0;
        tesselator = 0;
        renderer = 0;
    }
};

static int do_face(int ac, char** av) {
    return current_importer->face(ac, av);
}

static int do_obj(int ac, char** av) {
    return current_importer->obj(ac, av);
}
bool rtReadMgf(const char* filename) {
#define CIE_x_r                 0.640    /* nominal CRT primaries */
#define CIE_y_r                 0.330
#define CIE_x_g                 0.290
#define CIE_y_g                 0.600
#define CIE_x_b                 0.150
#define CIE_y_b                 0.060
#define CIE_x_w                 0.3333333333  /* monitor white point */
#define CIE_y_w                 0.3333333333
    ComputeColorConversionTransforms(CIE_x_r, CIE_y_r, CIE_x_g, CIE_y_g,
            CIE_x_b, CIE_y_b, CIE_x_w, CIE_y_w);

    rtInit();
    rtBeginWorld();
    rtMgfImporter import;
    bool rc = import.parse(filename);
    rtEndWorld();
    return rc;
}

/* sets the number of quarter circle divisions for discretizing cylinders,
 * spheres, cones ... */
void rtMgfSetNrQuartCircDivs(int divs) {
    mg_nqcdivs = divs;
}

