/***************************************************************************
 *
 *  Copyright (C) 2024 CutDigital Enterprise Ltd
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  For your convenience, a copy of the License has been included in this
 *  repository.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.WE
 *
 **************************************************************************/

#ifndef __MIO_OBJ_H__
#define __MIO_OBJ_H__  1

#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus
    typedef struct MioMesh
    {
        double* pVertices;
        double* pNormals;
        double* pTexCoords;

        unsigned int* pFaceSizes;
        unsigned int* pFaceVertexIndices;
        unsigned int* pFaceVertexTexCoordIndices;
        unsigned int* pFaceVertexNormalIndices;

        unsigned int numVertices;
        unsigned int numNormals;
        unsigned int numTexCoords;
        unsigned int numFaces;
    }MioMesh; 
    void mioFree(void* pMemPtr);
    void mioFreeMesh(MioMesh* pMemPtr);
/*
    Funcion to read in an obj file that stores a single 3D mesh object (in ASCII
    format). The pointer parameters will be allocated inside this function and must
    be freed by caller. The function only handles polygonal faces, so commands like
    "vp" command (which is used to specify control points of the surface or curve)
    are ignored if encountered in file.

*/
void mioReadOBJ(
    // absolute path to file
    const char* fpath,
    // pointer to list of vertex coordinates stored as [xyz,xyz,xyz,...]
    double** pVertices,
    // pointer to list of vertex normals stored as [xyz,xyz,xyz...]
    double** pNormals,
    // pointer to list of texture coordinates list stored as [xy,xy,xy...]
    double** pTexCoords,
    // pointer to list of face sizes (number of vertices in each face) stored as [a,b,c,d,e,f,g,...]
    unsigned int** pFaceSizes,
    // pointer to list of face-vertex indices stored as [ijkl,ijk,ijkl,ijklmn,ijk,...]
    unsigned int** pFaceVertexIndices,
    // pointer to list of face-vertex texture-coord indices stored as [ijkl,ijk,ijkl,ijklmn,ijk,...] (same order as in "pFaceVertexIndices")
    unsigned int** pFaceVertexTexCoordIndices,
    // pointer to list of face texture coordvertex-normal indices stored as [ijkl,ijk,ijkl,ijklmn,ijk,...] (same order as in "pFaceVertexIndices")
    unsigned int** pFaceVertexNormalIndices,
    // number of vertices in "pVertices"
    unsigned int* numVertices,
    // number of vertex normals in "pNormals"
    unsigned int* numNormals,
    // number of texture coordinates in "pTexCoords"
    unsigned int* numTexcoords,
    // number of faces
    unsigned int* numFaces);

/*
    Funcion to write out an obj file that stores a single 3D mesh object (in ASCII
    format).
*/
void mioWriteOBJ(
    // absolute path to file
    const char* fpath,
    // pointer to list of vertex coordinates stored as [xyz,xyz,xyz,...]
    double* pVertices,
    // pointer to list of vertex normals stored as [xyz,xyz,xyz...]
    double* pNormals,
    // pointer to list of texture coordinates list stored as [xy,xy,xy...]
    double* pTexCoords,
    // pointer to list of face sizes (number of vertices in each face) stored as [a,b,c,d,e,f,g,...]
    unsigned int* pFaceSizes,
    // pointer to list of face-vertex indices stored as [ijkl,ijk,ijkl,ijklmn,ijk,...]
    unsigned int* pFaceVertexIndices,
    // pointer to list of face-vertex texture-coord indices stored as [ijkl,ijk,ijkl,ijklmn,ijk,...] (same order as in "pFaceVertexIndices")
    unsigned int* pFaceVertexTexCoordIndices,
    // pointer to list of face texture coordvertex-normal indices stored as [ijkl,ijk,ijkl,ijklmn,ijk,...] (same order as in "pFaceVertexIndices")
    unsigned int* pFaceVertexNormalIndices,
    // number of vertices in "pVertices"
    unsigned int numVertices,
    // number of vertex normals in "pNormals"
    unsigned int numNormals,
    // number of texture coordinates in "pTexCoords"
    unsigned int numTexcoords,
    // number of faces (number of element in "pFaceSizes")
    unsigned int numFaces);

#ifdef __cplusplus
} // extern "C"
#endif // #ifdef __cplusplus

#endif // #ifndef __MIO_OBJ_H__