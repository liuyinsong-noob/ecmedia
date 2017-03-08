
/*@file video_beauty_filter.h
  @author Guoqing Zeng
  @date 20170308
  @description:
          -beauty algorithm based on surface blur filter.
		  -used log curve adjusting to realize whitening algorithm.
		  -used multiple frames to stable frame and de-noise.
  @version:   
          -"Beauty_Filter_v_20170308_1.0"
*/
#ifndef __VIDEO_BEAUTY_FILTER_H__
#define __VIDEO_BEAUTY_FILTER_H__



#ifdef __cplusplus
extern "C" {
#endif

	//Return:
	//the version of the filter
	unsigned char* Get_Version_Beauty_Filter();


	//Input/Output:
	// *|inst| instance of the filter 
	//Return:
	//0 for success, -1 for failed
	int Create_Beauty_Filter(void** inst);


	//Input:
	// *|self| instance of the filter
	// *|radius| radius of the filter, in [1, 100]
	// *|thres| the threshold of the filter, in[1, 40]
	//Return:
	//0 for success, -1 for failed 
	int Init_Beauty_Filter(void* self, unsigned char radius, unsigned char thres);


	//Input:
	// *|self| instance of the filter
	// *|radius| radius of the filter, in [1, 100]
	// *|thres| the threshold of the filter, in[1, 40]
	//Return:
	//0 for success, -1 for failed
	int Set_Radius_Thres(void* self, unsigned char radius, unsigned char thres);


	//Input:
	// *|self| instance of the filter
	// *|enable| enables the whitening
	//Return:
	//0 for success, -1 for failed
	int Enable_Whitening(void* self, int enable);


	//Input:
	// *|self| instance of the filter
	// *|w| width of the image
	// *|h| height of the image
	// *|scan| the pitch of the image, in bytes
	// *|psrdst| the buf of be filtered, also store the filtered image
	// *|maskscan| the pitch of the mask, in bytes
	// *|mask| the buf of the mask
	//Output:
	// *|psrdst| the buf of be filtered, also store the filtered image
	//Return:
	//0 for success, -1 for failed
	int Proc_Beauty_Filter(void* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrcdst, int maskscan, unsigned char* mask);


	//Input:
	// *|self| instance of the filter to be free
	//Return:
	//0 for success, -1 for failed
	int Free_Beauty_Filter(void* self);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !__VIDEOFILTER_H__
