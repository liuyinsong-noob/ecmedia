

#define   _CRTDBG_MAP_ALLOC 
#include "servicecore.h"
#include "serphonecall.h"
#include "friends.h"
#include "lpconfig.h"
#include "enum.h"
#include "sal_eXosip2.h"

#ifdef  WIN32   
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

bool_t serphone_chat_room_matches(SerphoneChatRoom *cr, const SerphoneAddress *from){
	if (serphone_address_get_username(cr->peer_url) && serphone_address_get_username(from) && 
		strcmp(serphone_address_get_username(cr->peer_url),serphone_address_get_username(from))==0) return TRUE;
	return FALSE;
}

void serphone_chat_room_text_received(SerphoneChatRoom *cr, ServiceCore *lc, const SerphoneAddress *from, const SerphoneAddress *to, const char *msgid,
	const char *msg, const char *date){
	if (lc->vtable.text_received!=NULL) lc->vtable.text_received(lc, cr, from, to, msgid, msg, date);
}

void ServiceCore::serphone_core_text_received(const char *from, const char *to, const char *msgid, const char *msg, const char *date)
{
//	MSList *elem;
//	SerphoneChatRoom *cr=NULL;
//	SerphoneAddress *addr;
//	SerphoneAddress *toAddr;
//	char *cleanfrom;
//
//	toAddr=serphone_address_new(to);
//	serphone_address_clean(toAddr);
//
//	addr=serphone_address_new(from);
//	serphone_address_clean(addr);
//	for(elem=this->chatrooms;elem!=NULL;elem=ms_list_next(elem)){
//		cr=(SerphoneChatRoom*)elem->data;
//		if (serphone_chat_room_matches(cr,addr)){
//			break;
//		}
//		cr=NULL;
//	}
//	cleanfrom=serphone_address_as_string(addr);
//	if (cr==NULL){
//		/* create a new chat room */
//		cr=serphone_core_create_chat_room(cleanfrom);
//	}
//	serphone_address_destroy(addr);
//	serphone_chat_room_text_received(cr,this,cr->peer_url, toAddr, msg);
//	serphone_address_destroy(toAddr);
//	ms_free(cleanfrom);
//    MSList *elem;
//	SerphoneChatRoom *cr=NULL;
	SerphoneAddress *addr;
	SerphoneAddress *toAddr;
//	char *cleanfrom;
    
    std::map<std::string, bool>::iterator it = filterDupMessage.find(msgid);
    if (it != filterDupMessage.end()) {
        return; //Duplicate message, discard!
    }
    else
    {
        filterDupMessage.insert(std::pair<std::string, bool>(msgid, true));
    }
	toAddr=serphone_address_new(to);
	serphone_address_clean(toAddr);
    
	addr=serphone_address_new(from);
	serphone_address_clean(addr);
    
	serphone_chat_room_text_received(NULL,this,addr, toAddr, msgid, msg,date);
	serphone_address_destroy(toAddr);
    serphone_address_destroy(addr);
}

const char *ServiceCore::serphone_core_send_text(const char*to, const char*msg)
{
	yuntongxunwebrtc::CriticalSectionScoped lock(m_criticalSection);
	MSList *elem;
	SerphoneChatRoom *cr=NULL;
	SerphoneAddress *addr;
	char *cleanto;

	addr=serphone_core_interpret_url(to);
    if(!addr)
        return "";
    
	serphone_address_clean(addr);
	for(elem=this->chatrooms;elem!=NULL;elem=ms_list_next(elem)){
		cr=(SerphoneChatRoom*)elem->data;
		if (serphone_chat_room_matches(cr,addr)){
			break;
		}
		cr=NULL;
	}
	cleanto=serphone_address_as_string(addr);
	if (cr==NULL){
		/* create a new chat room */
		cr=serphone_core_create_chat_room(cleanto);
	}
	serphone_address_destroy(addr);
	ms_free((void **)&cleanto);
    cleanto = NULL;
	if(!cr)
		return "";
	return serphone_chat_room_send_message(cr,msg);
}

SerphoneChatRoom * ServiceCore::serphone_core_create_chat_room(const char *to)
{
	SerphoneAddress *parsed_url=NULL;

	if ((parsed_url=serphone_core_interpret_url(to))!=NULL){
		SerphoneChatRoom *cr=(SerphoneChatRoom *)malloc(sizeof(SerphoneChatRoom)*1);  
		memset((void *)cr,0,sizeof(SerphoneChatRoom)*1);

		cr->lc=this;
		cr->peer=serphone_address_as_string(parsed_url);
		cr->peer_url=parsed_url;
		this->chatrooms=ms_list_append(this->chatrooms,(void *)cr);
		return cr;
	}
	return NULL;
}

void serphone_chat_room_destroy1(SerphoneChatRoom **cr)
{
    ServiceCore *lc=(*cr)->lc;
    serphone_address_destroy((*cr)->peer_url);
    ms_free((void **)&(*cr)->peer);
    (*cr)->peer = NULL;
    ms_free((void **)cr);
}

void serphone_chat_room_destroy(SerphoneChatRoom **cr)
{
	ServiceCore *lc=(*cr)->lc;
	//lc->chatrooms=ms_list_remove(lc->chatrooms,(void *) cr);
	serphone_address_destroy((*cr)->peer_url);
	ms_free((void **)&(*cr)->peer);
    (*cr)->peer = NULL;
	if ((*cr)->op)
		 sal_op_release((*cr)->op);
	ms_free((void **)cr);
}

const SerphoneAddress* serphone_chat_room_get_peer_address(SerphoneChatRoom *cr)
{
	return cr->peer_url;
}

const char *serphone_chat_room_send_message(SerphoneChatRoom *cr, const char *msg)
{
	static int msgid =1;
	const char *route=NULL;
	const char *identity=cr->lc->serphone_core_find_best_identity(cr->peer_url,&route);
	SalOp *op=NULL;
	//hubin modified, 如果判读出在通话中发送IM消息，发送的所有IM消息的call-id会相同。  
	//SerPhoneCall *call;
	//if((call = cr->lc->serphone_core_get_call_by_remote_address(cr->peer))!=NULL){
	//	if (call->state==LinphoneCallConnected ||
	//	    call->state==LinphoneCallStreamsRunning ||
	//	    call->state==LinphoneCallPaused ||
	//	    call->state==LinphoneCallPausing ||
	//	    call->state==LinphoneCallPausedByRemote){
	//		op = call->op;
	//	}
	//}
	if (op==NULL){
		/*sending out of calls*/
		op = sal_op_new(cr->lc->sal);
		sal_op_set_route(op,route);
		/*if (cr->op!=NULL){
			sal_op_release (cr->op);
			cr->op=NULL;
		}
		cr->op=op;*/
	}
	if(op)
	{
		op->mid = ++msgid;
        char *callid = sal_text_send(op,identity,cr->peer,msg);
        if(!callid)
            return "";
        
        if(op->msgid)
        {
			ms_free((void **)&op->msgid);
            op->msgid = NULL;
        }
		op->msgid = (char*)ms_malloc(128);
		strcpy(op->msgid, callid);
        osip_free(callid);
        return op->msgid;
	}
    
    return "";
}

void serphone_chat_room_set_user_data(SerphoneChatRoom *cr, void * ud)
{
	cr->user_data=ud;
}

void * serphone_chat_room_get_user_data(SerphoneChatRoom *cr)
{
	return cr->user_data;
}

