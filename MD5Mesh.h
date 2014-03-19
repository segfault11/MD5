#ifndef MD5MESH_H
#define MD5MESH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <Fxs/Math/Vector2.h>
#include <Fxs/Math/Vector3.h>
#include <Fxs/Math/Quaternion.h>
#include <Fxs/Math/Matrix4.h>
#include "MD5Animation.h"

/*
** Submeshes contain faces that index vertices in the submesh
*/ 
typedef struct
{
	int id; 			/* id of the face */
	unsigned int v1;    /* id of the first vertex */
	unsigned int v2; 	/* id of the second vertex */
	unsigned int v3; 	/* id of the third vertex */
}
FxsMD5Face;

typedef struct
{
	char* name; 			/* name of the joint */
	int parent; 			/* id to the parent of this joint */
	FxsVector3 position;    /* position of the joint relative to its parent */
	FxsQuaternion orientation; /* orientation relative to the parent */
	FxsMatrix4 transform;   /* world transform of this joint */
}
FxsMD5Joint;

typedef struct 
{
	int id;
	FxsVector2 texCoords;
	int weightId;           /* id of the first weight in the weights array of
                            ** the submesh this vertex belongs to.
                            */
	int numWeights;         /* # of weights that define the vertex position */
}
FxsMD5Vertex;

typedef struct
{
	int id;                 /* id of this weight */
	int jointId;            /* joint associated with this weight */
	float value;            /* values of this weight */
	FxsVector3 position;    /* position of the weight */
}
FxsMD5Weight;

typedef struct
{
	int numJoints;
	FxsMD5Joint* joints;
}
FxsMD5Skeleton;

typedef struct
{
	int numVertices;
	int numFaces;
	int numWeights;
	int texIndex;

    char* shader;               /* file name for this meshes shader */

	FxsMD5Face* faces;
	FxsMD5Weight* weights;
	FxsMD5Vertex* vertices;
}
FxsMD5SubMesh;

typedef struct 
{
	//char** jointNames;
	unsigned int currentAnimationFrame;
	unsigned int numSubMeshes;  		 /* # of submeshes */
    
    FxsMD5Skeleton bindPose;
    FxsMD5Skeleton currentPose;
    FxsMD5SubMesh* meshes;
}
FxsMD5Mesh;

/*
** Loads a MD5 mesh from a file. Returns 0 if it fails to load.
*/ 
int FxsMD5MeshCreateWithFile(FxsMD5Mesh** mesh, const char* filename);

/*
** Releases the MD5 mesh.
*/ 
void FxsMD5MeshDestroy(FxsMD5Mesh** mesh);

/*
** Updates the current pose of a mesh with the frame of an animation
** Returns 0 if it fails, otherwise 1.
*/ 
int FxsMD5MeshUpdatePoseWithAnimationFrame(
	FxsMD5Mesh* mesh,
	const FxsMD5Animation* animation,
	unsigned int frame
);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: MD5MESH_H */
