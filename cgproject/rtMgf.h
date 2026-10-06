/* rtMgf.h: MGF file loader for SRTK library */

#ifndef _RT_MGL_H_
#define _RT_MGL_H_

extern bool rtReadMgf(const char* filename);

/* sets the number of quarter circle divisions for discretizing cylinders,
 * spheres, cones ... */
extern void rtMgfSetNrQuartCircDivs(int divs);

#endif /* RT_MGL_H_ */

