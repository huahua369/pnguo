#ifndef __tesselate_h__
#define __tesselate_h__

#ifdef __cplusplus
extern "C" {
#endif

void tessellate(double **verts, int *nverts, int **tris, int *ntris, const double **contoursbegin,
                const double **contoursend);

#ifdef __cplusplus
}
#endif

#endif /* __tesselate_h__ */
