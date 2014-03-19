#include "MD5Animation.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define ERR_MSG(X) printf("In file: %s line: %d\n\t%s\n", __FILE__, __LINE__, X);

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
** Loads the joint hierachy. which is basically how joint information is stored.
*/
static int loadHierarchy(FxsMD5Animation* animation, FILE* file)
{
	char line[256];
	char pline[256];
	int scres = 0;
	char name[256];
	char pname[256]; /* name of the joint w/o the quotation marks */
	int parent = 0;
	int flags = 0;
	int frameIndex = 0;
	int count = 0;
	int i = 0, j = 0; /* loop vars */
	size_t len = 0;

	while (1)
	{
		if (readLine(line, sizeof(line), file))
		{
			ERR_MSG("unexpected end of line")
			return 0; 
		}
		
		processLine(pline, line);

		scres = sscanf(
				pline, 
				"%s %d %d %d",
				name,
				&parent,
				&flags,
				&frameIndex
			);

		/* if everything could be read, add the joint to the animation */		
		if (scres == 4) 
		{
			/* complain if the amount of joints exceeds the intially promised
			** value. 
			*/
			if (count >= animation->numJoints) 
			{
				ERR_MSG("Too many joints found")
				return 0;
			}		

			/*lets remove the quotation marks of the joints name */
			i = 0;
			j = 0;

			while (name[i] != '\0')
			{
				if (name[i] != '"')
				{
					pname[j] = name[i]; 
					j++;
				} 	
				i++;
			}
			
			pname[j] = '\0';			
			
			len = strlen(pname) + 1;	
			
			/* alloc some space for the joints name and copy */
			animation->joints[count].name = (char*)malloc(len);
			memcpy(animation->joints[count].name, pname, len);
			
			/*copy the rest */
			animation->joints[count].parent = parent;		    
			animation->joints[count].flags = flags;
			animation->joints[count].frameIndex = frameIndex;

			count++;
		}

		/* leave loop if we meet the closing bracket */
		if (pline[0] == '}')
		{
			break;
		}
	}

	/* complain if not enough joins could be loaded */
	if (animation->numJoints != count)
	{
	    ERR_MSG("Could not load enough joints");
		return 0;
	}

	return 1;
}

/*
** Load the bounds
*/ 
static int loadBounds(FxsMD5Animation* animation, FILE* file)
{
	char line[256];
	char pline[256];
	int scres = 0;
	FxsVector3 min;
	FxsVector3 max;
	int count = 0;

	/* read the bounds */
	while (1)
	{
		/* complain if we reach the end of file */
		if (readLine(line, sizeof(line), file)) 
		{
			ERR_MSG("Unexpected end of file")
			return 0;
		}
		
		processLine(pline, line);

		/* exit if we reach closing bracket */
		if (pline[0] == '}') 
		{
		    break;
		}

		/* read in the bound by bound */
		scres = sscanf(
				pline, 
				"( %f %f %f ) ( %f %f %f )",
				&min.x,
				&min.y,
				&min.z,
				&max.x,		
				&max.y,		
				&max.z		
			);	

		/* if bound was correctly read, store it */
		if (scres == 6) 
		{
		    /* complain if there are more bound than promised,
			** there is a bound for each frame in the animation.
			*/
			if (animation->numFrames <= count)
			{
				ERR_MSG("Too many bounds found") 
				return 0;
			}
			
			/* store bound for the frame */
			animation->bounds[count].min = min;	
			animation->bounds[count].max = max;	
			
			count++;
		}		

	}

	/* complain if not all bounds could be loaded */
	if (animation->numFrames != count) 
	{
		ERR_MSG("Could not load all bounds") 
		return 0;
	}

	return 1;
}

int loadBaseFrame(FxsMD5Animation* animation, FILE* file)
{
	char line[256];
	char pline[256];
	int scres = 0;
	FxsVector3 position;
	FxsVector3 orientation;
	int count = 0;

	while (1)
	{
		/* compl. when eof */
		if (readLine(line, sizeof(line), file))
		{
			ERR_MSG("Unexpected end of file")
			return 0;
		}

		processLine(pline, line);	
	
		/* end when we reach closing bracket */
		if (pline[0] == '}')
		{
			break;
		}
	
		scres = sscanf(
				pline, 
				"( %f %f %f ) ( %f %f %f )",
				&position.x,
				&position.y,
				&position.z,
				&orientation.x,
				&orientation.y,
				&orientation.z
			);

		if (6 == scres) 
		{
			/* cant load more than numJoints joints for baseframe */
			if (count >= animation->numJoints) 
			{
			    ERR_MSG("too many joints in base frame")
				return 0;	
			}			

			animation->baseFrame.positions[count] = position;
			
			FxsQuaternionMakeWithAxis(
				&animation->baseFrame.orientations[count],
				&orientation	
			);
			
			count++;
		}
	}

	/* we need numJoints joint pos/orientations ... */
	if (animation->numJoints != count)
	{
		ERR_MSG("Could not load all joints for base frame")
		return 0;
	}
	
	return 1;
}

/*
** loads data for a frame
*/ 
int loadFrame(FxsMD5Animation* animation, int frame, FILE* file)
{
    char line[256];
    char pline[256];
	size_t linelen;
	int count = 0;
	int i = 0;
	char* numStart;

	/* make sure we don't load more frames than numFrames */
	if (frame >= animation->numFrames) 
	{
		ERR_MSG("Too many frames")
	   	return 0; 
	}

	/* malloc memory for a frame's components */
	animation->frames[frame].data = (float*)malloc(
			sizeof(float)*animation->numAnimatedComponents
		);

	if (!animation->frames[frame].data) 
	{
	    ERR_MSG("malloc failed")
		return 0;
	}

	memset(
		animation->frames[frame].data,
		0,
		sizeof(float)*animation->numAnimatedComponents
	);

	/* read in frame */
	while (1)
	{
		readLine(line, sizeof(line), file);
		processLine(pline, line);	

		/* break if we reach closing bracket */
		if (pline[0] == '}')
		{
			break;
		}	

		/* read numbers of this line */
		i = 0;
		numStart = &pline[0];
		linelen = strlen(pline);

		while (1)
		{
			/* check if we are about to load more then numAnimatedComponents */
			if (count >= animation->numAnimatedComponents) 
			{
			    ERR_MSG("Too much frame components")
				return 0;
			}

			/* every time we find a space or the terminating zero, we read in
			** a number
			*/
			if (pline[i] == ' ' || pline[i] == '\0') 
			{
			    pline[i] = '\0';
				animation->frames[frame].data[count] = atof(numStart);
				numStart = pline + i + 1;
				count++;
			}
		
			/* break when we reached the end of the line */
			if (i == linelen) 
			{
			    break;
			}

			i++;
		}
	}
	
	/* complain if not enough components were loaded */
	if (animation->numAnimatedComponents != count) 
	{
	    ERR_MSG("Did not load enough frame components")
		return 0;
	}
	
	return 1;
}

/*
** Loads an animation from a file.
*/
int FxsMD5AnimationCreateWithFile(
	FxsMD5Animation** animation, 
	const char* filename
)
{
    FILE* file;
    char line[256];
    char pline[256];
    int isEOF = 0;
    int numFrames = 0; /* # of animation frames */
	int numJoints = 0; /* # of joints */
	int frameRate = 0; /* frame rate of the animation */
	int numAnimationedComponents; 
	int scres = 0;     /* result of sscanf */
	int frame = 0;
    int loadedFrames = 0;
    int success = 1;
    
    file = fopen(filename, "r");
    
    if (!file)
    {
        ERR_MSG("Could not open file")
        return 0;
    }
    
    *animation = (FxsMD5Animation*)malloc(sizeof(FxsMD5Animation));
	memset(*animation, 0, sizeof(animation));
    
    if (!*animation)
    {
        ERR_MSG("could not alloc animation")
        return 0;
    }
    
    /* load the animation from file */
    while (1)
    {
        isEOF = readLine(line, sizeof(line), file);
        processLine(pline, line);
        
        if (strstr(pline, "numFrames") == pline) /* # of frames */
        {
        	scres = sscanf(pline, "numFrames %d", &numFrames);	    
			
			if (scres == 1) 
			{
				(*animation)->numFrames = numFrames; 
				
				/* alloc and memset animation frames, same for bounds. */
				(*animation)->frames = (FxsMD5AnimationFrame*)malloc(
						numFrames*sizeof(FxsMD5AnimationFrame)
					);
				
				if (!(*animation)->frames) 
				{
				    ERR_MSG("malloc failed")
                    success = 0;
                    break;
				}

				memset(
					(*animation)->frames, 
					0, 
					numFrames*sizeof(FxsMD5AnimationFrame)
				);
		
				(*animation)->bounds = (FxsMD5AnimationBound*)malloc(
						numFrames*sizeof(FxsMD5AnimationBound)
					);
        
				if (!(*animation)->bounds) 
				{
				    ERR_MSG("malloc failed")
                    success = 0;
                    break;
				}

				memset(
					(*animation)->bounds,
					0,
					numFrames*sizeof(FxsMD5AnimationBound)
				);
			}
        }
		else if (strstr(pline, "numJoints") == pline)  /* # of joints */
		{
			scres = sscanf(pline, "numJoints %d", &numJoints);
			
			if (scres == 1) 
			{
			  	(*animation)->numJoints = numJoints; 


				/* alloc and init joints */
				(*animation)->joints = (FxsMD5AnimationJoint*)malloc(
						numJoints*sizeof(FxsMD5AnimationJoint)
					);
				
			    if (!(*animation)->joints) 
			    {
			        ERR_MSG("alloc failed")
                    success = 0;
                    break;
			    }		
		
				memset(
					(*animation)->joints, 
					0, 
					numJoints*sizeof(FxsMD5AnimationJoint)
				);

				/* alloc mem for joint data in the base frame */
				(*animation)->baseFrame.positions = (FxsVector3*)malloc(
						numJoints*sizeof(FxsVector3)
					); 

				(*animation)->baseFrame.orientations = (FxsQuaternion*)malloc(
						numJoints*sizeof(FxsQuaternion)
					); 
				
				if (!(*animation)->baseFrame.positions
				|| !(*animation)->baseFrame.orientations) 
				{
				    ERR_MSG("alloc failed")
                    success = 0;
                    break;
				}
				
				memset(
					(*animation)->baseFrame.positions, 
					0, 
					sizeof(FxsVector3)*numJoints
				);

				memset(
					(*animation)->baseFrame.orientations, 
					0, 
					sizeof(FxsQuaternion)*numJoints
				);
			}
		
		} 
		else if (strstr(pline, "frameRate") == pline) /* frame rate */
		{
			scres = sscanf(pline, "frameRate %d", &frameRate);	
		
			if (scres == 1) 
			{
			    (*animation)->frameRate = frameRate;
			}
		}
		else if (strstr(pline, "numAnimatedComponents") == pline) /* # animated components */ 
		{
			scres = sscanf(
					pline, 
					"numAnimatedComponents %d", 
					&numAnimationedComponents
				);
			
			if (scres == 1) 
			{
				(*animation)->numAnimatedComponents = numAnimationedComponents;
			}
		}
		else if (strstr(pline, "hierarchy {") == pline) /* joint hierarchy */
		{
			if (!loadHierarchy(*animation, file))
			{
			    success = 0;
				break;
			} 
		}
		else if (strstr(pline, "bounds {") == pline) /* bounds */
		{
			if (!loadBounds(*animation, file))
			{
			    success = 0;
				break;
			}
		}
		else if (strstr(pline, "baseframe {") == pline) /* base frame */
		{
			if (!loadBaseFrame(*animation, file))
			{
			    success = 0;
				break;
			}
		}
		else if (sscanf(pline, "frame %d {", &frame) == 1)
		{
            /* NOTE frame should occur more than one time,
            ** this check was not implemented.
            */
			if (!loadFrame(*animation, frame, file)) 
			{
			    success = 0;
				break;
			}
            else
            {
                loadedFrames++;
            }
		}
        
        if (isEOF)
        {
            break;
        }
        
    }
    
    /* complain if not all frames were loaded*/
    if (loadedFrames != numFrames)
    {
        ERR_MSG("not enough frames loaded");
        success = 0;
    }
    
    /* clean up */
    fclose(file);

    if (!success)
    {
        FxsMD5AnimationDestroy(animation);
        return 0;
    }
    
    return 1;
}

void FxsMD5AnimationDestroy(FxsMD5Animation** animation)
{
    int  i = 0;
    FxsMD5AnimationFrame* frame;
    
    if (!(*animation))
    {
        return;
    }

    /* delete all frames */
    if ((*animation)->frames)
    {
        for (i = 0; i < (*animation)->numFrames; i++)
        {
            frame = (*animation)->frames + i;
        
            if (frame)
            {
                if (frame->data)
                {
                    free(frame->data);
                }
            }
        }
        
        free((*animation)->frames);
    }

    /* delete base frame data */
    if ((*animation)->baseFrame.positions)
    {
        free((*animation)->baseFrame.positions);
    }
    
    if ((*animation)->baseFrame.orientations)
    {
        free((*animation)->baseFrame.orientations);
    }
    
    /* delete all animation joints */
    if ((*animation)->joints)
    {
        for (i = 0; i < (*animation)->numJoints; i++)
        {
            if ((*animation)->joints[i].name)
            {
                free((*animation)->joints[i].name);
            }
        }
        
        free((*animation)->joints);
    }

    /* delete bounds */
    if ((*animation)->bounds)
    {
        free((*animation)->bounds);
    }
    
    /* delete the animation */
    free(*animation);
    animation = NULL;
}