/* rtAssimp.h: model loader for SRTK built on the Open Asset Import Library.
 *
 * Replaces the old lib3ds-based rt3ds.cpp. Assimp reads .3ds natively, so
 * models/kamer.3ds still loads, and in addition it handles .obj, .ply,
 * .gltf/.glb, .dae, .fbx, .stl and about forty other formats -- see
 * rtAssimpSupportedExtensions().
 *
 * .mgf files are *not* handled here: they go through rtMgf.cpp, which
 * understands MGF's physically specified materials.
 */

#ifndef _RT_ASSIMP_H_
#define _RT_ASSIMP_H_

/* Loads any model format Assimp understands into the srtk world. */
extern bool rtReadAssimp(const char* filename);

/* Returns a static, human readable list of the extensions this build of
 * Assimp can read, e.g. "*.obj;*.3ds;*.ply;...". Used for error messages. */
extern const char* rtAssimpSupportedExtensions(void);

#endif /* _RT_ASSIMP_H_ */
