
#define   _CRTDBG_MAP_ALLOC 
#include "serphonecall.h"
#include "servicecore.h"
#include "sal_eXosip2.h"

#if !defined(NO_VOIP_FUNCTION)
//#include "voe_observer.h"
//#include "RecordVoip.h"

#ifdef VIDEO_ENABLED
#include "videoframe.h"
#endif
#endif

#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

/*prevent a gcc bug with %c*/
static size_t my_strftime(char *s, size_t max, const char  *fmt,  const struct tm *tm){
#if !defined(_WIN32_WCE)
	return strftime(s, max, fmt, tm);
#else
	return 0;
	/*FIXME*/
#endif /*_WIN32_WCE*/
}

static void set_call_log_date(SerphoneCallLog *cl, const struct tm *loctime){
	my_strftime(cl->start_date,sizeof(cl->start_date),"%c",loctime);
}

/* this function is called internally to get rid of a call.
 It performs the following tasks:
 - remove the call from the internal list of calls
 - update the call logs accordingly
*/

static void serphone_call_set_terminated(SerPhoneCall *call)
{
	ServiceCore *lc=call->core;

//	linphone_core_update_allocated_audio_bandwidth(lc);  commented by zdm

	call->owns_call_log=FALSE;
	serphone_call_log_completed(call);
    
#if !defined(NO_VOIP_FUNCTION)
	//TODO:
	//if(call->record_voip && call->record_voip->isStartRecordWav()) {
	//	lc->serphone_call_stop_record_audio(call);
	//}
 //   
 //   if(call->record_voip && call->record_voip->isStartRecordMp4()) {
 //       lc->serphone_call_stop_record_voip(call);
 //   }

	//if(call->record_voip && call->record_voip->isStartRecordScree()) {
	//	lc->serphone_call_stop_record_screen(call);
	//}
#endif

	if (call == lc->current_call){
		PrintConsole("Resetting the current call\n");
		lc->current_call=NULL;
	}

	if (lc->serphone_core_del_call(call) != 0){
		PrintConsole("Could not remove the call from the list !!!\n");
	}

	if (ms_list_size(lc->calls)==0)
		lc->serphone_core_notify_all_friends(lc->presence_mode);

//	linphone_core_conference_check_uninit(lc);  commented by zdm
	if (call->ringing_beep){
		lc->serphone_core_stop_dtmf();
		call->ringing_beep=FALSE;
	}
	if (call->referer){
		serphone_call_unref(call->referer);
		call->referer=NULL;
	}
	//hubin resolve memory leak
	lc->serphone_call_delete_ice_session(call);
}

const SerphoneAddress * serphone_call_get_remote_address(const SerPhoneCall *call)
{
	return call->dir==LinphoneCallIncoming ? call->log->from : call->log->to;
}

SerphoneCallLog * serphone_call_log_new(SerPhoneCall *call,SerphoneAddress *from, SerphoneAddress * to)
{
//	SerphoneCallLog *cl=ms_new0(SerphoneCallLog,1);  //ms_new0
	SerphoneCallLog *cl=(SerphoneCallLog *)malloc(sizeof(SerphoneCallLog)*1);  //ms_new0
	memset((void *)cl,0,sizeof(SerphoneCallLog)*1);


	struct tm loctime;
	cl->dir=call->dir;
#ifdef WIN32
#if !defined(_WIN32_WCE)
	loctime=*localtime(&call->start_time);
	/*FIXME*/
#endif /*_WIN32_WCE*/
#else
	localtime_r(&call->start_time,&loctime);
#endif
	set_call_log_date(cl,&loctime);
	cl->from=from;
	cl->to=to;
    cl->status=LinphoneCallAborted; /*default status*/
	return cl;
}

void serphone_call_log_destroy(SerphoneCallLog **cl)
{
	if ((*cl)->from!=NULL) serphone_address_destroy((*cl)->from);
	if ((*cl)->to!=NULL) serphone_address_destroy((*cl)->to);
	if ((*cl)->refkey!=NULL) {ms_free((void **)&(*cl)->refkey);(*cl)->refkey = NULL;}
	ms_free((void **)cl);
}

void serphone_call_set_transfer_state(SerPhoneCall* call,SerphoneCallState state)
{
	if (state != call->transfer_state) {
		ServiceCore* lc = call->core;
		call->transfer_state = state;
		if (lc->vtable.transfer_state_changed)
			lc->vtable.transfer_state_changed(lc, call, state);
	}
}

SerphoneCallState serphone_call_get_transfer_state(SerPhoneCall *call)
{
	return call->transfer_state;
}

void serphone_call_init_common(SerPhoneCall *call,SerphoneAddress *from, SerphoneAddress *to)
{
	int port_offset;
	call->magic=serphone_call_magic;
	call->refcnt=1;
	call->state=LinphoneCallIdle;
	call->transfer_state = LinphoneCallIdle;
	call->start_time=time(NULL);
	call->media_start_time=0;
	call->log=serphone_call_log_new(call, from, to);
	call->owns_call_log=TRUE;
	call->core->serphone_core_notify_all_friends(LinphoneStatusOnThePhone);
	port_offset=call->core->find_port_offset ();
	if (port_offset==-1) return;
	call->audio_port=call->core->serphone_core_get_audio_port()+port_offset;
	call->video_port=call->core->serphone_core_get_video_port()+port_offset;
}

void serphone_call_set_state(SerPhoneCall *call,SerphoneCallState cstate, const char *message)
{
    
	ServiceCore *lc=call->core;

	if (call->state!=cstate){
		if (call->state==LinphoneCallEnd || call->state==LinphoneCallError){
			if (cstate!=LinphoneCallReleased){
				PrintConsole("Spurious call state change from %s to %s, ignored.\n",serphone_call_state_to_string(call->state),
				   serphone_call_state_to_string(cstate));
				return;
			}
		}
		PrintConsole("[Call] [%04d]: %s ----> %s\n",call->op->cid <0 ?0 : call->op->cid,
				serphone_call_state_to_string(call->state),
		           serphone_call_state_to_string(cstate));
		if (cstate!=LinphoneCallRefered && cstate!=LinphoneCallUpdatedRemoteVideoratio){
			/*LinphoneCallRefered is rather an event, not a state.
			 Indeed it does not change the state of the call (still paused or running)
             LinphoneCallUpdatedRemoteVideoratio does not change the state of the call too*/
			call->state=cstate;
		}
		if (cstate==LinphoneCallEnd || cstate==LinphoneCallError){
             if (call->reason==SerphoneReasonDeclined){
				call->log->status=LinphoneCallDeclined;
			}
			serphone_call_set_terminated (call);
		}
		if (cstate == LinphoneCallConnected) {
			call->log->status=LinphoneCallSuccess;
			call->media_start_time=time(NULL);
		}

		if (lc->vtable.call_state_changed)
			lc->vtable.call_state_changed(lc,call,cstate,message);
		if (cstate==LinphoneCallReleased){
			if (call->op!=NULL) {
				/* so that we cannot have anymore upcalls for SAL
				 concerning this call*/
				sal_op_release(call->op);
				call->op=NULL;
			}
			serphone_call_unref(call);
		}
	}
}


/**
 * Increments the call 's reference count.
 * An application that wishes to retain a pointer to call object
 * must use this function to unsure the pointer remains
 * valid. Once the application no more needs this pointer,
 * it must call linphone_call_unref().
**/
SerPhoneCall * serphone_call_ref(SerPhoneCall *obj)
{
	obj->refcnt++;
	return obj;
}

/**
 * Decrements the call object reference count.
 * See linphone_call_ref().
**/
void serphone_call_unref(SerPhoneCall *obj){
	obj->refcnt--;
	if (obj->refcnt==0){
		serphone_call_destroy(obj);
	}
}

void serphone_call_destroy(SerPhoneCall *obj)
{
	if (obj->op!=NULL) {
		sal_op_release(obj->op);
		obj->op=NULL;
	}
	if (obj->resultdesc!=NULL) {
		sal_media_description_unref(&obj->resultdesc);
	}
	if (obj->localdesc!=NULL) {
		sal_media_description_unref(&obj->localdesc);
	}
	if (obj->ping_op) {
		sal_op_release(obj->ping_op);
	}
	if (obj->refer_to){
		ms_free((void **)&obj->refer_to);
	}
	if (obj->owns_call_log)
		serphone_call_log_destroy(&obj->log);
	if (obj->auth_token) {
		ms_free((void **)&obj->auth_token);
	}

#if !defined(NO_VOIP_FUNCTION)

	if (obj->voe_observer) {
		delete obj->voe_observer;
        obj->voe_observer = NULL;
	}

    if (obj->record_voip) {
        delete obj->record_voip;
        obj->record_voip = NULL;
    }

#ifdef VIDEO_ENABLED
	if (obj->deliver_frame) {
		delete obj->deliver_frame;
        obj->deliver_frame = NULL;
	}

	if (obj->vie_observer)
	{
		delete obj->vie_observer;
		obj->vie_observer = NULL;
	}
	
#endif
#endif

	ms_free((void **)&obj);
}

const char *serphone_call_state_to_string(SerphoneCallState cs)
{
	switch (cs){
		case LinphoneCallIdle:
			return "CallIdle";
		case LinphoneCallIncomingReceived:
			return "CallIncomingReceived";
		case LinphoneCallOutgoingInit:
			return "CallOutgoingInit";
		case LinphoneCallOutgoingProgress:
			return "CallOutgoingProgress";
		case LinphoneCallOutgoingProceeding:
			return "CallOutgoingProceeding";
		case LinphoneCallOutgoingRinging:
			return "CallOutgoingRinging";
		case LinphoneCallOutgoingEarlyMedia:
			return "CallOutgoingEarlyMedia";
		case LinphoneCallConnected:
			return "CallConnected";
		case LinphoneCallStreamsRunning:
			return "CallStreamsRunning";
		case LinphoneCallPausing:
			return "CallPausing";
		case LinphoneCallPaused:
			return "CallPaused";
		case LinphoneCallResuming:
			return "CallResuming";
		case LinphoneCallRefered:
			return "CallRefered";
		case LinphoneCallError:
			return "CallError";
		case LinphoneCallEnd:
			return "CallEnd";
		case LinphoneCallPausedByRemote:
			return "CallPausedByRemote";
		case LinphoneCallUpdatedByRemote:
			return "CallUpdatedByRemote";
		case LinphoneCallIncomingEarlyMedia:
			return "CallIncomingEarlyMedia";
		case LinphoneCallUpdated:
			return "CallUpdated";
        case LinphoneCallUpdatedRemoteVideoratio:
            return "CallUpdatedRemoteVideoRatio";
        case LinphoneCallUpdatedAudioDestinationChanged:
            return "CallUpdatedAudioDestinationChanged";
        case LinphoneCallUpdatedVideoDestinationChanged:
            return "CallUpdatedVideoDestinationChanged";
		case LinphoneCallReleased:
			return "CallReleased";
	}
	return "undefined state";
}

/**
 * Returns the remote address associated to this call as a string.
 *
 * The result string must be freed by user using ms_free().
**/
char *serphone_call_get_remote_address_as_string(const SerPhoneCall *call)
{
	return serphone_address_as_string(serphone_call_get_remote_address(call));
}


/**
 * Returns direction of the call (incoming or outgoing).
**/
SerphoneCallDir serphone_call_get_dir(const SerPhoneCall *call)
{
	return call->log->dir;
}


/**
 * Returns the call log associated to this call.
**/
SerphoneCallLog *serphone_call_get_call_log(const SerPhoneCall *call)
{
	return call->log;
}

/**
 * Returns the refer-to uri (if the call was transfered).
**/
const char *serphone_call_get_refer_to(const SerPhoneCall *call){
	return call->refer_to;
}

ServiceCore *serphone_call_get_core(const SerPhoneCall *call)
{
	return call->core;
}

/**
 * Returns TRUE if the LinphoneCall asked to autoanswer
 *
**/
bool_t serphone_call_asked_to_autoanswer(SerPhoneCall *call){
	//return TRUE if the unique(for the moment) incoming call asked to be autoanswered
	if(call)
		return sal_call_autoanswer_asked(call->op);
	else
		return FALSE;
}

/**
 * Returns call's duration in seconds.
**/
int serphone_call_get_duration(const SerPhoneCall *call){
	if (call->media_start_time==0) return 0;
	return   (int)(time(NULL)-call->media_start_time);
}

/**
 * Returns the call object this call is replacing, if any.
 * Call replacement can occur during call transfers.
 * By default, the core automatically terminates the replaced call and accept the new one.
 * This function allows the application to know whether a new incoming call is a one that replaces another one.
**/
SerPhoneCall *serphone_call_get_replaced_call(SerPhoneCall *call){
	SalOp *op=sal_call_get_replaces(call->op);
	if (op){
		return (SerPhoneCall*)sal_op_get_user_pointer(op);
	}
	return NULL;
}

/**
 * Returns true if this calls has received a transfer that has not been
 * executed yet.
 * Pending transfers are executed when this call is being paused or closed,
 * locally or by remote endpoint.
 * If the call is already paused while receiving the transfer request, the
 * transfer immediately occurs.
**/
bool_t serphone_call_has_transfer_pending(const SerPhoneCall *call){
	return call->refer_pending;
}

/**
 * Get the user_pointer in the SerPhoneCall
 *
 * @ingroup call_control
 *
 * return user_pointer an opaque user pointer that can be retrieved at any time
**/
void *serphone_call_get_user_pointer(SerPhoneCall *call)
{
	return call->user_pointer;
}

/**
 * Set the user_pointer in the LinphoneCall
 *
 * @ingroup call_control
 *
 * the user_pointer is an opaque user pointer that can be retrieved at any time in the LinphoneCall
**/
void serphone_call_set_user_pointer(SerPhoneCall *call, void *user_pointer)
{
	call->user_pointer = user_pointer;
}

/**
 * Retrieves the call's current state.
**/
SerphoneCallState serphone_call_get_state(const SerPhoneCall *call)
{
	return call->state;
}

SerPhoneCall * is_a_linphone_call(void *user_pointer)
{
	SerPhoneCall *call=(SerPhoneCall*)user_pointer;
	if (call==NULL) return NULL;
	return call->magic==serphone_call_magic ? call : NULL;
}

void serphone_call_fix_call_parameters(SerPhoneCall *call)
{
	call->params.has_video=call->current_params.has_video;
	call->params.media_encryption=call->current_params.media_encryption;
}

RtpProfile *make_profile(SerPhoneCall *call, const SalMediaDescription *md, const SalStreamDescription *desc, int *used_pt)
{
	int bw;
	const MSList *elem;
	RtpProfile *prof=rtp_profile_new("Call profile");
	bool_t first=TRUE;
	int remote_bw=0;
	ServiceCore *lc=call->core;
	int up_ptime=0;
	*used_pt=-1;

	for(elem=desc->payloads;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		int number;

		if ((pt->flags & PAYLOAD_TYPE_FLAG_CAN_SEND) && first) {
			if (desc->type==SalAudio){
				serphone_core_update_allocated_audio_bandwidth_in_call(call,pt);
				up_ptime=lc->serphone_core_get_upload_ptime();
			}
			*used_pt=payload_type_get_number(pt);
			first=FALSE;
		}
		if (desc->bandwidth>0) remote_bw=desc->bandwidth;
		else if (md->bandwidth>0) {
			/*case where b=AS is given globally, not per stream*/
			remote_bw=md->bandwidth;
			if (desc->type==SalVideo){
				remote_bw=get_video_bandwidth(remote_bw,call->audio_bw);
			}
		}

		if (desc->type==SalAudio){
				bw=get_min_bandwidth(call->audio_bw,remote_bw);
		}else bw=get_min_bandwidth(get_video_bandwidth(lc->serphone_core_get_upload_bandwidth(),call->audio_bw),remote_bw);
		if (bw>0) pt->normal_bitrate=bw*1000;
		else if (desc->type==SalAudio){
			pt->normal_bitrate=-1;
		}
		if (desc->ptime>0){
			up_ptime=desc->ptime;
		}
		if (up_ptime>0){
			char tmp[40];
			snprintf(tmp,sizeof(tmp),"ptime=%i",up_ptime);
			payload_type_append_send_fmtp(pt,tmp);
		}
		number=payload_type_get_number(pt);
		if (rtp_profile_get_payload(prof,number)!=NULL){
			PrintConsole("A payload type with number %i already exists in profile !\n",number);
		}else
			rtp_profile_set_payload(prof,number,pt);
	}
	return prof;
}

void serphone_core_update_allocated_audio_bandwidth_in_call(SerPhoneCall *call, const PayloadType *pt)
{
	call->audio_bw=(int)(call->core->get_audio_payload_bandwidth(pt)/1000.0);
	PrintConsole("Audio bandwidth for this call is %i\n",call->audio_bw);
}

void serphone_call_log_completed(SerPhoneCall *call)
{
	ServiceCore *lc=call->core;

	call->log->duration=(int)(time(NULL)-call->start_time);

	if (call->log->status==LinphoneCallMissed){
		char *info;
		lc->missed_calls++;
		info=ser_strdup_printf(ngettext("You have missed %i call.",
                                         "You have missed %i calls.", lc->missed_calls),
                                lc->missed_calls);
        if (lc->vtable.display_status!=NULL)
            lc->vtable.display_status(lc,info);
		ms_free((void **)&info);
	}
	lc->call_logs=ms_list_prepend(lc->call_logs,(void *)call->log);
	if (ms_list_size(lc->call_logs)>lc->max_call_logs){
		MSList *elem,*prevelem=NULL;
		/*find the last element*/
		for(elem=lc->call_logs;elem!=NULL;elem=elem->next){
			prevelem=elem;
		}
		elem=prevelem;
		serphone_call_log_destroy((SerphoneCallLog**)&elem->data);
		lc->call_logs=ms_list_remove_link(lc->call_logs,elem);
	}
	if (lc->vtable.call_log_updated!=NULL){
		lc->vtable.call_log_updated(lc,call->log);
	}
	call_logs_write_to_config_file(lc);
}

void call_logs_write_to_config_file(ServiceCore *lc)
{
	MSList *elem;
	char logsection[32];
	int i;
//	char *tmp;
	LpConfig *cfg=lc->config;

	if (lc->serphone_core_get_global_state ()==LinphoneGlobalStartup) return;

	for(i=0,elem=lc->call_logs;elem!=NULL;elem=elem->next,++i){
		SerphoneCallLog *cl=(SerphoneCallLog*)elem->data;
/*
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		lp_config_set_int(cfg,logsection,"dir",cl->dir);
		lp_config_set_int(cfg,logsection,"status",cl->status);
		tmp=serphone_address_as_string(cl->from);
		lp_config_set_string(cfg,logsection,"from",tmp);
		ms_free(tmp);
		tmp=serphone_address_as_string(cl->to);
		lp_config_set_string(cfg,logsection,"to",tmp);
		ms_free(tmp);
		lp_config_set_string(cfg,logsection,"start_date",cl->start_date);
		lp_config_set_int(cfg,logsection,"duration",cl->duration);
		if (cl->refkey) lp_config_set_string(cfg,logsection,"refkey",cl->refkey);
		lp_config_set_float(cfg,logsection,"quality",cl->quality);
        lp_config_set_int(cfg,logsection,"video_enabled", cl->video_enabled);
*/
	}

	for(;i<lc->max_call_logs;++i){
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		lp_config_clean_section(cfg,logsection);
	}
}
