/* @file: ipl_color.h
*  @author: Guoqing Zeng
*  @date: 20160826
*  @description: support the color space conversion
*                yuv420<->rgb  yuv420p<->rgb
*                rgb<->hsip yuv420<->hsip yuv420p<->hsip
*  @version:
*         -zipl_yuv420p_rgb
*         -zipl_yuv420_rgb
*		  -zipl_rgb_yuv420p
*		  -zipl_rgb_yuv420
*/
#ifndef __ZIPL_COLOR_H__
#define __ZIPL_COLOR_H__

#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////
//  yuv420p_rgb    yuv420_rgb
//  rgb_yuv420p    rgb_yuv420
//////////////////////////////////////////////////////////////////////////
int zipl_yuv420p_rgb( int w,
	                  int h,
					  unsigned char* pyuv,
					  int scan_rgb,
					  unsigned char* prgb
					 );

int zipl_yuv420_rgb( int w, int h,
	                 int scany, unsigned char* py,
	                 int scanu, unsigned char* pu,
	                 int scanv, unsigned char* pv,
	                 int scan_rgb,unsigned char* prgb
	                );

int zipl_rgb_yuv420p( int w,
	                  int h,
					  int scan_rgb,
					  unsigned char* prgb,
					  unsigned char* pyuv
					 );

int zipl_rgb_yuv420( int w, int h,
	                 int scan_rgb, unsigned char* prgb,
	                 int scany, unsigned char* py,
					 int scanu, unsigned char* pu,
					 int scanv, unsigned char* pv
	                 );



//////////////////////////////////////////////////////////////////////////
//    yuv420p_hsip      yuv420_hsip
//    hsip_yuv420p      hsip_yuv420
//    hsi, h is float type, s i is int type  h [0, 360], s i [0, 255]
/////////////////////////////////////////////////////////////////////////

int zipl_yuv420p_hsip( int w, 
	                   int h,
	                   unsigned char* pyuv,
					   float* phsi
					  );

int zipl_yuv420_hsip( int w, int h,
	                  int scany, unsigned char* py,
	                  int scanu, unsigned char* pu,
	                  int scanv, unsigned char* pv,
	                  float* phsi
	                 );

int zipl_hsip_yuv420p( int w, 
	                   int h,
	                   float* phsi,
	                   unsigned char* pyuv
	                  );

int zipl_hsip_yuv420( int w, int h,
	                  float* phsi,
					  int scany, unsigned char* py,
					  int scanu, unsigned char* pu,
					  int scanv, unsigned char* pv
					 );



//////////////////////////////////////////////////////////////////////////
///   rgb_hsip      rgb_hsip_fast
///   hsip_rgb      hsip_rgb_fast
//    hsi, h for float type, s i for int type.   
//////////////////////////////////////////////////////////////////////////

int zipl_rgb_hsip( int w, 
	               int h,
				   int scan_rgb,
				   unsigned char* prgb,
				   float* phsi
				  );

int zipl_rgb_hsip_fast( int w, 
	                    int h,
	                    int scan_rgb,
	                    unsigned char* prgb,
	                    float* phsi
	                   );

int zipl_hsip_rgb( int w, 
	               int h,
	               float* phsi,
	               int scan_rgb,
				   unsigned char* prgb
	              );

int zipl_hsip_rgb_fast( int w, 
	                    int h,
	                    float* phsi,
	                    int scan_rgb, 
                        unsigned char* prgb
	                   );

//
int zipl_scale_4in1_rgb(int w, int h, int scan, unsigned char* rgb);


//
int get_skin_mask_rgb(int w, int h, int scanrgb, unsigned char* prgb, int scanmask, unsigned char* mask);
int get_skin_mask_hsip(int w, int h, float* hsi, int scanmask, unsigned char* mask);
int get_skin_mask_yuv420p(int w, int h, unsigned char*pyuv, int scanmask, unsigned char* mask);
int get_skin_mask_yuv420p_hsip(int w, int h, unsigned char* pyuv, float* hsi, int scanmask, unsigned char* mask);
int sunshine_comp_rgb(int w, int h, int scanrgb, unsigned char* prgb);


#ifdef __cplusplus
}
#endif

#endif