 /*
** some notes:
** - FxsMD5Joint might not need position, orientation and parent member, as
** 	 as this information is already stored for the corresponding 
**   FxsMD5AnimationJoint
*/ 


#include "MD5Mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_MSG(X) printf("In file: %s line: %d\n\t%s\n", __FILE__, __LINE__, X);

/*
** Conversation matrix to OpenGL camera space.
*/
static FxsMatrix4 conversation = {
        -1.0, 0.0, 0.0, 0.0,
         0.0, 0.0, 1.0, 0.0,
         0.0, 1.0, 0.0, 0.0,
         0.0, 0.0, 0.0, 1.0
    };

/*
** read a line from a file. return 1 in case of EOF.
*/ 
static int readLine(char* line, int maxChars, FILE* file)
{
	char c;
	int numChars = 0;

	do
	{
		c = fgetc(file);
		line[numChars] = c;
		numChars++;
	} 
	while (c != EOF && c != '\r' && c != '\n' && numChars < maxChars - 1);

    /* finish the string */
	line[numChars - 1] = '\0';
    
    
    /* swallow the line feed as well */
    if (c == '\r')
    {
        c = fgetc(file);
    }
    
    /* check for end of file */
    if (c == EOF)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*
** Trims the line and removes comments. replace tabs with spaces and 
** dont allow more than two spaces in a row.
*/
static void processLine(char* pline, const char* line)
{
    int i = 0;
    int l = 0;
    char* c;
    int isPrevSpace = 0;
    
    /* find first non blank char in the line */
    while (line[i] == ' ' || line[i] == '\t')
    {
        i++;
    }
    
    /* cut line where comment is found */
    c = strstr(line, "//");
    
    if (c)
    {
        *c = '\0';
    }
    
    /* replace tabs and no more two or more spaces in a row */
    while (line[i] != '\0')
    {
        if ((line[i] == '\t' || line[i] == ' ') && !isPrevSpace)
        {
            pline[l] = ' ';
            l++;
            isPrevSpace = 1;
        }
        else
        {
            isPrevSpace = 0;
            pline[l] = line[i];
            l++;
        }
        
        i++;
    }
    
    /* make sure: no spaces at the end of the line*/
    if (l > 1 && pline[l - 1] == ' ')
    {
        pline[l-1] = '\0';
    }
    else
    {
        pline[l] = '\0';
    }
}

/*
** Loads joints, returns 1 if everything was okay 0 otherwise/
*/
static int loadJoints(FxsMD5Mesh* mesh, FILE* file)
{
    int i = 0, j = 0;               /* loop var */
    char line[256];
    char pline[256];
    char name[256];
    char pname[256];                /* name of the joints w/o quotation marks*/
    size_t nameLength;
    FxsVector3 position;
    FxsVector3 orientationAxis;
    int parent;
    int scres;                  /* result of sscanf */
    int loaded = 0;             /* counts loaded joints */
    FxsMatrix4 trans;
    FxsMatrix4 rot;
    FxsMatrix4 temp;

    /* read in joints */
    while (1)
    {
        /* read in line */
        if (readLine(line, sizeof(line), file))
        {
            /* if file end unexpectedly, complain ... */
            ERR_MSG("File ended unexpectedly");
            return 0;
        }
        
        processLine(pline, line);
        
        /* scan line for joint data */
        scres = sscanf(
                pline,
                "%s %d ( %f %f %f ) ( %f %f %f )",
                name,
                &parent,
                &position.x,
                &position.y,
                &position.z,
                &orientationAxis.x,
                &orientationAxis.y,
                &orientationAxis.z
            );
        
        /* if full data was found copy it to our mesh's bindPose */
        if (scres == 8)
        {
            if (loaded >= mesh->bindPose.numJoints)
            {
                ERR_MSG("Too many joints found");
                return 0;
            }
            
            nameLength = strlen(name);
        
            /* remove quotation marks of name*/
            j = 0;
            for (i = 0; i < nameLength; i++)
            {
                if (name[i] != '"')
                {
                    pname[j] = name[i];
                    j++;
                }
            }

            pname[j] = '\0';
            nameLength = strlen(pname);

            /* copy scanned data to joint*/
            mesh->bindPose.joints[loaded].name = (char*)malloc(nameLength + 1);
            memcpy(mesh->bindPose.joints[loaded].name, pname, nameLength + 1);
            mesh->bindPose.joints[loaded].parent = parent;
            mesh->bindPose.joints[loaded].position = position;
            
            /* make the quaternion for the rotation of the joint*/
            if (!FxsQuaternionMakeWithAxis(
                &mesh->bindPose.joints[loaded].orientation,
                &orientationAxis)
            )
            {
                ERR_MSG("Invalid quaternion axis");
                return 0;
            }
            
            /* make the transformation matrix and copy to joint */
            FxsMatrix4MakeTranslation(&trans, position.x, position.y, position.z);
            FxsMatrix4MakeRotationWithQuaternion(
                &rot,
                &mesh->bindPose.joints[loaded].orientation
            );

            /* I guess its rotation first then translation ... */
            FxsMatrix4Multiply(&temp, &trans, &rot);

            /* store the transform but don't forget to convert it
            ** to opengl space.
            */
            FxsMatrix4Multiply(
                &mesh->bindPose.joints[loaded].transform,
                &conversation,
                &temp
            );

            
            loaded++;
        }
        
        /* stop when we reach the closing curly brace*/
        if (pline[0] == '}')
        {
            break;
        }
    }
    
    /* check if the file lists the correct amount of joints */
    if (loaded != mesh->bindPose.numJoints)
    {
        ERR_MSG("Invalid number of joints found");
        return 0;
    }
    
    return 1;
}

static int loadSubMeshes(FxsMD5SubMesh* mesh, FILE* file)
{
    char line[256];
    char pline[256];
    char str[256];
    size_t len;
    int scres;
    
    /* vertex data */
    int numVertices = 0;
    int loadedVertices = 0;
    int vertId;
    FxsVector2 texCoord;
    int weightIdVert;
    int numWeightsVert;
    
    /* face data */
    int numTriangles = 0;
    int loadedTriangles = 0;
    int triId;
    int v1, v2, v3;
    
    /* weights data */
    int loadedWeights = 0;
    int numWeights = 0;
    int weightId;
    int joinIdWeight;
    float weightValue;
    FxsVector3 weightPos;
    
    
    /* read in the submesh mesh */
    while (1)
    {
    
        /* read in line */
        if (readLine(line, sizeof(line), file))
        {
            /* if file end unexpectedly, complain ... */
            ERR_MSG("File ended unexpectedly");
            return 0;
        }
        
        processLine(pline, line);
        
        /* read the shader filename */
        scres = sscanf(pline, "shader %s", str);

        if (scres == 1)
        {
            len = strlen(str);
            
            mesh->shader = (char*)malloc(len + 1);
            memcpy(mesh->shader, str, len + 1);
            
            continue;
        }
        
        /* read the # of vertices for this submesh, and malloc */
        scres = sscanf(pline, "numverts %d", &numVertices);
        
        if (scres == 1)
        {
            mesh->numVertices = numVertices;
            mesh->vertices = (FxsMD5Vertex*)malloc(sizeof(FxsMD5Vertex)*numVertices);
        
            if (!mesh->vertices)
            {
                ERR_MSG("malloc failed");
                return 0;
            }
            
            continue;
        }
        
        /* read each vertex */
        scres = sscanf(
                pline,
                "vert %d ( %f %f ) %d %d",
                &vertId,
                &texCoord.x,
                &texCoord.y,
                &weightIdVert,
                &numWeightsVert
            );
        
        if (scres == 5)
        {
            /* check for array overflow */
            if (loadedVertices >= mesh->numVertices)
            {
                ERR_MSG("Too many vertices found")
                return  0;
            }
        
            mesh->vertices[loadedVertices].id = vertId;
            mesh->vertices[loadedVertices].texCoords = texCoord;
            mesh->vertices[loadedVertices].weightId = weightIdVert;
            mesh->vertices[loadedVertices].numWeights = numWeightsVert;
            
            loadedVertices++;
            
            continue;
        }
    
        /* read the number of triangles */
        scres = sscanf(pline, "numtris %d", &numTriangles);
        
        if (scres == 1)
        {
            mesh->numFaces = numTriangles;
            mesh->faces = (FxsMD5Face*)malloc(sizeof(FxsMD5Face)*numTriangles);

            if (!mesh->faces)
            {
                ERR_MSG("malloc failed");
                return 0;
            }
            
            continue;
        }

        /* read triangles */
        scres = sscanf(
                pline,
                "tri %d %d %d %d",
                &triId,
                &v1,
                &v2,
                &v3
            );
        
        if (scres == 4)
        {
            /* check for array overflow */
            if (loadedTriangles >= mesh->numFaces)
            {
                ERR_MSG("Too many vertices found")
                return  0;
            }
        
            mesh->faces[loadedTriangles].id = triId;
            mesh->faces[loadedTriangles].v1 = v1;
            mesh->faces[loadedTriangles].v2 = v2;
            mesh->faces[loadedTriangles].v3 = v3;
            
            loadedTriangles++;
        }

        /* read # of weights */
        scres = sscanf(pline, "numweights %d", &numWeights);
        
        if (scres == 1)
        {
            mesh->numWeights = numWeights;
            mesh->weights = (FxsMD5Weight*)malloc(sizeof(FxsMD5Weight)*numWeights);

            if (!mesh->weights)
            {
                ERR_MSG("malloc failed");
                return 0;
            }
            
            continue;
        }

        /* read weights */
        scres = sscanf(
                pline,
                "weight %d %d %f ( %f %f %f )",
                &weightId,
                &joinIdWeight,
                &weightValue,
                &weightPos.x,
                &weightPos.y,
                &weightPos.z
            );
        
        if (scres == 6)
        {
            /* overflow check */
            if (loadedWeights >= mesh->numWeights)
            {
                ERR_MSG("Too many weights found");
                return 0;
            }
            
            mesh->weights[loadedWeights].id = weightId;
            mesh->weights[loadedWeights].jointId = joinIdWeight;
            mesh->weights[loadedWeights].value = weightValue;
            mesh->weights[loadedWeights].position = weightPos;
            
            loadedWeights++;
        }

        /* stop when we reach the closing curly brace*/
        if (pline[0] == '}')
        {
            break;
        }
    }
    
    /* check if all vertices, weights and triangles were loaded */
    if (numVertices != loadedVertices
    || numTriangles != loadedTriangles
    || numWeights != loadedWeights)
    {
        ERR_MSG("Invalid number of vertices, triangles or weights");
        return 0;
    }

    return 1;
}

/*
** Loads the mesh from a file. Returns 0, if it fails.
*/ 
int FxsMD5MeshCreateWithFile(FxsMD5Mesh** mesh, const char* filename)
{
	FILE* file;
    char line[256];
    char pline[256];
    int isEOF;
    int numJoints = 0; /* # of joints */
    int numMeshes = 0; /* # of meshes */
    int loadedMeshes = 0;
    int success = 1;

	*mesh = (FxsMD5Mesh*)malloc(sizeof(FxsMD5Mesh));	

	if (!mesh) 
	{
	    return 0;
	}
    
    /* init mesh to zero */
    memset(*mesh, 0, sizeof(FxsMD5Mesh));
	
	file = fopen(filename, "r");

	if (!file) 
	{
        ERR_MSG("Could not open file")
		free(*mesh);
	    return 0;
	}

	/* load the file ...  */
	while (1)
	{
        isEOF = readLine(line, sizeof(line), file);
        processLine(pline, line);
        
        /* find the number of joints */
        if (strstr(pline, "numJoints") == pline)
        {
            sscanf(pline, "numJoints %d", &numJoints);
            
			/* alloc and init the bind pose */
			(*mesh)->bindPose.joints = (FxsMD5Joint*)malloc(sizeof(FxsMD5Joint)*numJoints);
            (*mesh)->bindPose.numJoints = numJoints;
            
            /* in case allocation fails */
            if (!(*mesh)->bindPose.joints)
            {
                success = 0;
                break;
            }
            
            /* default init the joints of the bind pose to zero*/
            memset((*mesh)->bindPose.joints, 0, sizeof(FxsMD5Joint)*numJoints);

			/* alloc and init the current pose */
            (*mesh)->currentPose.joints = (FxsMD5Joint*)malloc(sizeof(FxsMD5Joint)*numJoints);
            (*mesh)->currentPose.numJoints = numJoints;
            
            /* in case allocation fails */
            if (!(*mesh)->currentPose.joints)
            {
                success = 0;
                break;
            }
            
            /* default init the joints of the current pose to zero*/
            memset((*mesh)->currentPose.joints, 0, sizeof(FxsMD5Joint)*numJoints);
        }
        
        /* find the number of meshes */
        if (strstr(pline, "numMeshes") == pline)
        {
            sscanf(pline, "numMeshes %d", &numMeshes);
            (*mesh)->meshes = (FxsMD5SubMesh*)malloc(sizeof(FxsMD5SubMesh)*numMeshes);
            (*mesh)->numSubMeshes = numMeshes;
            
            /* in case allocation fails */
            if (!(*mesh)->meshes)
            {
                success = 0;
                break;
            }
            
            /* default init all sub-meshes to zero*/
            memset((*mesh)->meshes, 0, sizeof(FxsMD5SubMesh)*numMeshes);
        }
        
        /* load all joints */
        if (strstr(pline, "joints {") == pline)
        {
            if (!loadJoints(*mesh, file))
            {
                success = 0;
                break;
            }

			/* copy the bind pos data to the current pos */
			memcpy(
				(*mesh)->currentPose.joints, 
				(*mesh)->bindPose.joints, 
				sizeof(FxsMD5Joint)*(*mesh)->bindPose.numJoints
			);
        }
        
        /* load all sub meshes */
        if (strstr(pline, "mesh {") == pline)
        {
            if (!loadSubMeshes(&(*mesh)->meshes[loadedMeshes], file))
            {
                success = 0;
                break;
            }
            else
            {
                loadedMeshes++;
            }
        }
        
        /* quit if eof */
        if (isEOF)
        {
            break;
        }
	}
    
    /* check if all meshes were loaded */
    if (loadedMeshes != numMeshes)
    {
        ERR_MSG("Failed to load all sub-meshes");
        success = 0;
    }

	/* clean up */
	fclose(file);

    if (!success)
    {
        FxsMD5MeshDestroy(mesh);
        return 0;
    }

	return 1;
}

/*
**  Releases an MD5 Mesh
*/
void FxsMD5MeshDestroy(FxsMD5Mesh** mesh)
{
    int i = 0;
    
    if (!*mesh)
    {
        return;
    }
    
    /* release submeshes */
    for (i = 0; i < (*mesh)->numSubMeshes; i++)
    {
        if ((*mesh)->meshes)
        {
            FxsMD5Face* faces = (*mesh)->meshes[i].faces;
            FxsMD5Vertex* vertices = (*mesh)->meshes[i].vertices;
            FxsMD5Weight* weights = (*mesh)->meshes[i].weights;

            if (faces)
            {
                free(faces);
            }
            
            if (vertices)
            {
                free(vertices);
            }
            
            if (weights)
            {
                free(weights);
            }
        }
    }
    
    if ((*mesh)->bindPose.joints)
    {
        free((*mesh)->bindPose.joints);
    }

	if ((*mesh)->currentPose.joints) 
	{
	    free((*mesh)->currentPose.joints);
	}
    
    free(*mesh);
    
    *mesh = NULL;
}

/*
** Updates the skeleton of the mesh (the currentSkeleton) according to an 
** animation frame. To do so we determine the transformation for each animation
** joint and copy the transformation (along with its local position and 
** orientation) to the corresponding joint of the currentSkeleton of the mesh.
*/ 
int FxsMD5MeshUpdatePoseWithAnimationFrame(
	FxsMD5Mesh* mesh,
	const FxsMD5Animation* animation,
	unsigned int frame
)
{
	FxsMD5AnimationJoint* animJoint = NULL;
	FxsMD5AnimationFrame* animFrame = NULL;
	FxsVector3 position;
	FxsVector3 orientationAxis;
    float orientationAxisMagnitude = 0.0;
	FxsMatrix4 orientationMatrix;
	FxsMatrix4 translationMatrix;
	FxsMatrix4 transformationMatrix;
	int i = 0, j = 0;

	if (animation->numJoints != mesh->currentPose.numJoints) 
	{
	    /* animation does not apply to this mesh ... */
		return 0;
	}

	animFrame = &animation->frames[frame];

	for (i = 0; i < animation->numJoints; i++)
	{
        j = 0;
		animJoint = &animation->joints[i];
		position = animation->baseFrame.positions[i];
		orientationAxis.x = animation->baseFrame.orientations[i].x;
		orientationAxis.y = animation->baseFrame.orientations[i].y;
		orientationAxis.z = animation->baseFrame.orientations[i].z;

		if (animJoint->flags & FXS_MD5_ANIM_XPOS) 
		{
		    position.x = animFrame->data[animJoint->frameIndex + j];
			j++;
		}		
		
		if (animJoint->flags & FXS_MD5_ANIM_YPOS) 
		{
		    position.y = animFrame->data[animJoint->frameIndex + j];
			j++;
		}		
	
		if (animJoint->flags & FXS_MD5_ANIM_ZPOS) 
		{
		    position.z = animFrame->data[animJoint->frameIndex + j];
			j++;
		}		

		if (animJoint->flags & FXS_MD5_ANIM_XQUAT)
		{
		    orientationAxis.x = animFrame->data[animJoint->frameIndex + j];
			j++;
		}

		if (animJoint->flags & FXS_MD5_ANIM_YQUAT)
		{
		    orientationAxis.y = animFrame->data[animJoint->frameIndex + j];
			j++;
		}

		if (animJoint->flags & FXS_MD5_ANIM_ZQUAT)
		{
		    orientationAxis.z = animFrame->data[animJoint->frameIndex + j];
			j++;
		}

		/* update the joint for the current skeleton */
		mesh->currentPose.joints[i].parent = animJoint->parent;
		mesh->currentPose.joints[i].position = position;

        FxsVector3Length(&orientationAxisMagnitude, &orientationAxis);

        //
        // TODO is the following correct??
        //
    
        if (orientationAxisMagnitude >= 1.0)
        {
            /* if the magnitude of the orientation axis is greater than one
            ** only the orientation axis changes not and the angle of rotation
            ** is zero.
            */
            FxsVector3Normalize(&orientationAxis);

            FxsQuaternionMake(
                &mesh->currentPose.joints[i].orientation,
                orientationAxis.x,
                orientationAxis.y,
                orientationAxis.z,
                0.0
            );
        }
        else
        {
            FxsQuaternionMakeWithAxis(
                &mesh->currentPose.joints[i].orientation,
                &orientationAxis
            );
        }

		FxsMatrix4MakeRotationWithQuaternion(
			&orientationMatrix, 
			&mesh->currentPose.joints[i].orientation
		);

		FxsMatrix4MakeTranslation(
			&translationMatrix, 
			position.x, 
			position.y, 
			position.z
		); 

		FxsMatrix4Multiply(
			&transformationMatrix, 
			&translationMatrix, 
			&orientationMatrix
		); 
		
		if (animJoint->parent < 0) 
		{
            mesh->currentPose.joints[i].transform = transformationMatrix;
        
			FxsMatrix4Multiply(
				&mesh->currentPose.joints[i].transform,
				&conversation,
				&transformationMatrix
			);
		}
		else
		{
			/* - converstation matrix is already part of the parents transform
			**   and hence does not need to be multiplied anymore 
			** - parents transform is assumed to be already computed before
			**   this joint ...
			*/
			FxsMatrix4Multiply(
				&mesh->currentPose.joints[i].transform,
				&mesh->currentPose.joints[animJoint->parent].transform,
				&transformationMatrix
			);
		}

	}
	
	return 1;
}
