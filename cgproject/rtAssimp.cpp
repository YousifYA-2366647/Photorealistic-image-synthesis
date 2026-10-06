// rtAssimp.cpp: model loader for srtk built on the Open Asset Import Library.
//
// This replaces the old lib3ds loader. Besides being a dependency that is
// still maintained and packaged, Assimp gives us two things the 3DS path
// never had: more material data (transparency and index of refraction
// actually come through now, which matters for the refraction part of the
// practicum), and every other common mesh format for free.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifdef HAVE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_GIF
#include <stb_image.h>
#endif

#include "rtAssimp.h"
#include "srtk/rt.h"

using namespace rt;

//---------------------------------------------------------------------------
// Textures
//---------------------------------------------------------------------------

// rtTexture() returns an index into srtk's texture array, and the first
// texture loaded gets index 0 -- so 0 is a perfectly valid id and cannot
// double as "no texture". These are the sentinels instead.
static const int NO_TEXTURE = -1;    // resolved: this material has none
static const int TEXTURE_UNRESOLVED = -2;  // not looked at yet

#ifdef HAVE_STB_IMAGE

// srtk keeps only the pointer handed to rtTexture() and expects the image
// bottom-to-top, so we flip on load and never free the buffer -- it has to
// outlive the loader anyway, and it lives until the program exits.
static int upload_texture(const unsigned char* data, int w, int h, int comp) {
    if(!data || w <= 0 || h <= 0)
        return NO_TEXTURE;

    const int channels = (comp == 4) ? 4 : 3;
    return rtTexture(w, h, channels, data);
}

static int load_texture_file(const std::string& path) {
    int w = 0, h = 0, comp = 0;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 0);

    if(!data) {
        fprintf(stderr, "  texture '%s' could not be read (%s)\n",
                path.c_str(), stbi_failure_reason());
        return NO_TEXTURE;
    }

    if(comp != 3 && comp != 4) {   // greyscale: re-read as RGB
        stbi_image_free(data);
        data = stbi_load(path.c_str(), &w, &h, &comp, 3);
        comp = 3;
        if(!data) return NO_TEXTURE;
    }

    return upload_texture(data, w, h, comp);
}

static int load_embedded_texture(const aiTexture* tex) {
    int w = 0, h = 0, comp = 0;
    stbi_set_flip_vertically_on_load(1);

    if(tex->mHeight == 0) {
        // Compressed blob (png/jpg/...) of mWidth bytes.
        unsigned char* data = stbi_load_from_memory(
            (const unsigned char*)tex->pcData, (int)tex->mWidth,
            &w, &h, &comp, 0);
        if(!data) return NO_TEXTURE;
        if(comp != 3 && comp != 4) {
            stbi_image_free(data);
            data = stbi_load_from_memory((const unsigned char*)tex->pcData,
                                         (int)tex->mWidth, &w, &h, &comp, 3);
            comp = 3;
            if(!data) return NO_TEXTURE;
        }
        return upload_texture(data, w, h, comp);
    }

    // Uncompressed ARGB8888, top-to-bottom. Convert to bottom-up RGBA.
    w = (int)tex->mWidth;
    h = (int)tex->mHeight;
    unsigned char* rgba = (unsigned char*)malloc((size_t)w * h * 4);
    if(!rgba) return NO_TEXTURE;

    for(int y = 0; y < h; y++) {
        const aiTexel* src = tex->pcData + (size_t)y * w;
        unsigned char* dst = rgba + (size_t)(h - 1 - y) * w * 4;
        for(int x = 0; x < w; x++) {
            dst[4 * x + 0] = src[x].r;
            dst[4 * x + 1] = src[x].g;
            dst[4 * x + 2] = src[x].b;
            dst[4 * x + 3] = src[x].a;
        }
    }
    return upload_texture(rgba, w, h, 4);
}

#endif /* HAVE_STB_IMAGE */

// Resolves the diffuse texture of a material, if any. Returns an srtk texture
// id, or NO_TEXTURE.
static int material_texture(const aiScene* scene, const aiMaterial* mat,
                            const std::string& modeldir) {
    if(mat->GetTextureCount(aiTextureType_DIFFUSE) == 0)
        return NO_TEXTURE;

    aiString texpath;
    if(mat->GetTexture(aiTextureType_DIFFUSE, 0, &texpath) != AI_SUCCESS)
        return NO_TEXTURE;

#ifndef HAVE_STB_IMAGE
    (void)scene; (void)modeldir;
    static bool warned = false;
    if(!warned) {
        fprintf(stderr, "  model references textures but this build has no "
                        "image loader (stb_image.h was not found)\n");
        warned = true;
    }
    return NO_TEXTURE;
#else
    if(const aiTexture* embedded = scene->GetEmbeddedTexture(texpath.C_Str()))
        return load_embedded_texture(embedded);

    std::string path = texpath.C_Str();
    // Model files habitually store Windows paths; srtk does not care but the
    // filesystem does.
    for(size_t i = 0; i < path.size(); i++)
        if(path[i] == '\\') path[i] = '/';

    int id = load_texture_file(path);
    if(id < 0 && !modeldir.empty())
        id = load_texture_file(modeldir + "/" + path);

    // Last resort: try just the basename next to the model.
    if(id < 0) {
        size_t slash = path.find_last_of('/');
        if(slash != std::string::npos) {
            std::string base = path.substr(slash + 1);
            id = load_texture_file(base);
            if(id < 0 && !modeldir.empty())
                id = load_texture_file(modeldir + "/" + base);
        }
    }
    return id;
#endif
}

//---------------------------------------------------------------------------
// Materials
//---------------------------------------------------------------------------

static void bind_material(const aiScene* scene, unsigned index,
                          const std::string& modeldir,
                          int* texture_ids) {
    if(index >= scene->mNumMaterials) {
        // No material: the same neutral grey the old loader used.
        float black[3] = {0, 0, 0};
        float grey[3]  = {0.8f, 0.8f, 0.8f};
        rtMaterial(black, grey, black, 0.f, black, 1.f);
        rtUnbindTexture();
        return;
    }

    const aiMaterial* mat = scene->mMaterials[index];

    aiColor3D diffuse(0.8f, 0.8f, 0.8f);
    aiColor3D specular(0.f, 0.f, 0.f);
    aiColor3D emissive(0.f, 0.f, 0.f);
    aiColor3D transparent(0.f, 0.f, 0.f);
    float shininess = 0.f;
    float opacity = 1.f;
    float ior = 1.f;

    mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
    mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
    mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
    mat->Get(AI_MATKEY_SHININESS, shininess);
    mat->Get(AI_MATKEY_OPACITY, opacity);
    mat->Get(AI_MATKEY_REFRACTI, ior);

    // srtk treats shininess > 128 as a perfect mirror; the old 3DS loader
    // clamped there and we keep that, so a model never silently turns into
    // a mirror. Use an .mgf file if you want one.
    if(shininess > 128.f) shininess = 128.f;
    if(shininess < 0.f)   shininess = 0.f;

    // Assimp reports transparency either as a scalar opacity or as a
    // transmission filter colour; prefer the colour when it is present.
    float transmissivity[3] = {0.f, 0.f, 0.f};
    if(mat->Get(AI_MATKEY_COLOR_TRANSPARENT, transparent) == AI_SUCCESS &&
       (transparent.r > 0.f || transparent.g > 0.f || transparent.b > 0.f)) {
        transmissivity[0] = transparent.r;
        transmissivity[1] = transparent.g;
        transmissivity[2] = transparent.b;
    } else if(opacity < 1.f) {
        const float t = 1.f - opacity;
        transmissivity[0] = transmissivity[1] = transmissivity[2] = t;
    }

    if(ior < 1.f) ior = 1.f;

    float e[3] = {emissive.r, emissive.g, emissive.b};
    float d[3] = {diffuse.r,  diffuse.g,  diffuse.b};
    float s[3] = {specular.r, specular.g, specular.b};

    rtMaterial(e, d, s, shininess, transmissivity, ior);

    rtUnbindTexture();
    if(texture_ids) {
        if(texture_ids[index] == TEXTURE_UNRESOLVED)
            texture_ids[index] = material_texture(scene, mat, modeldir);
        if(texture_ids[index] >= 0)
            rtBindTexture(texture_ids[index]);
    }
}

//---------------------------------------------------------------------------
// Geometry
//---------------------------------------------------------------------------

// Assimp always delivers a right-handed Y-up scene (its 3DS importer, for
// instance, rotates the file's native Z-up data by -90 degrees about X).
// The framework's world is Z-up, like MGF, so rotate back: (x,y,z) -> (x,-z,y).
static inline void yup_to_zup(const aiVector3D& v, float out[3]) {
    out[0] =  v.x;
    out[1] = -v.z;
    out[2] =  v.y;
}

static void add_mesh(const aiMesh* mesh) {
    rtBeginTriangleSet();

    for(unsigned f = 0; f < mesh->mNumFaces; f++) {
        const aiFace& face = mesh->mFaces[f];
        if(face.mNumIndices != 3)
            continue;   // aiProcess_Triangulate should have prevented this

        for(unsigned k = 0; k < 3; k++) {
            const unsigned v = face.mIndices[k];

            if(mesh->HasNormals()) {
                float n[3];
                yup_to_zup(mesh->mNormals[v], n);
                rtNormal3(n);
            }

            if(mesh->HasTextureCoords(0)) {
                float t[3] = {mesh->mTextureCoords[0][v].x,
                              mesh->mTextureCoords[0][v].y,
                              mesh->mTextureCoords[0][v].z};
                rtTexCoord3(t);
            }

            float p[3];
            yup_to_zup(mesh->mVertices[v], p);
            rtVertex3(p);
        }
    }

    rtEndTriangleSet();
}

//---------------------------------------------------------------------------

const char* rtAssimpSupportedExtensions(void) {
    static std::string ext;
    if(ext.empty()) {
        Assimp::Importer importer;
        aiString s;
        importer.GetExtensionList(s);
        ext = s.C_Str();
    }
    return ext.c_str();
}

bool rtReadAssimp(const char* filename) {
    Assimp::Importer importer;

    // aiProcess_PreTransformVertices flattens the node hierarchy and bakes
    // every node transform into the vertices, which is exactly what srtk
    // wants: it has no scene graph of its own.
    const unsigned flags =
        aiProcess_Triangulate |
        aiProcess_PreTransformVertices |
        aiProcess_GenSmoothNormals |        // only fires if normals are absent
        aiProcess_JoinIdenticalVertices |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_GenUVCoords |
        aiProcess_SortByPType;

    printf("Reading '%s' with Assimp ... ", filename);
    fflush(stdout);

    const aiScene* scene = importer.ReadFile(filename, flags);

    if(!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
       !scene->mRootNode) {
        fprintf(stderr, "\n***ERROR*** could not load '%s': %s\n",
                filename, importer.GetErrorString());
        return false;
    }

    if(scene->mNumMeshes == 0) {
        fprintf(stderr, "\n***ERROR*** '%s' contains no meshes.\n", filename);
        return false;
    }

    // Directory of the model, for resolving relative texture paths.
    std::string modeldir;
    {
        std::string f = filename;
        for(size_t i = 0; i < f.size(); i++)
            if(f[i] == '\\') f[i] = '/';
        size_t slash = f.find_last_of('/');
        if(slash != std::string::npos)
            modeldir = f.substr(0, slash);
    }

    // Textures are loaded lazily, at most once per material.
    int* texture_ids = 0;
    if(scene->mNumMaterials > 0) {
        texture_ids = new int [scene->mNumMaterials];
        for(unsigned i = 0; i < scene->mNumMaterials; i++)
            texture_ids[i] = TEXTURE_UNRESOLVED;
    }

    rtInit();
    rtBeginWorld();

    unsigned long long tris = 0;
    for(unsigned m = 0; m < scene->mNumMeshes; m++) {
        const aiMesh* mesh = scene->mMeshes[m];

        // aiProcess_SortByPType leaves point/line meshes in the scene; a ray
        // tracer has nothing to do with them.
        if(!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
            continue;
        if(mesh->mNumFaces == 0)
            continue;

        bind_material(scene, mesh->mMaterialIndex, modeldir, texture_ids);
        add_mesh(mesh);
        tris += mesh->mNumFaces;
    }

    rtEndWorld();

    delete [] texture_ids;

    printf("done: %llu triangles, %u meshes, %u materials.\n",
           tris, scene->mNumMeshes, scene->mNumMaterials);

    if(tris == 0) {
        fprintf(stderr, "***ERROR*** '%s' contains no triangles.\n", filename);
        return false;
    }

    return true;
}
