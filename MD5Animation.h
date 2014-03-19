#ifndef MD5ANIMATION_H
#define MD5ANIMATION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <Fxs/Math/Vector3.h>
#include <Fxs/Math/Quaternion.h>

/*
** Bitflags that indicate the components of the Animation joint.
*/
#define FXS_MD5_ANIM_XPOS   1
#define FXS_MD5_ANIM_YPOS	2
#define FXS_MD5_ANIM_ZPOS	4
#define FXS_MD5_ANIM_XQUAT	8
#define FXS_MD5_ANIM_YQUAT	16
#define FXS_MD5_ANIM_ZQUAT	32

typedef struct
{
    char* name;   /* name of the joint */
    int parent;
    int flags;           /* identifies which components of the joint change */
    int frameIndex;
}
FxsMD5AnimationJoint;

/*
** Bounding box for each frame
*/ 
typedef struct  
{
	FxsVector3 min;    
	FxsVector3 max;    
}
FxsMD5AnimationBound;

typedef struct
{
    FxsVector3* positions;           /* positions of the base frame */
    FxsQuaternion* orientations;     /* orientations of the base frame */
}
FxsMD5AnimationBaseFrame;

typedef struct
{
    float* data;    /* information about the joint update for a frame */
}
FxsMD5AnimationFrame;

typedef struct
{
    unsigned int frameRate;
    unsigned int numJoints;
    unsigned int numFrames;
    unsigned int numAnimatedComponents;

    FxsMD5AnimationFrame* frames;
    FxsMD5AnimationBaseFrame baseFrame;
    FxsMD5AnimationJoint* joints;
	FxsMD5AnimationBound* bounds;
}
FxsMD5Animation;


int FxsMD5AnimationCreateWithFile(FxsMD5Animation** animation, const char* filename);
void FxsMD5AnimationDestroy(FxsMD5Animation** animation);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: MD5ANIMATION_H */
