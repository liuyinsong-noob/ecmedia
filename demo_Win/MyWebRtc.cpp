// MyWebRtc.cpp : 定义控制台应用程序的入口点。
//
#define _CRTDBG_MAP_ALLOC

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>

#include "serphoneinterface.h"
#include "serphonecall.h"
#include "voe_audio_processing.h"
#include "voe_base.h"
#include "voe_file.h"
#include "voe_codec.h"
#include "voe_hardware.h"
#include "servicecore.h"
#include "sometools.h"

#include <stdlib.h>
#include <crtdbg.h>

#ifndef PATH_MAX
#define  PATH_MAX  256
#endif

static char configfile_name[PATH_MAX];

namespace yuntongxunwebrtc {

void RunHarness() {

  VoiceEngine* voe = VoiceEngine::Create();
  if (voe == NULL)
  {
	  printf("create engine fail\n");
	  _exit(-1);
  }
  VoEAudioProcessing* audio = VoEAudioProcessing::GetInterface(voe);
  
  VoEBase* base = VoEBase::GetInterface(voe);
  VoECodec* codec = VoECodec::GetInterface(voe);

  VoEHardware* hardware = VoEHardware::GetInterface(voe);
  VoEFile* file  = VoEFile::GetInterface(voe);

  base->Init();
  int channel = base->CreateChannel();
/*
//  file->StartPlayingFileAsMicrophone(-1, "audio_short16.pcm", true,true);
  ret = file->StartPlayingFileLocally(channel, "audio_short16.pcm", true);
  printf("StartPlayingFileLocally ret=%d,error=%d\n",ret,base->LastError());
  base->StartPlayout(channel);

    int num_devices = 0;
  hardware->GetNumOfPlayoutDevices(num_devices);
  char device_name[128] = {0};
  char guid[128] = {0};
  bool device_found = false;
  int device_index=0;
  hardware->GetPlayoutDeviceName(device_index, device_name, guid);
  hardware->SetPlayoutDevice(device_index);


//  base->StopPlayout(0);
  file->StartPlayingFileLocally(-1, "audio_long16.pcm", true);
  base->StartPlayout(-1);
  printf("playfile is processing id(%d)\n",channel);
*/

  base->SetSendDestination(channel, 1234, "127.0.0.1");
  base->SetLocalReceiver(channel, 1234);

  CodecInst codec_params = {0};
  bool codec_found = false;
  for (int i = 0; i < codec->NumOfCodecs(); i++) {
    codec->GetCodec(i, codec_params);
  }
  if (codec_found)
     codec->SetSendCodec(channel, codec_params);

  int num_devices = 0;
  hardware->GetNumOfPlayoutDevices(num_devices);
  char device_name[128] = {0};
  char guid[128] = {0};
  bool device_found = false;
  int device_index=0;
//  for (device_index = 0; device_index < num_devices; device_index++) {
    hardware->GetPlayoutDeviceName(device_index, device_name, guid);
//    if (FLAGS_render.compare(device_name) == 0) {
      device_found = true;
//      break;
 //   }
//  }
  hardware->SetPlayoutDevice(device_index);

  // Disable all audio processing.
  audio->SetAgcStatus(false);
  audio->SetEcStatus(true);
  audio->EnableHighPassFilter(true);
  audio->SetNsStatus(false);
  
  base->StartReceive(channel);
  base->StartPlayout(channel);
  base->StartSend(channel);
  
  // Run forever...
  int i=0;
//  while (1)
  while(i++<10)
  {
    Sleep(1000);
  }
  base->StopReceive(channel);
  base->StopSend(channel);
  base->StopPlayout(channel);
  base->DeleteChannel(channel);
  audio->Release();
  codec->Release();
  hardware->Release();
  file->Release();
    Sleep(1000);
  base->Terminate();
  base->Release();
    Sleep(1000);
  VoiceEngine::Delete(voe);
}

}  // namespace webrtc

void mainstatechange(class ServiceCore *lc, SerphoneGlobalState gstate, const char *message)
{
	PrintConsole("global state change %s,state=%s\n",message,serphone_global_state_to_string(gstate));
	return;
}

void maincallstatechange(class ServiceCore *lc, SerPhoneCall *call, SerphoneCallState cstate, const char *message)
{
	PrintConsole("call state change %s,state=%s\n",message,serphone_call_state_to_string(cstate));
	switch( cstate )
	{
	case LinphoneCallIdle:
		break;
	case LinphoneCallIncomingReceived:
	    lc->serphone_core_accept_call(call);  //自动应答呼叫
		break;
    case LinphoneCallOutgoingInit:
		break;
	case LinphoneCallOutgoingProgress:
		break;
	case LinphoneCallOutgoingRinging:
		break;
	case LinphoneCallOutgoingEarlyMedia:
		break;
	case LinphoneCallConnected:
		break;
	case LinphoneCallStreamsRunning:
		break;
	}
	return;
}

int cmd_register(class ServiceCore *lc)
{
	char identity[512];
	char proxy[512];
	char passwd[512];
	SerphoneProxyConfig *cfg;
	MSList *elem = NULL;

//	char * args="<sip:1001@114.251.190.144> sip:114.251.190.144:30100 1001";
	char * args="<sip:1002@192.168.105.127> sip:192.168.105.127:5060 1234";
//	char * args="<sip:1002@192.168.1.102> sip:192.168.1.102:5060 1234";
	passwd[0]=proxy[0]=identity[0]='\0';
//	printf("parameter %511s %511s %511s\n",identity,proxy,passwd);
	sscanf(args,"%511s %511s %511s",identity,proxy,passwd);
	if (proxy[0]=='\0' || identity[0]=='\0'){
		printf("Missing parameters, see help register\n");
		return 1;
	}
	if (passwd[0]!='\0'){
		SerphoneAddress *from;
		SerphoneAuthInfo *info;
		if ((from=serphone_address_new(identity))!=NULL){
			char realm[128];
			snprintf(realm,sizeof(realm)-1,"\"%s\"",serphone_address_get_domain(from));
			info=serphone_auth_info_new(serphone_address_get_username(from),NULL,passwd,NULL,NULL);
			lc->serphone_core_add_auth_info(info);
			serphone_address_destroy(from);
			serphone_auth_info_destroy(info);
		}
	}
	elem=lc->serphone_core_get_proxy_config_list();
	if (elem) {
		cfg=(SerphoneProxyConfig*)elem->data;
		cfg->serphone_proxy_config_edit();
	}
	else cfg=serphone_proxy_config_new();
	cfg->serphone_proxy_config_set_identity(identity);
	cfg->serphone_proxy_config_set_server_addr(proxy);
	cfg->serphone_proxy_config_enable_register(TRUE);
	if (elem) cfg->serphone_proxy_config_done();
	else lc->serphone_core_add_proxy_config(cfg);
	lc->serphone_core_set_default_proxy(cfg);
	return 1;
}

int main(int argc, char** argv) {

//  webrtc::RunHarness();

#ifndef _WIN32
	snprintf(configfile_name, PATH_MAX, "%s/.serphonerc",
			getenv("HOME"));
#elif defined(_WIN32_WCE)
	strncpy(configfile_name,".\\serphonerc",PATH_MAX);
	mylogfile=fopen(PACKAGE_DIR "\\" "serphonec.log","w");
#else
	strncpy(configfile_name,  "serphonerc",PATH_MAX);
#endif
	SerphoneCoreVTable l_vtable;
	memset(&l_vtable,0,sizeof(SerphoneCoreVTable));
	l_vtable.global_state_changed = mainstatechange;
	l_vtable.call_state_changed = maincallstatechange;
	ServiceCore * core = serphone_core_new(&l_vtable,configfile_name,NULL,NULL);
	cmd_register(core);
	int i=0;
	while(i<5){
	    core->serphone_core_iterate();

	    ::Sleep(1000); 
		i++;
	}
	char *p_to ="1006";
	char *p_msg="hello,who are you";
	SerphoneChatRoom *cr = core->serphone_core_create_chat_room(p_to);
	serphone_chat_room_send_message(cr,p_msg);
	serphone_chat_room_destroy(cr);
	i=0;
	while(i<5){
	    core->serphone_core_iterate();

	    ::Sleep(1000); 
		i++;
	}
	serphone_core_destroy(core);
  return 0;
}

