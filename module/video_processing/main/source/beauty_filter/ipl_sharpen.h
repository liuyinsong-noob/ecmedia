/* @file: ipl_sharpen.h
*  @author: Guoqing Zeng
*  @date: 20160830
*  @description: support image sharpen algorithm        
*  @version:
*
*/
#ifndef __ZIPL_SHARPEN_H__
#define __ZIPL_SHARPEN_H__

#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////
//----------------sample code-------------------
//  int w = 480;
//  int h = 640;
//  int size = zipl_sharpen_getoutbufsize(w, h);
//  unsigned char* buf = malloc(size);
//  zipl_sharpen_yuv420p(...,buf) / zipl_sharpen_yuv420p_r(...,buf) / zipl_sharpen_yuv420(...,buf) / zipl_sharpen_rgb(...,buf)
//  free(buf);
//

int zipl_sharpen_get_outbufsize(int w, int h);

//test passed
int zipl_sharpen_yuv420p( int w, int h,
	                      unsigned char* psrc, 
						  unsigned char* pdst,
						  unsigned char* poutbuf
						 );

//test passed
int zipl_sharpen_yuv420p_r( int w, int h, 
	                        unsigned char* psrcdst,
						    unsigned char* poutbuf 
						   );

//test passed
int zipl_sharpen_yuv420( int w, int h, 
		                 int scany_src, unsigned char* py_src, 
						 int scanu_src, unsigned char* pu_src, 
						 int scanv_src, unsigned char* pv_src,
						 int scany_dst, unsigned char* py_dst, 
						 int scanu_dst, unsigned char* pu_dst, 
						 int scanv_dst, unsigned char* pv_dst,
						 unsigned char* poutbuf
					   );

//test passed
int zipl_sharpen_yuv420_r( int w, int h, 
	                       int scany, unsigned char* py_srcdst, 
	                       int scanu, unsigned char* pu_srcdst, 
	                       int scanv, unsigned char* pv_srcdst,
	                       unsigned char* poutbuf
	                      );

//test passed
int zipl_sharpen_rgb( int w, int h, 
		              int scansrc, unsigned char* psrc, 
					  int scandst, unsigned char* pdst,
					  unsigned char* poutbuf
					 );

//test passed
int zipl_sharpen_rgb_r( int w, int h, 
	                    int scan, unsigned char* psrcdst, 
	                    unsigned char* poutbuf
	                   );


#ifdef __cplusplus
}
#endif

#endif