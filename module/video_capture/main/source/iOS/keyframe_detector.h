/*@file keyframe_detector.h 
* @author Guoqing Zeng
* @date   20160526
* @description: realize fast moving shots by similarity compute.
* @version: Create!@20150625
*/
#ifndef	__KEYFRAMEDETECT_H__
#define __KEYFRAMEDETECT_H__

typedef struct
{
	int width;
	int height;

	int firstframe;

	unsigned char* tmpBuf;
	unsigned char* currBuf;
	unsigned char* prevBuf;

	int initFlag;
}KeyFrameDetectCore;

//
int CreateKeyFrameDetect(KeyFrameDetectCore** self);

int KeyFrameDetectInitCore(KeyFrameDetectCore* self, 
	                       int width,
						   int height);

// return value
// -1 for error
// 0 for not keyframe
// 1 for keyframe
int KeyFrameDetectProcess(KeyFrameDetectCore*self,
	                      int stepy,
						  unsigned char* pyBuf,
						  int stepu,
						  unsigned char* puBuf,
						  int stepv,
						  unsigned char* pvBuf,
						  void* flog = 0);

int KeyFrameDetectFree(KeyFrameDetectCore*self);

#endif