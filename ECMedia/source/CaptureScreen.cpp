// Copyright 2001 - 2003 RenderSoft Software & Web Publishing
//
// camstudio_cl: Screen recording from the command line.
//
// Based on the open source version of CamStudio:
//    http://camstudio.org/
// 
// Source code retrieved from:
//    http://camstudio.org/dev/CamStudio.2.5.b1.src.zip
// 
// License: GPL
// 
// Command line author: dimator(AT)google(DOT)com
// Multi screen support: karol(dot)toth(at)gmail(dot)com


//#include <algorithm>
//#include <string>
//#include <cstring>
//#include <vector>
//#include <iostream>
//#include <iterator>
//#include <objbase.h>

#include <windows.h>
//#include <windowsx.h>
//#include <windef.h>
//#include <vfw.h>
//#include <atltypes.h>
//#include <atlstr.h>
//#include <time.h>

#include "CaptureScreen.h"

using namespace std;

#ifndef CAPTUREBLT
  #define CAPTUREBLT (DWORD)0x40000000
#endif

screen::screen(){
	index = 0;
	left= 0;
	right = 0;
	top = 0;
	bottom = 0;
	width = 0;
	height = 0;
};

bool screen::SetDimensions(int left, int right, int top, int bottom){
	if(right > left && bottom > top)
	{
		this->left = left;
		this->right = right;
		this->top = top;
		this->bottom = bottom;
		this->width = (right - left);
		this->height = (bottom - top);
		return true;
	}
	else
	{
		return false;
	}
}

//Screen Detection Functions
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor,HDC hdcMonitor,LPRECT lprcMonitor,LPARAM dwData)
{
	CaptureScreen *captureScreen = (CaptureScreen*)dwData;
	screen* obr = (screen*)captureScreen->pscreen;
	captureScreen->mon_current = captureScreen->mon_current + 1;
	
	MONITORINFOEX mo;
	mo.cbSize = sizeof(MONITORINFOEX);
	if( !GetMonitorInfo(hMonitor,&mo) ) 
	{
		return false;
	}
	obr[captureScreen->mon_current].SetDimensions(mo.rcMonitor.left, mo.rcMonitor.right, mo.rcMonitor.top, mo.rcMonitor.bottom);
	obr[captureScreen->mon_current].hMonitor=hMonitor;
	obr[captureScreen->mon_current].hdcMonitor=hdcMonitor;
	return true;
}

CaptureScreen::CaptureScreen()
{
	count_mon=0;
	pscreen=NULL;
	_alpbi = NULL;
	hSavedCursor = NULL;
	mon_current = -1;
	bits = 24;
	recordcursor = 1;
	captureTrans=1;
	versionOp = 5;
	timestampAnnotation = 0;

	count_mon = GetSystemMetrics(SM_CMONITORS); // get nubmer of monitors
	pscreen = new screen[3]();

	//Detection of screens
	//cout << "Detected displays:" << endl;
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)this);

	if(2==count_mon)
	{
		pscreen[2].left = pscreen[0].left;
		pscreen[2].top = pscreen[0].top;
		pscreen[2].right = pscreen[1].right;
		if(pscreen[1].bottom<pscreen[0].bottom)
		{
			pscreen[2].bottom = pscreen[0].bottom;
		}else
		{
			pscreen[2].bottom = pscreen[1].bottom;
		}
		pscreen[2].width= pscreen[2].right-pscreen[2].left-1;
		pscreen[2].height = pscreen[2].bottom-pscreen[2].top-1;
		//cout << "Creating recording thread for screen no.width:" <<pscreen[2].width <<" right:"<<pscreen[2].height << endl;
	}
}
CaptureScreen::~CaptureScreen()
{
	if(NULL!=pscreen)
	{
		delete[] pscreen;
		pscreen=NULL;
	}

	if(NULL!=_alpbi)
	{
		FreeFrame(_alpbi);
		_alpbi=NULL;
	}
	mon_current = -1;
}

int CaptureScreen::getScreenInfo(screen **screeninfo)
{
	*screeninfo = pscreen;
	return count_mon;
}

HANDLE  CaptureScreen::Bitmap2Dib( HBITMAP hbitmap, UINT bits )
{
  HANDLE               hdib ;
  HDC                 hdc ;
  BITMAP              bitmap ;
  UINT                wLineLen ;
  DWORD               dwSize ;
  DWORD               wColSize ;
  LPBITMAPINFOHEADER  lpbi ;
  LPBYTE              lpBits ;

  GetObject(hbitmap,sizeof(BITMAP),&bitmap) ;

  // DWORD align the width of the DIB
  // Figure out the size of the colour table
  // Calculate the size of the DIB
  //
  wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
  wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
  dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
    (DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

  //
  // Allocate room for a DIB and set the LPBI fields
  //
  hdib = GlobalAlloc(GHND,dwSize);
  if (!hdib)
    return hdib ;

  lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;

  lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
  lpbi->biWidth = bitmap.bmWidth ;
  lpbi->biHeight = bitmap.bmHeight ;
  lpbi->biPlanes = 1 ;
  lpbi->biBitCount = (WORD) bits ;
  lpbi->biCompression = BI_RGB ;
  lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
  lpbi->biXPelsPerMeter = 0 ;
  lpbi->biYPelsPerMeter = 0 ;
  lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
  lpbi->biClrImportant = 0 ;

  //
  // Get the bits from the bitmap and stuff them after the LPBI
  //
  lpBits = (LPBYTE)(lpbi+1)+wColSize ;

  hdc = CreateCompatibleDC(NULL) ;

  GetDIBits(hdc,hbitmap,0,bitmap.bmHeight,lpBits,(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

  lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

  DeleteDC(hdc) ;
  GlobalUnlock(hdib);

  return hdib ;
}

LPBITMAPINFOHEADER CaptureScreen::captureScreenFrame(HWND hWnd,int left,int top,int width, int height)
{
	//cout << "captureScreenFrame"  << endl;
	//获得屏幕的HDC.//hWnd：设备上下文环境被检索的窗口的句柄，如果该值为NULL，GetDC则检索整个屏幕的设备上下文环境。
   HDC hScreenDC= ::GetDC(hWnd);
 

  //该函数创建一个与指定设备兼容的内存设备上下文环境（DC）。通过GetDc()获取的HDC直接与相关设备沟通，而本函数创建的DC，则是与内存中的一个表面相关联。
  HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
  HBITMAP hbm;

  //该函数创建与指定的设备环境相关的设备兼容的位图
  hbm = CreateCompatibleBitmap(hScreenDC, width, height);
  HBITMAP oldbm = (HBITMAP) SelectObject(hMemDC, hbm);

  //BitBlt(hMemDC, 0, 0, width, height, hScreenDC, left, top, SRCCOPY);

  //ver 1.6
  DWORD bltFlags = SRCCOPY;
  if ((captureTrans) && (versionOp>4))
    bltFlags |= CAPTUREBLT;
   // bltFlags |= NOMIRRORBITMAP;
  //该函数对指定的源设备环境区域中的像素进行位块（bit_block）转换，以传送到目标设备环境。
  BitBlt(hMemDC, 0, 0, width, height, hScreenDC, left, top, bltFlags);

  if(timestampAnnotation){
    SYSTEMTIME systime;
    ::GetLocalTime(&systime);
    //::GetSystemTime(&systime);
    //TCHAR msg[0x100];
    //::sprintf_s(msg, "%s %02d:%02d:%02d:%03d", "Recording", systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
  }

  //Get Cursor Pos
  POINT xPoint;
  GetCursorPos( &xPoint );
  HCURSOR hcur = FetchCursorHandle();
  xPoint.x -= left;
  xPoint.y -= top;

  //Draw the Cursor
  if (recordcursor == 1) {
    ICONINFO iconinfo ;
    BOOL ret;
    ret = GetIconInfo( hcur,  &iconinfo );
    if (ret) {
      xPoint.x -= iconinfo.xHotspot;
      xPoint.y -= iconinfo.yHotspot;

      //need to delete the hbmMask and hbmColor bitmaps
      //otherwise the program will crash after a while after running out of resource
      if (iconinfo.hbmMask)
        DeleteObject(iconinfo.hbmMask);
      if (iconinfo.hbmColor)
        DeleteObject(iconinfo.hbmColor);
    }
	
	::DrawIconEx( hMemDC, xPoint.x, xPoint.y, hcur, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
  }

  SelectObject(hMemDC,oldbm);
  LPBITMAPINFOHEADER pBM_HEADER = (LPBITMAPINFOHEADER)GlobalLock(Bitmap2Dib(hbm, bits));

  DeleteObject(oldbm);
  DeleteObject(hbm);
  DeleteDC(hMemDC);


  ReleaseDC(hWnd,hScreenDC) ;

  return pBM_HEADER;
}

void CaptureScreen::FreeFrame(LPBITMAPINFOHEADER alpbi)
{
  if (!alpbi)
    return ;
  GlobalFree(alpbi);
  //GlobalFreePtr(alpbi);
  alpbi = NULL;
}

HCURSOR CaptureScreen::FetchCursorHandle() {

	CURSORINFO ci = {0};
    ci.cbSize = sizeof(CURSORINFO);
    if(GetCursorInfo(&ci))
	{
		hSavedCursor =ci.hCursor;
	}else
	{
		hSavedCursor = GetCursor();
	}
  return hSavedCursor;
}

////工具
//int CaptureScreen::RGB24Snapshot(BYTE* pData, int size,int width, int height, const char* filename)
//{
//      // int size = width*height*3; // 每个像素点3个字节
//
//       // 位图第一部分，文件信息
//       BITMAPFILEHEADER bfh;
//       bfh.bfType = 0x4d42; //bm
//       bfh.bfSize = size // data size
//              + sizeof( BITMAPFILEHEADER ) // first section size
//              + sizeof( BITMAPINFOHEADER ) // second section size
//              ;
//       bfh.bfReserved1 = 0; // reserved
//       bfh.bfReserved2 = 0; // reserved
//       bfh.bfOffBits = bfh.bfSize - size;
//
//       // 位图第二部分，数据信息
//       BITMAPINFOHEADER bih;
//       bih.biSize = sizeof(BITMAPINFOHEADER);
//       bih.biWidth = width;
//       bih.biHeight = height;
//       bih.biPlanes = 1;
//       bih.biBitCount = 24;
//       bih.biCompression = 0;
//       bih.biSizeImage = size;
//       bih.biXPelsPerMeter = 0;
//       bih.biYPelsPerMeter = 0;
//       bih.biClrUsed = 0;
//       bih.biClrImportant = 0;  
//
//       FILE * fp = fopen(filename,"wb");
//       if(!fp) 
//	   {
//			printf("RGB24Snapshot fopen return NULL,filename=%s",filename);
//			return 1;
//	   }
//       fwrite( &bfh, 1, sizeof(BITMAPFILEHEADER), fp );
//       fwrite( &bih, 1, sizeof(BITMAPINFOHEADER), fp );
//	   int linesize = width*3;
//    /*   for(int line=height-1; line>=0; line--)
//       {
//        fwrite(pData+width*line*3, linesize, 1, fp);
//        }*/
//       fwrite( pData, 1, size, fp );
//       fclose( fp );
//	   return 0;
//}



int CaptureScreen::getScreenFrame(int id, unsigned char **buf, int *size, int *pwidth, int *pheight)
{
	  int ret=0;

	  if((id>2||id<0))
	  {
		  return ERR_CAMSTUDIO_NULL;
	  }
 
	  if(id>count_mon)
	  {
		  return ERR_CAMSTUDIO_MONITORCOUNT_LESS;
	  }


	  int top=pscreen[id].top;;
	  int left=pscreen[id].left;
	  int width=pscreen[id].width;
	  int height=pscreen[id].height;

	  if(NULL!=_alpbi)
	  {
		FreeFrame(_alpbi);
		_alpbi=NULL;
	  }
	  
	  _alpbi = captureScreenFrame(NULL,left,top,width, height);
	  if(NULL==_alpbi)
	  {
		  return ERR_CAMSTUDIO_SCREEN_FRAME_NULL;
	  }
	  else
	  {
		  *pwidth=_alpbi->biWidth ;
          *pheight=_alpbi->biHeight;
          *size=_alpbi->biSizeImage ;
		     // pointer to data
		  *buf=(LPBYTE) _alpbi + _alpbi->biSize + _alpbi->biClrUsed * sizeof(RGBQUAD);
	  }

	 // RGB24Snapshot((BYTE*) buf, _alpbi->biSizeImage, _alpbi->biWidth, _alpbi->biHeight, "d:\\screen.bmp");

	return ret;
}

int CaptureScreen::getScreenFrameEx(unsigned char **buf, int *size, int *pwidth, int *pheight, int left, int top, int width, int height)
{
	int ret=0;

	if(NULL!=_alpbi)
	{
		FreeFrame(_alpbi);
		_alpbi=NULL;
	}

	_alpbi = captureScreenFrame(NULL,left,top,width, height);
	if(NULL==_alpbi)
	{
		return ERR_CAMSTUDIO_SCREEN_FRAME_NULL;
	}
	else
	{
		*pwidth=_alpbi->biWidth ;
		*pheight=_alpbi->biHeight;
		*size=_alpbi->biSizeImage ;
		// pointer to data
		*buf=(LPBYTE) _alpbi + _alpbi->biSize + _alpbi->biClrUsed * sizeof(RGBQUAD);
	}

	//RGB24Snapshot((BYTE*) buf, _alpbi->biSizeImage, _alpbi->biWidth, _alpbi->biHeight, "d:\\screen.bmp");

	return ret;
}