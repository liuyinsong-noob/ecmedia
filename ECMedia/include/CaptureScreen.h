#ifndef _CAMSTUDIO_H_
#define _CAMSTUDIO__H_

//错误码定义
enum {
	ERR_CAMSTUDIO_NULL =-200,//空
	ERR_CAMSTUDIO_MONITORCOUNT_LESS,//系统屏幕少于目标屏幕数
	ERR_CAMSTUDIO_SCREEN_FRAME_NULL//空帧
};

class screen{
public:
	int index;
	int left;
	int right;
	int top;
	int bottom;
	int width;
	int height;
	char outFile[50];
	char dispName[50];
	int fps;
	HMONITOR hMonitor;
	HDC hdcMonitor;
public:
	screen();
	bool SetDimensions(int left, int right, int top, int bottom);
};

class CaptureScreen {

public:
	CaptureScreen();
	~CaptureScreen();

	int getScreenInfo(screen **screeninfo);

	//Mouse Capture functions
	HCURSOR FetchCursorHandle();

	HANDLE Bitmap2Dib(HBITMAP, UINT);

	//Use these 2 functions to create frames and free frames
	LPBITMAPINFOHEADER captureScreenFrame(HWND hWnd,int left,int top,int width, int height);
	void FreeFrame(LPBITMAPINFOHEADER) ;
	int RGB24Snapshot( BYTE * pData, int size, int width, int height, const char * filename );//主流USB视频摄像头的媒体格式为RGB24，如何把这些数据变成BMP位图

	/*! @function
	********************************************************************************
	函数名   : getLocalVideoSnapshot
	功能     : 视频通话中，抓取本地视频截图
	参数     :  [IN] callid :  0主,1副,2双
			[OUT] buf: 截图内容
			[OUT] size: 内容长度
			[OUT] width: 截图宽度
			[OUT] height: 截图高度
	返回值   :  成功 0 失败 -1
	*********************************************************************************/
	int getScreenFrame(int id, unsigned char **buf, int *size, int *width, int *height);
	int getScreenFrameEx(unsigned char **buf, int *size, int *pwidth, int *pheight, int left, int top, int width, int height);

private:
	////////////////////////////////////
	//供外部调用时，全局变量
	int count_mon;
	LPBITMAPINFOHEADER _alpbi;
	HCURSOR hSavedCursor;
	int bits;
	int recordcursor;
	int captureTrans;
	int versionOp;
	int timestampAnnotation;
public:
	screen* pscreen;
	int mon_current;

};

#endif 
