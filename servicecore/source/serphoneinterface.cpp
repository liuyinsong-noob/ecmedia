
#define   _CRTDBG_MAP_ALLOC 
#include "serphoneinterface.h"
#include "servicecore.h"
#include "serphonecall.h"
#include "friends.h"
#include "lpconfig.h"
#include "enum.h"
#include "ECMedia.h"

#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

#ifdef WEBRTC_ANDROID
#include <android/log.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
	#include <unistd.h>
#endif 

/**
 * Instanciates a ServiceCore object.
 * @ingroup initializing
 *
 * The ServiceCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param vtable a SerphoneCoreVTable structure holding your application callbacks
 * @param config_path a path to a config file. If it does not exists it will be created.
 *        The config file is used to store all settings, call logs, friends, proxies... so that all these settings
 *	       become persistent over the life of the ServiceCore object.
 *	       It is allowed to set a NULL config file. In that case ServiceCore will not store any settings.
 * @param factory_config_path a path to a read-only config file that can be used to
 *        to store hard-coded preference such as proxy settings or internal preferences.
 *        The settings in this factory file always override the one in the normal config file.
 *        It is OPTIONAL, use NULL if unneeded.
 * @param userdata an opaque user pointer that can be retrieved at any time (for example in
 *        callbacks) using serphone_core_get_user_data().
 *
**/
//const char * g_log_filename = "./console.log";
//FILE *g_interface_fp =NULL;
//#define MAX_LOG_LINE   3000
//int g_log_line =0;


//PrintConsoleHook gPrintConsoleHook = NULL;
//yuntongxunwebrtc::CriticalSectionWrapper  *g_printConsole_lock_;

//static void init_print_log()
//{
//	if(!g_TraceFlag) {
//		return;
//	}
//	if (NULL == g_interface_fp )
//		g_interface_fp = fopen(g_log_filename,"wt");
//}

//static void backup()
//{
////char bak_name[MAX_PATH];
//    char bak_name[256];
//	if (g_interface_fp)
//		fclose(g_interface_fp);
//	else
//		return;
//	g_interface_fp = NULL;
//	strcpy(bak_name,g_log_filename);
//	strcat(bak_name,"_bak");
//#ifdef WIN32
//	_unlink(bak_name);
//#else
//	unlink(bak_name);
//#endif
//	rename(g_log_filename,bak_name);
//	g_interface_fp = fopen(g_log_filename,"wt");
//	g_log_line = 0;
//}


//extern void PrintConsole(const char * fmt,...);
//{
//    if(!g_TraceFlag) {
//        return;
//    }
//	struct tm *pt = NULL;
//	time_t curr_time;
//    curr_time = time(NULL);
//
//	#ifdef WIN32
//		pt = localtime(&curr_time);
//	#else
//		struct tm t1;
//		pt = localtime_r(&curr_time,&t1);
//	#endif
//	if( !pt)
//		return;
//	char log_buffer[2048] = {0};
//	va_list ap;
//	va_start(ap,fmt);
//#ifndef WIN32
//	int count = sprintf(log_buffer,"%02d%02d %02d:%02d:%02d ",
//			pt->tm_mon+1,pt->tm_mday,
//			pt->tm_hour,pt->tm_min,pt->tm_sec);
//
//	vsnprintf(log_buffer+count, 2047-count, fmt, ap);
//#else
//	int count = sprintf(log_buffer,"%02d%02d %02d:%02d:%02d ",
//			pt->tm_mon+1,pt->tm_mday,
//			pt->tm_hour,pt->tm_min,pt->tm_sec);
//	_vsnprintf(log_buffer+count, 2047-count, fmt, ap);
//#endif
//
//	va_end(ap);
//
//#ifdef WEBRTC_ANDROID
//	__android_log_print(ANDROID_LOG_DEBUG,"console","%s", log_buffer);
//#else
//	printf("%s\n",log_buffer);
//#endif
//
//	if( gPrintConsoleHook) 
//		gPrintConsoleHook(0,log_buffer);
//
//	if( NULL == g_interface_fp || NULL == g_printConsole_lock_ ) {
//		return;
//	}
//	yuntongxunwebrtc::CriticalSectionScoped lock(g_printConsole_lock_);
//	fprintf(g_interface_fp, "%s\n", log_buffer);
//	fflush(g_interface_fp);
//	g_log_line ++;
//	
//}

ServiceCore *serphone_core_new(const SerphoneCoreVTable *vtable,
						const char *config_path, const char *factory_config_path, void* userdata)
{
	//init_print_log();
	//g_printConsole_lock_ = yuntongxunwebrtc::CriticalSectionWrapper::CreateCriticalSection();

//	ServiceCore *core=ms_new(ServiceCore,1); commented by zdm,类不能用这个函数，否则无法进入类构造函数
	ServiceCore *core=new ServiceCore;
    PrintConsole("serphone_core_new\n");
	core->serphone_core_init(vtable,config_path, factory_config_path, userdata);
	return core;
}

/**
 * Destroys a LinphoneCore
 *
 * @ingroup initializing
**/
void serphone_core_destroy(ServiceCore *lc)
{
	lc->serphone_core_uninit();
	delete lc;

	//if(g_interface_fp)
	//	fclose(g_interface_fp);
	//g_interface_fp = NULL;
	//
	//if( g_printConsole_lock_)
	//	delete g_printConsole_lock_;
	//g_printConsole_lock_ = NULL;

#ifdef WIN32
   // _CrtDumpMemoryLeaks(); 
#endif
}



