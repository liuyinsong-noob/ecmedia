/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sal_eXosip2.h"
#include "offeranswer.h"
#include "servicecore.h"

extern FILE *traceFile;
extern std::string timetodate(time_t const timer);
//#ifdef ANDROID
//// Necessary to make it linked
//static void for_linker() { eXosip_transport_hook_register(NULL); }
//#endif
static bool_t call_failure(Sal *sal, eXosip_event_t *ev);

static void text_received(Sal *sal, eXosip_event_t *ev);

static void masquerade_via(osip_message_t *msg, const char *ip, const char *port);
static bool_t fix_message_contact(SalOp *op, osip_message_t *request,osip_message_t *last_answer, bool_t expire_last_contact);
static void update_contact_from_response(SalOp *op, osip_message_t *response);

void sal_use_dates(Sal *ctx, bool_t enabled);
void print_sdp(sdp_message_t *sdp);

void _osip_list_set_empty(osip_list_t *l, void (*freefunc)(void*)){
	void *data;
	while(!osip_list_eol(l,0)) {
		data=osip_list_get(l,0);
		osip_list_remove(l,0);
		if (data) freefunc(data);
	}
}

void sal_get_default_local_ip(Sal *sal, int address_family,char *ip, size_t iplen){
	if (eXosip_guess_localip(address_family,ip,iplen)<0){
		/*default to something */
		strncpy(ip,address_family==AF_INET6 ? "::1" : "127.0.0.1",iplen);
		PrintConsole("sal_eXosip2 Could not find default routable ip address !\n");
	}
}


static SalOp * sal_find_call(Sal *sal, int cid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->calls;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->cid==cid) return op;
	}
	return NULL;
}

static void sal_add_call(Sal *sal, SalOp *op){
	sal->calls=ms_list_append(sal->calls,op);
}

static void sal_remove_call(Sal *sal, SalOp *op){
	sal->calls=ms_list_remove(sal->calls, op);
}

static SalOp * sal_find_register(Sal *sal, int rid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->registers;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->rid==rid) return op;
	}
	return NULL;
}

static void sal_add_register(Sal *sal, SalOp *op){
	sal->registers=ms_list_append(sal->registers,op);
}

static void sal_remove_register(Sal *sal, int rid){
	MSList *elem;
	SalOp *op;
	for(elem=sal->registers;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->rid==rid) {
			sal->registers=ms_list_remove_link(sal->registers,elem);
			return;
		}
	}
}

static SalOp * sal_find_other(Sal *sal, osip_message_t *message){
	const MSList *elem;
	SalOp *op;
	osip_call_id_t *callid=osip_message_get_call_id(message);
	if (callid==NULL) {
		PrintConsole("sal_eXosip2 There is no call-id in this message !\n");
		return NULL;
	}
	for(elem=sal->other_transactions;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (osip_call_id_match(callid,op->call_id)==0) return op;
	}
	return NULL;
}

void sal_add_other(Sal *sal, SalOp *op, osip_message_t *request){
	osip_call_id_t *callid=osip_message_get_call_id(request);
	if (callid==NULL) {
		PrintConsole("sal_eXosip2 There is no call id in the request !\n");
		return;
	}
	osip_call_id_clone(callid,&op->call_id);
	sal->other_transactions=ms_list_append(sal->other_transactions,op);
}

static void sal_remove_other(Sal *sal, SalOp *op){
	sal->other_transactions=ms_list_remove(sal->other_transactions,op);
}


static void sal_add_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_append(sal->pending_auths,op);
}


static void sal_remove_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_remove(sal->pending_auths,op);
}

void sal_exosip_fix_route(SalOp *op){
	if (sal_op_get_route(op)!=NULL){
		osip_route_t *rt=NULL;
		osip_uri_param_t *lr_param=NULL;
		
		osip_route_init(&rt);
		if (osip_route_parse(rt,sal_op_get_route(op))<0){
			PrintConsole("sal_eXosip2 Bad route  %s!\n",sal_op_get_route(op));
			sal_op_set_route(op,NULL);
		}else{
			/* check if the lr parameter is set , if not add it */
			osip_uri_uparam_get_byname(rt->url, "lr", &lr_param);
		  	if (lr_param==NULL){
				char *tmproute;
				osip_uri_uparam_add(rt->url,osip_strdup("lr"),NULL);
				osip_route_to_str(rt,&tmproute);
				sal_op_set_route(op,tmproute);
				osip_free(tmproute);
			}
		}
		osip_route_free(rt);
	}
}

SalOp * sal_op_new(Sal *sal){
//	SalOp *op=ms_new0(SalOp,1);
    SalOp *op=(SalOp *)malloc(sizeof(SalOp)*1);  //ms_new
	__sal_op_init(op,sal);
//	op->cid=op->did=op->tid=op->rid=op->nid=op->sid=-1;
    op->cid=op->did=op->tid=op->rid=op->nid=op->sid=op->mid=-1;
	op->result=NULL;
	op->supports_session_timers=FALSE;
	op->sdp_offering=TRUE;
	op->pending_auth=NULL;
	op->sdp_answer=NULL;
	op->reinvite=FALSE;
	op->call_id=NULL;
	op->replaces=NULL;
	op->referred_by=NULL;
	op->masquerade_via=FALSE;
	op->auto_answer_asked=FALSE;
	op->auth_info=NULL;
	op->terminated=FALSE;
    op->invite_userdata = NULL;
    op->group_id = NULL;
    
//haiyuntong
#ifdef HAIYUNTONG
    op->akey = NULL;
    op->bkey = NULL;
    op->confKey = NULL;
#endif
    
    op->msgid = NULL;
	op->check_account_online = FALSE;
	return op;
}

bool_t sal_call_autoanswer_asked(SalOp *op)
{
	return op->auto_answer_asked;
}

void sal_op_release(SalOp *op){
	if (op->sdp_answer)
		sdp_message_free(op->sdp_answer);
	if (op->pending_auth)
		eXosip_event_free(op->pending_auth);
	if (op->rid!=-1){
		sal_remove_register(op->base.root,op->rid);
		eXosip_register_remove(op->rid);
	}
	if (op->cid!=-1){
		PrintConsole("sal_eXosip2 Cleaning cid %i\n",op->cid);
		sal_remove_call(op->base.root,op);
	}
	if (op->sid!=-1){
		sal_remove_out_subscribe(op->base.root,op);
	}
	if (op->nid!=-1){
		sal_remove_in_subscribe(op->base.root,op);
		if (op->call_id)
			osip_call_id_free(op->call_id);
		op->call_id=NULL;
	}
	if (op->pending_auth){
		sal_remove_pending_auth(op->base.root,op);
	}
	if (op->result)
		sal_media_description_unref(&(op->result));
	if (op->call_id){
		sal_remove_other(op->base.root,op);
		osip_call_id_free(op->call_id);
        op->call_id = NULL;
	}
	if (op->replaces){
		ms_free((void **)&op->replaces);
	}
	if (op->referred_by){
		ms_free((void **)&op->referred_by);
	}
	if (op->auth_info) {
		sal_auth_info_delete(op->auth_info);
        op->auth_info = NULL;
	}
    if( op->invite_userdata) {
		ms_free((void **)&op->invite_userdata);
	}
    if( op->group_id) {
		ms_free((void **)&op->group_id);
	}
    
//haiyuntong
#ifdef HAIYUNTONG
    if( op->akey) {
        ms_free((void **)&op->akey);
    }
    if( op->bkey) {
        ms_free((void **)&op->bkey);
    }
    if( op->confKey) {
        ms_free((void **)&op->confKey);
    }
#endif
    
    if( op->msgid) {
		ms_free((void **)&op->msgid);
	}
	if( op->base.call_id) {
		ms_free((void **)&op->base.call_id);
	}
	__sal_op_free(op);
}

static void _osip_trace_func(char *fi, int li, osip_trace_level_t level, char *chfr, va_list ap){
//	int ortp_level=ORTP_DEBUG;
//	switch(level){
//		case OSIP_INFO1:
//		case OSIP_INFO2:
//		case OSIP_INFO3:
//		case OSIP_INFO4:
//			ortp_level=ORTP_MESSAGE;
//			break;
//		case OSIP_WARNING:
//			ortp_level=ORTP_WARNING;
//			break;
//		case OSIP_ERROR:
//		case OSIP_BUG:
//			ortp_level=ORTP_ERROR;
//			break;
//		case OSIP_FATAL:
//			ortp_level=ORTP_FATAL;
//			break;
//		case END_TRACE_LEVEL:
//			break;
//	}
//	if (ortp_log_level_enabled(level)){
//		int len=strlen(chfr);
//		char *chfrdup=ortp_strdup(chfr);
//		/*need to remove endline*/
//		if (len>1){
//			if (chfrdup[len-1]=='\n')
//				chfrdup[len-1]='\0';
//			if (chfrdup[len-2]=='\r')
//				chfrdup[len-2]='\0';
//		}
//		ortp_logv(ortp_level,chfrdup,ap);
//		ortp_free(chfrdup);
//	}
    char log_buffer[2048];
#ifndef WIN32
    vsnprintf(log_buffer, 2047, chfr, ap);
#else
    _vsnprintf(log_buffer, 2047, chfr, ap);
#endif
    PrintConsole("sal_eXosip2 %s\n",log_buffer);
}

static void osip_message_hook_func( int received,const char *host, int port, const char *buf)
{
	const char *p = strstr(buf,"\r\n");
	char firstLine[256] = {0};
	strncpy(firstLine,buf,p-buf > 255 ? 255 : p-buf);
	PrintConsole("sal_eXosip2 [SIP] %s %s %s %s %d\n",received ? "Recv" : "Send", firstLine,received ? "From:" : "To:", host,port);
}

Sal * sal_init(){
	static bool_t firsttime=TRUE;
	Sal *sal;
	if (firsttime){
		osip_trace_initialize_func (OSIP_INFO4,&_osip_trace_func);
        osip_set_message_hook_func(osip_message_hook_func);
		firsttime=FALSE;
	}
	eXosip_init();
//	sal=ms_new0(Sal,1);
    sal=(Sal *)malloc(sizeof(Sal)*1);
	memset((void *)sal,0,sizeof(Sal)*1);
	sal->keepalive_period=30;
	sal->double_reg=TRUE;
	sal->use_rports=TRUE;
	sal->use_101=TRUE;
	sal->reuse_authorization=FALSE;
	sal->rootCa = 0;
	sal->verify_server_certs=TRUE;
	sal->verify_server_cn=TRUE;
	sal->expire_old_contact=FALSE;
	sal->add_dates=FALSE;
	sal->dscp=-1;
	return sal;
}

void sal_reinit_network() {
	eXosip_reinit_network();
}

void sal_uninit(Sal* sal){
	eXosip_quit();
	if (sal->rootCa)
    {
        ms_free((void **)&sal->rootCa);
    }
	ms_free((void **)&sal);
}

void sal_set_user_pointer(Sal *sal, void *user_data){
	sal->up=user_data;
}

void *sal_get_user_pointer(const Sal *sal){
	return sal->up;
}

static void unimplemented_stub(){
	PrintConsole("sal_eXosip2 Unimplemented SAL callback\n");
}

void sal_set_callbacks(Sal *ctx, const SalCallbacks *cbs){
	memcpy(&ctx->callbacks,cbs,sizeof(*cbs));
	if (ctx->callbacks.call_received==NULL) 
		ctx->callbacks.call_received=(SalOnCallReceived)unimplemented_stub;
	if (ctx->callbacks.call_ringing==NULL) 
		ctx->callbacks.call_ringing=(SalOnCallRinging)unimplemented_stub;
	if (ctx->callbacks.call_accepted==NULL) 
		ctx->callbacks.call_accepted=(SalOnCallAccepted)unimplemented_stub;
	if (ctx->callbacks.call_proceeding==NULL)
		ctx->callbacks.call_proceeding=(SalOnCallProceeding)unimplemented_stub;
	if (ctx->callbacks.call_failure==NULL) 
		ctx->callbacks.call_failure=(SalOnCallFailure)unimplemented_stub;
	if (ctx->callbacks.call_terminated==NULL) 
		ctx->callbacks.call_terminated=(SalOnCallTerminated)unimplemented_stub;
	if (ctx->callbacks.call_released==NULL)
		ctx->callbacks.call_released=(SalOnCallReleased)unimplemented_stub;
	if (ctx->callbacks.call_updating==NULL) 
		ctx->callbacks.call_updating=(SalOnCallUpdating)unimplemented_stub;
	if (ctx->callbacks.auth_requested==NULL) 
		ctx->callbacks.auth_requested=(SalOnAuthRequested)unimplemented_stub;
	if (ctx->callbacks.auth_success==NULL) 
		ctx->callbacks.auth_success=(SalOnAuthSuccess)unimplemented_stub;
	if (ctx->callbacks.register_success==NULL) 
		ctx->callbacks.register_success=(SalOnRegisterSuccess)unimplemented_stub;
	if (ctx->callbacks.register_failure==NULL) 
		ctx->callbacks.register_failure=(SalOnRegisterFailure)unimplemented_stub;
	if (ctx->callbacks.dtmf_received==NULL) 
		ctx->callbacks.dtmf_received=(SalOnDtmfReceived)unimplemented_stub;
	if (ctx->callbacks.notify==NULL)
		ctx->callbacks.notify=(SalOnNotify)unimplemented_stub;
	if (ctx->callbacks.notify_presence==NULL)
		ctx->callbacks.notify_presence=(SalOnNotifyPresence)unimplemented_stub;
	if (ctx->callbacks.subscribe_received==NULL)
		ctx->callbacks.subscribe_received=(SalOnSubscribeReceived)unimplemented_stub;
	if (ctx->callbacks.text_received==NULL)
		ctx->callbacks.text_received=(SalOnTextReceived)unimplemented_stub;
	if (ctx->callbacks.ping_reply==NULL)
		ctx->callbacks.ping_reply=(SalOnPingReply)unimplemented_stub;
//Sean add begin 20131022 for video fast update in video conference
    if (ctx->callbacks.vfu_request==NULL) {
        ctx->callbacks.vfu_request=(SalOnVfuRequest)unimplemented_stub;
    }
//Sean add end 20131022 for video fast update in video conference
	if (ctx->callbacks.option_reply==NULL)
		ctx->callbacks.option_reply=(SalOnOptionReply)unimplemented_stub;
}

int sal_unlisten_ports(Sal *ctx){
	if (ctx->running){
		eXosip_quit();
		eXosip_init();
		ctx->running=FALSE;
	}
	return 0;
}

int sal_reset_transports(Sal *ctx){
#ifdef HAVE_EXOSIP_RESET_TRANSPORTS
	if (ctx->running){
		ms_message("Exosip transports reset.");
		eXosip_reset_transports();
	}
	return 0;
#else
	PrintConsole("sal_eXosip2 sal_reset_transports() not implemented in this version.\n");
	return -1;
#endif
}


static void set_tls_options(Sal *ctx){
	if (ctx->rootCa) {
		eXosip_tls_ctx_t tlsCtx;
		memset(&tlsCtx, 0, sizeof(tlsCtx));
		snprintf(tlsCtx.root_ca_cert, sizeof(tlsCtx.client.cert), "%s", ctx->rootCa);
		eXosip_set_tls_ctx(&tlsCtx);
	}                       
#ifdef HAVE_EXOSIP_TLS_VERIFY_CERTIFICATE
	eXosip_tls_verify_certificate(ctx->verify_server_certs);
#endif
#ifdef HAVE_EXOSIP_TLS_VERIFY_CN
	eXosip_tls_verify_cn(ctx->verify_server_cn);
#endif
}

void sal_set_dscp(Sal *ctx, int dscp){
	ctx->dscp=dscp;
#ifdef HAVE_EXOSIP_DSCP
	if (dscp!=-1)
		eXosip_set_option(EXOSIP_OPT_SET_DSCP,&ctx->dscp);
#endif
}

int sal_check_sip_thread_active()
{
    return eXosip_check_sip_active();
}

int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_secure){
	int err;
	bool_t ipv6;
	int proto=IPPROTO_UDP;
	int keepalive = ctx->keepalive_period;

	ctx->transport = tr;
	switch (tr) {
	case SalTransportUDP:
		proto=IPPROTO_UDP;
		eXosip_set_option (EXOSIP_OPT_UDP_KEEP_ALIVE, &keepalive);
		break;
	case SalTransportTCP:
        proto= IPPROTO_TCP;
        eXosip_set_option (EXOSIP_OPT_UDP_KEEP_ALIVE,&keepalive);
        set_tls_options(ctx);
        break;
	case SalTransportTLS:
		proto= IPPROTO_TCP;
		if (!ctx->tcp_tls_keepalive) keepalive=-1;
		eXosip_set_option (EXOSIP_OPT_UDP_KEEP_ALIVE,&keepalive);
		set_tls_options(ctx);
		break;
	default:
		PrintConsole("sal_eXosip2 unexpected proto, using datagram\n");
	}
	/*see if it looks like an IPv6 address*/
	int use_rports = ctx->use_rports; // Copy char to int to avoid bad alignment
	eXosip_set_option(EXOSIP_OPT_USE_RPORT,&use_rports);
	int dont_use_101 = !ctx->use_101; // Copy char to int to avoid bad alignment
	eXosip_set_option(EXOSIP_OPT_DONT_SEND_101,&dont_use_101);
	sal_set_dscp(ctx,ctx->dscp);
	sal_use_dates(ctx,ctx->add_dates);

	ipv6=strchr(addr,':')!=NULL;
	eXosip_enable_ipv6(ipv6);

	if (is_secure && tr == SalTransportUDP){
		PrintConsole("sal_eXosip2 SIP over DTLS is not supported yet.\n");
		return -1;
	}
	err=eXosip_listen_addr(proto, addr, port, ipv6 ?  PF_INET6 : PF_INET, is_secure);
	ctx->running=TRUE;
	return err;
}

ortp_socket_t sal_get_socket(Sal *ctx){
#ifdef HAVE_EXOSIP_GET_SOCKET
	return eXosip_get_socket(IPPROTO_UDP);
#else
	PrintConsole("sal_eXosip2 Sorry, eXosip does not have eXosip_get_socket() method\n");
	return -1;
#endif
}

void sal_set_user_agent(Sal *ctx, const char *user_agent){
	eXosip_set_user_agent(user_agent);
}

void sal_use_session_timers(Sal *ctx, int expires){
	ctx->session_expires=expires;
}

void sal_use_one_matching_codec_policy(Sal *ctx, bool_t one_matching_codec){
	ctx->one_matching_codec=one_matching_codec;
}

MSList *sal_get_pending_auths(Sal *sal){
	return ms_list_copy(sal->pending_auths);
}

void sal_use_double_registrations(Sal *ctx, bool_t enabled){
	ctx->double_reg=enabled;
}

void sal_expire_old_registration_contacts(Sal *ctx, bool_t enabled){
	ctx->expire_old_contact=enabled;
}

void sal_use_dates(Sal *ctx, bool_t enabled){
	ctx->add_dates=enabled;
#ifdef EXOSIP_OPT_REGISTER_WITH_DATE
	{
		int tmp=enabled;
		eXosip_set_option(EXOSIP_OPT_REGISTER_WITH_DATE,&tmp);
	}
#else
	if (enabled) PrintConsole("sal_eXosip2 Exosip does not support EXOSIP_OPT_REGISTER_WITH_DATE option.\n");
#endif
}

void sal_use_rport(Sal *ctx, bool_t use_rports){
	ctx->use_rports=use_rports;
}
void sal_use_101(Sal *ctx, bool_t use_101){
	ctx->use_101=use_101;
}

void sal_set_root_ca(Sal* ctx, const char* rootCa) {
	if (ctx->rootCa)
    {
        ms_free((void **)&ctx->rootCa);
    }
	ctx->rootCa = ms_strdup(rootCa);
	set_tls_options(ctx);
}

const char *sal_get_root_ca(Sal* ctx) {
	return ctx->rootCa;
}

void sal_verify_server_certificates(Sal *ctx, bool_t verify){
	ctx->verify_server_certs=verify;
#ifdef HAVE_EXOSIP_TLS_VERIFY_CERTIFICATE
	eXosip_tls_verify_certificate(verify);
#endif
}

void sal_verify_server_cn(Sal *ctx, bool_t verify){
	ctx->verify_server_cn=verify;
#ifdef HAVE_EXOSIP_TLS_VERIFY_CN
	eXosip_tls_verify_cn(verify);
#endif
}

static int extract_received_rport(osip_message_t *msg, const char **received, int *rportval,SalTransport* transport){
	osip_via_t *via=NULL;
	osip_generic_param_t *param=NULL;
	const char *rport=NULL;

	*rportval=5060;
	*received=NULL;
	osip_message_get_via(msg,0,&via);
	if (!via) {
		PrintConsole("sal_eXosip2 extract_received_rport(): no via.\n");
		return -1;
	}

	*transport = sal_transport_parse(via->protocol);
	
	if (via->port && via->port[0]!='\0')
		*rportval=atoi(via->port);
	
	osip_via_param_get_byname(via,"rport",&param);
	if (param) {
		rport=param->gvalue;
		if (rport && rport[0]!='\0') *rportval=atoi(rport);
		*received=via->host;
	}
	param=NULL;
	osip_via_param_get_byname(via,"received",&param);
	if (param) *received=param->gvalue;

	if (rport==NULL && *received==NULL){
		PrintConsole("sal_eXosip2 extract_received_rport(): no rport and no received parameters.\n");
		return -1;
	}
	return 0;
}

static void set_sdp(osip_message_t *sip,sdp_message_t *msg){
	int sdplen;
	char clen[10];
	char *sdp=NULL;
	sdp_message_to_str(msg,&sdp);
	sdplen=strlen(sdp);
	snprintf(clen,sizeof(clen),"%i",sdplen);
	osip_message_set_body(sip,sdp,sdplen);
	osip_message_set_content_type(sip,"application/sdp");
	osip_message_set_content_length(sip,clen);
	osip_free(sdp);
}

static void set_sdp_from_desc(osip_message_t *sip, const SalMediaDescription *desc){
//    printf("%s called\n",__FUNCTION__);
	sdp_message_t *msg=media_description_to_sdp(desc);
	if (msg==NULL) {
		PrintConsole("sal_eXosip2 Fail to print sdp message !\n");
		return;
	}
	set_sdp(sip,msg);
	sdp_message_free(msg);
}

static void sdp_process(SalOp *h){
	PrintConsole("sal_eXosip2 Doing SDP offer/answer process of type %s\n",h->sdp_offering ? "outgoing" : "incoming\n");
	if (h->result){
		sal_media_description_unref(&h->result);
	}
	h->result=sal_media_description_new();
	if (h->sdp_offering){
//        printf("555555555555 local_media\n");
//        osip_header_t *hh=NULL;
////        osip_message_header_get_byname(ev->request,"P-Call-UserData",0,&h);
////        osip_message_header_get_byname(media_description_to_sdp(h->base.remote_media),"Status-Line",0,&hh);
////        if( h) {
////            PrintConsole("sal_eXosip2 P-Call-UserData is %s\n",hh->hvalue );
////            //            op->invite_userdata = ms_strdup(h->hvalue);
////        }
//        print_sdp(media_description_to_sdp(h->base.remote_media));
//        printf("666666666666 remote_media\n");
		offer_answer_initiate_outgoing(h->base.local_media,h->base.remote_media,h->result);
	}else{
		int i;
		if (h->sdp_answer){
			sdp_message_free(h->sdp_answer);
		}
//        printf("111111111111 local_media\n");
//        print_sdp(media_description_to_sdp(h->base.local_media));
//        printf("222222222222 remote_media\n");
//        print_sdp(media_description_to_sdp(h->base.remote_media));
//        printf("333333333333\n");
		offer_answer_initiate_incoming(h->base.local_media,h->base.remote_media,h->result,h->base.root->one_matching_codec);
		h->sdp_answer=media_description_to_sdp(h->result);
//        print_sdp(h->sdp_answer);
		/*once we have generated the SDP answer, we modify the result description for processing by the upper layer.
		 It should contains media parameters constraint from the remote offer, not our response*/
		strcpy(h->result->addr,h->base.remote_media->addr);
		h->result->bandwidth=h->base.remote_media->bandwidth;
		
		for(i=0;i<h->result->n_active_streams;++i){
			strcpy(h->result->streams[i].rtp_addr,h->base.remote_media->streams[i].rtp_addr);
			strcpy(h->result->streams[i].rtcp_addr,h->base.remote_media->streams[i].rtcp_addr);
			h->result->streams[i].ptime=h->base.remote_media->streams[i].ptime;
			h->result->streams[i].bandwidth=h->base.remote_media->streams[i].bandwidth;
			h->result->streams[i].rtp_port=h->base.remote_media->streams[i].rtp_port;
			h->result->streams[i].rtcp_port=h->base.remote_media->streams[i].rtcp_port;
			if (h->result->streams[i].proto == SalProtoRtpSavp) {
				h->result->streams[i].crypto[0] = h->base.remote_media->streams[i].crypto[0];
			}
		}
		for(i=0;i<h->result->nstreams;++i){
			if (h->result->streams[i].port>0){
				strcpy(h->result->streams[i].addr,h->base.remote_media->streams[i].addr);
//				h->result->streams[i].ptime=h->base.remote_media->streams[i].ptime;
//				h->result->streams[i].bandwidth=h->base.remote_media->streams[i].bandwidth;
				h->result->streams[i].port=h->base.remote_media->streams[i].port;
				
//				if (h->result->streams[i].proto == SalProtoRtpSavp) {
//					h->result->streams[i].crypto[0] = h->base.remote_media->streams[i].crypto[0]; 
//				}
			}
		}
	}
	
}

int sal_call_is_offerer(const SalOp *h){
	return h->sdp_offering;
}

int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc){
	if (desc)
		sal_media_description_ref(desc);
	if (h->base.local_media)
		sal_media_description_unref(&h->base.local_media);
	h->base.local_media=desc;
	if (h->base.remote_media){
		/*case of an incoming call where we modify the local capabilities between the time
		 * the call is ringing and it is accepted (for example if you want to accept without video*/
		/*reset the sdp answer so that it is computed again*/
		if (h->sdp_answer){
			sdp_message_free(h->sdp_answer);
			h->sdp_answer=NULL;
		}
	}
	return 0;
}

int sal_call(SalOp *h, const char *from, const char *to){
	int err;
	const char *route;
	osip_message_t *invite=NULL;
	osip_call_id_t *callid;
	sal_op_set_from(h,from);
	sal_op_set_to(h,to);
	sal_exosip_fix_route(h);
	
	h->terminated = FALSE;

	route = sal_op_get_route(h);
	err=eXosip_call_build_initial_invite(&invite,to,from,route,"Phone call");
	if (err!=0){
		PrintConsole("sal_eXosip2 Could not create call. Error %d (from=%s to=%s route=%s)\n",
				err, from, to, route);
		return -1;
	}
	osip_message_set_allow(invite, "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
	if (h->base.contact){
		_osip_list_set_empty(&invite->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(invite,h->base.contact);
	}
	if (h->base.root->session_expires!=0){
		char expires[16]  = {'\0'};
		sprintf(expires, "%d", h->base.root->session_expires);
		osip_message_set_header(invite, "Session-expires", expires);
		osip_message_set_supported(invite, "timer");
	}
	if (h->base.local_media){
		h->sdp_offering=TRUE;
		set_sdp_from_desc(invite,h->base.local_media);
	}else h->sdp_offering=FALSE;
	if (h->replaces){
		osip_message_set_header(invite,"Replaces",h->replaces);
		if (h->referred_by)
			osip_message_set_header(invite,"Referred-By",h->referred_by);
	}
	if( h->invite_userdata)
		osip_message_set_header(invite,"P-Call-UserData",h->invite_userdata);
    if( h->group_id)
		osip_message_set_header(invite,"P-CallNetWork",h->group_id);

//haiyuntong
#ifdef HAIYUNTONG
    if( h->akey)
        osip_message_set_header(invite,"P-cipher-AK",h->akey);
#endif
    
	eXosip_lock();
	err=eXosip_call_send_initial_invite(invite);
	eXosip_unlock();
	h->cid=err;
	if (err<0){
		PrintConsole("sal_eXosip2 Fail to send invite ! Error code %d\n", err);
		return -1;
	}else{
		callid=osip_message_get_call_id(invite);
		osip_call_id_to_str(callid,(char **)(&h->base.call_id));
		sal_add_call(h->base.root,h);
	}
	return 0;
}

int sal_call_notify_ringing(SalOp *h, bool_t early_media){
//    printf("%s called\n",__FUNCTION__);
	osip_message_t *msg;
	
	/*if early media send also 180 and 183 */
	if (early_media){
		msg=NULL;
		eXosip_lock();
		eXosip_call_build_answer(h->tid,183,&msg);
		if (msg){
			sdp_process(h);
			if (h->sdp_answer){
				set_sdp(msg,h->sdp_answer);
				sdp_message_free(h->sdp_answer);
				h->sdp_answer=NULL;
			}
//haiyuntong
#ifdef HAIYUNTONG
			eXosip_call_send_answer(h->tid,183,msg, h->bkey, 0);
#else
            eXosip_call_send_answer(h->tid, 183, msg, NULL, -1);
#endif
		}
		eXosip_unlock();
	}else{
		eXosip_lock();
//haiyuntong
#ifdef HAIYUNTONG
		eXosip_call_send_answer(h->tid,180,NULL, h->bkey, 0);
#else
        eXosip_call_send_answer(h->tid,180,NULL, NULL, -1);
#endif
		eXosip_unlock();
	}
	return 0;
}

int sal_call_accept(SalOp * h){
//    printf("%s called\n",__FUNCTION__);
	osip_message_t *msg;
	const char *contact=sal_op_get_contact(h);
	/* sends a 200 OK */
	int err=eXosip_call_build_answer(h->tid,200,&msg);
	if (err<0 || msg==NULL){
		PrintConsole("sal_eXosip2 Fail to build answer for call: err=%i\n",err);
		return -1;
	}
	if (h->base.root->session_expires!=0){
		if (h->supports_session_timers) osip_message_set_supported(msg, "timer");
	}

	if (contact) {
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,contact);
	}
	
    if (h->base.local_media){
		/*this is the case where we received an invite without SDP*/
		if (h->sdp_offering) {
			set_sdp_from_desc(msg,h->base.local_media);
		}else{
			if (h->sdp_answer==NULL)
            {
                sdp_process(h);
            }
			if (h->sdp_answer){
				set_sdp(msg,h->sdp_answer);
				sdp_message_free(h->sdp_answer);
				h->sdp_answer=NULL;
			}
		}
	}else{
		PrintConsole("sal_eXosip2 You are accepting a call but not defined any media capabilities !\n");
	}
	eXosip_call_send_answer(h->tid,200,msg, NULL, 0);
	return 0;
}

int sal_call_decline(SalOp *h, SalReason reason, const char *redirect){
	if (reason==SalReasonBusy){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,486,NULL, NULL, 0);
		eXosip_unlock();
	}
	else if (reason==SalReasonTemporarilyUnavailable){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,480,NULL, NULL, 0);
		eXosip_unlock();
	}else if (reason==SalReasonDoNotDisturb){
		eXosip_lock();
		eXosip_call_send_answer(h->tid,600,NULL, NULL, 0);
		eXosip_unlock();
	}else if (reason==SalReasonMedia){
		eXosip_lock();
        PrintConsole("sal_eXosip2 [DEBUG HAIYUNTONG] decline media wrong");
		int ret = eXosip_call_send_answer(h->tid,415,NULL, NULL, 0);
        PrintConsole("sal_eXosip2 [DEBUG HAIYUNTONG] 415 return value ret:%d",ret);
		eXosip_unlock();
	}else if (redirect!=NULL && reason==SalReasonRedirect){
		osip_message_t *msg;
		int code;
		if (strstr(redirect,"sip:")!=0) code=302;
		else code=380;
		eXosip_lock();
		eXosip_call_build_answer(h->tid,code,&msg);
		osip_message_set_contact(msg,redirect);
		eXosip_call_send_answer(h->tid,code,msg, NULL, 0);
		eXosip_unlock();
	}else if(reason==SalReasonNoVoip){
		eXosip_lock();
		eXosip_call_send_answer(h->tid, 430,NULL, NULL, 0);
		eXosip_unlock();
	}else if(reason >= SalReasonDefineError){
        reason = (SalReason)((int)reason-603);
        char userDefined[100] = {0};
        sprintf(userDefined, "Q.850;cause=%d;text=\"USER_DEFINED_ERROR\"",reason);
        osip_message_t *msg = NULL;
		eXosip_lock();
        eXosip_call_build_answer(h->tid, 603, &msg);
        if (msg) {
            osip_message_set_header(msg,"Reason",userDefined);
        }
		eXosip_call_send_answer(h->tid, 603,msg, NULL, 0);
		eXosip_unlock();
	}else sal_call_terminate(h);
	return 0;
}

SalMediaDescription * sal_call_get_remote_media_description(SalOp *h){
	return h->base.remote_media;
}

SalMediaDescription * sal_call_get_final_media_description(SalOp *h){
	if (h->base.local_media && h->base.remote_media && !h->result){
		sdp_process(h);
	}
	return h->result;
}

int sal_call_set_referer(SalOp *h, SalOp *refered_call){
	if (refered_call->replaces)
		h->replaces=ms_strdup(refered_call->replaces);
	if (refered_call->referred_by)
		h->referred_by=ms_strdup(refered_call->referred_by);
	return 0;
}

static int send_notify_for_refer(int did, const char *sipfrag){
	osip_message_t *msg;
	eXosip_lock();
	eXosip_call_build_notify(did,EXOSIP_SUBCRSTATE_ACTIVE,&msg);
	if (msg==NULL){
		eXosip_unlock();
		PrintConsole("sal_eXosip2 Could not build NOTIFY for refer.\n");
		return -1;
	}
	osip_message_set_content_type(msg,"message/sipfrag");
	osip_message_set_header(msg,"Event","refer");
	osip_message_set_body(msg,sipfrag,strlen(sipfrag));
	eXosip_call_send_request(did,msg);
	eXosip_unlock();
	return 0;
}

/* currently only support to notify trying and 200Ok*/
int sal_call_notify_refer_state(SalOp *h, SalOp *newcall){
	if (newcall==NULL){
		/* in progress*/
		send_notify_for_refer(h->did,"SIP/2.0 100 Trying\r\n");
	}
	else if (newcall->cid!=-1){
		if (newcall->did==-1){
			/* not yet established*/
			if (!newcall->terminated){
				/* in progress*/
				send_notify_for_refer(h->did,"SIP/2.0 100 Trying\r\n");
			}
		}else{
			if (!newcall->terminated){
				if (send_notify_for_refer(h->did,"SIP/2.0 200 Ok\r\n")==-1){
					/* we need previous notify transaction to complete, so buffer the request for later*/
					h->sipfrag_pending="SIP/2.0 200 Ok\r\n";
				}
			}
		}
	}
	return 0;
}

int sal_ping(SalOp *op, const char *from, const char *to){
	osip_message_t *options=NULL;
	
	sal_op_set_from(op,from);
	sal_op_set_to(op,to);
	sal_exosip_fix_route(op);

	eXosip_options_build_request (&options, sal_op_get_to(op),
			sal_op_get_from(op),sal_op_get_route(op));
	if (options){
		if (op->base.root->session_expires!=0){
			char expires[16]  = {'\0'};
			sprintf(expires, "%d", op->base.root->session_expires);
			osip_message_set_header(options, "Session-expires", expires);
			osip_message_set_supported(options, "timer");
		}
		sal_add_other(sal_op_get_sal(op),op,options);
		return eXosip_options_send_request(options);
	}
	return -1;
}

int sal_check_account_online(SalOp *op, const char *from, const char *to){
	osip_message_t *options=NULL;

	sal_op_set_from(op,from);
	sal_op_set_to(op,to);
	sal_exosip_fix_route(op);

	eXosip_options_build_request (&options, sal_op_get_to(op),
		sal_op_get_from(op),sal_op_get_route(op));
	if (options){
		sal_add_other(sal_op_get_sal(op),op,options);
		return eXosip_options_send_request(options);
	}
	return -1;
}

int sal_call_refer(SalOp *h, const char *refer_to){
	osip_message_t *msg=NULL;
	int err=0;
	eXosip_lock();
	eXosip_call_build_refer(h->did,refer_to, &msg);
	if (msg)
    {
        char *ipStart = strchr(((SalOpBase *)h)->contact, '@');
        char *ipEnd = strchr(ipStart, ':');
        osip_contact_t *tmp = (osip_contact_t *)osip_list_get(&msg->contacts, 0);
        memcpy(tmp->url->host, ipStart+1, ipEnd-ipStart-1);
        tmp->url->host[ipEnd-ipStart-1] = '\0';
        char *portEnd = strchr(ipEnd, ';');
        memcpy(tmp->url->port, ipEnd+1, portEnd-ipEnd-1);
        tmp->url->port[portEnd-ipEnd-1] = '\0';
        err=eXosip_call_send_request(h->did, msg);
    }
	else err=-1;
	eXosip_unlock();
	return err;
}

int sal_call_refer_with_replaces(SalOp *h, SalOp *other_call_h){
	osip_message_t *msg=NULL;
	char referto[256]={0};
	int err=0;
	eXosip_lock();
	if (eXosip_call_get_referto(other_call_h->did,referto,sizeof(referto)-1)!=0){
		PrintConsole("sal_eXosip2 eXosip_call_get_referto() failed for did=%i\n",other_call_h->did);
		eXosip_unlock();
		return -1;
	}
	eXosip_call_build_refer(h->did,referto, &msg);
	osip_message_set_header(msg,"Referred-By",h->base.from);
	if (msg) err=eXosip_call_send_request(h->did, msg);
	else err=-1;
	eXosip_unlock();
	return err;
}

SalOp *sal_call_get_replaces(SalOp *h){
	if (h!=NULL && h->replaces!=NULL){
		int cid;
		eXosip_lock();
		cid=eXosip_call_find_by_replaces(h->replaces);
		eXosip_unlock();
		if (cid>0){
			SalOp *ret=sal_find_call(h->base.root,cid);
			return ret;
		}
	}
	return NULL;
}

int sal_call_send_dtmf(SalOp *h, char dtmf){
	osip_message_t *msg=NULL;
	char dtmf_body[128];
	char clen[10];

	eXosip_lock();
	eXosip_call_build_info(h->did,&msg);
	if (msg){
		snprintf(dtmf_body, sizeof(dtmf_body), "Signal=%c\r\nDuration=250\r\n", dtmf);
		osip_message_set_body(msg,dtmf_body,strlen(dtmf_body));
		osip_message_set_content_type(msg,"application/dtmf-relay");
		snprintf(clen,sizeof(clen),"%lu",(unsigned long)strlen(dtmf_body));
		osip_message_set_content_length(msg,clen);		
		eXosip_call_send_request(h->did,msg);
	}
	eXosip_unlock();
	return 0;
}

static void push_auth_to_exosip(const SalAuthInfo *info){
	const char *userid;
	if (info->userid==NULL || info->userid[0]=='\0') userid=info->username;
	else userid=info->userid;
	PrintConsole("sal_eXosip2 Authentication info for username [%s], id[%s], realm [%s] added to eXosip", info->username,userid, info->realm);
	eXosip_add_authentication_info (info->username,userid,
                                  info->password, NULL,info->realm);
}
/*
 * Just for symmetry ;-)
 */
static void pop_auth_from_exosip() {
	eXosip_clear_authentication_info();
}

int sal_call_terminate(SalOp *h){
	int err;
	if (h == NULL) return -1;
	if (h->auth_info) push_auth_to_exosip(h->auth_info);
	eXosip_lock();
	err=eXosip_call_terminate(h->cid,h->did);
	eXosip_unlock();
	if (!h->base.root->reuse_authorization) pop_auth_from_exosip();
	if (err!=0){
		PrintConsole("sal_eXosip2 Exosip could not terminate the call: cid=%i did=%i\n", h->cid,h->did);
	}
	h->terminated=TRUE;
	return 0;
}

void sal_op_authenticate(SalOp *h, const SalAuthInfo *info){
       bool_t terminating=FALSE;
       if (h->pending_auth && strcmp(h->pending_auth->request->sip_method,"BYE")==0) {
               terminating=TRUE;
       }
       if (h->terminated && !terminating) return;

       if (h->pending_auth){
		push_auth_to_exosip(info);
		
        /*FIXME exosip does not take into account this update register message*/
	/*
        if (fix_message_contact(h, h->pending_auth->request,h->pending_auth->response)) {
            
        };
	*/
		update_contact_from_response(h,h->pending_auth->response);
		eXosip_lock();
		eXosip_default_action(h->pending_auth);
		eXosip_unlock();
		PrintConsole("sal_eXosip2 eXosip_default_action() done");
		if (!h->base.root->reuse_authorization) pop_auth_from_exosip();
		
		if (h->auth_info) sal_auth_info_delete(h->auth_info); /*if already exist*/
		h->auth_info=sal_auth_info_clone(info); /*store auth info for subsequent request*/
	}
}
void sal_op_cancel_authentication(SalOp *h) {
	if (h->rid >0) {
		sal_op_get_sal(h)->callbacks.register_failure(h,SalErrorFailure, SalReasonForbidden,"Authentication failure");
	} else if (h->cid >0) {
		sal_op_get_sal(h)->callbacks.call_failure(h,SalErrorFailure, SalReasonForbidden,"Authentication failure",0);
	} else {
		PrintConsole("sal_eXosip2 Auth failure not handled");
	}

}
static void set_network_origin(SalOp *op, osip_message_t *req){
	const char *received=NULL;
	int rport=5060;
	char origin[64]={0};
    SalTransport transport;
	if (extract_received_rport(req,&received,&rport,&transport)!=0){
		osip_via_t *via=NULL;
		char *tmp;
		osip_message_get_via(req,0,&via);
		received=osip_via_get_host(via);
		tmp=osip_via_get_port(via);
		if (tmp) rport=atoi(tmp);
	}
    if (transport != SalTransportUDP) {
        snprintf(origin,sizeof(origin)-1,"sip:%s:%i",received,rport);
    } else {
       snprintf(origin,sizeof(origin)-1,"sip:%s:%i;transport=%s",received,rport,sal_transport_to_string(transport)); 
    }
	__sal_op_set_network_origin(op,origin);
}

static void set_remote_ua(SalOp* op, osip_message_t *req){
	if (op->base.remote_ua==NULL){
		osip_header_t *h=NULL;
		osip_message_get_user_agent(req,0,&h);
		if (h){
			op->base.remote_ua=ms_strdup(h->hvalue);
		}
	}
}

static void set_akey_bkey(SalOp *op, osip_message_t *req, int isAkey)
{
//haiyuntong
#ifdef HAIYUNTONG
    osip_header_t *h = NULL;
        if (1 == isAkey) {
            osip_message_header_get_byname(( osip_message_t *)req,(const char *)"P-cipher-AK",0,&h);
            if (h) {
                PrintConsole("sal_eXosip2 [DEBUG HAIYUNTONG] set akey:%s\n",h->hvalue);
                op->akey = ms_strdup(h->hvalue);
            }

        }
        else if (0 == isAkey) {
            osip_message_header_get_byname(( osip_message_t *)req,(const char *)"P-cipher-BK",0,&h);
            if (h) {
                PrintConsole("sal_eXosip2 [DEBUG HAIYUNTONG] set bkey:%s\n",h->hvalue);
                op->bkey = ms_strdup(h->hvalue);
            }
            
        }
    
#endif
}

static void set_replaces(SalOp *op, osip_message_t *req){
	osip_header_t *h=NULL;

	if (op->replaces){
		ms_free((void **)&op->replaces);
	}
	osip_message_header_get_byname(req,"replaces",0,&h);
	if (h){
		if (h->hvalue && h->hvalue[0]!='\0'){
			op->replaces=ms_strdup(h->hvalue);
		}
	}
}

static SalOp *find_op(Sal *sal, eXosip_event_t *ev){
	if (ev->cid>0){
		return sal_find_call(sal,ev->cid);
	}
	if (ev->rid>0){
		return sal_find_register(sal,ev->rid);
	}
	if (ev->sid>0){
		return sal_find_out_subscribe(sal,ev->sid);
	}
	if (ev->nid>0){
		return sal_find_in_subscribe(sal,ev->nid);
	}
	if (ev->response) return sal_find_other(sal,ev->response);
	else if (ev->request) return sal_find_other(sal,ev->request);
	return NULL;
}

static void inc_new_call(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_op_new(sal);
	osip_from_t *from,*to;
	osip_call_info_t *call_info;
	char *tmp;
	sdp_message_t *sdp=eXosip_get_sdp_info(ev->request);
	osip_call_id_t *callid=osip_message_get_call_id(ev->request);
	osip_call_id_to_str(callid,(char**)(&op->base.call_id));

	set_network_origin(op,ev->request);
	set_remote_ua(op,ev->request);
	set_replaces(op,ev->request);
	
	if (sdp){
		op->sdp_offering=FALSE;
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_message_free(sdp);
	}else op->sdp_offering=TRUE;

	from=osip_message_get_from(ev->request);
	to=osip_message_get_to(ev->request);
	osip_from_to_str(from,&tmp);
	sal_op_set_from(op,tmp);
	osip_free(tmp);
	osip_from_to_str(to,&tmp);
	sal_op_set_to(op,tmp);
	osip_free(tmp);
	
	osip_header_t *h=NULL;
	osip_message_header_get_byname(ev->request,"P-Call-UserData",0,&h);
	if( h) {
		PrintConsole("sal_eXosip2 P-Call-UserData is %s\n",h->hvalue );
		op->invite_userdata = ms_strdup(h->hvalue);
	}
//haiyuntong
#ifdef HAIYUNTONG
    osip_message_header_get_byname(ev->request,"P-cipher-AK",0,&h);
    if( h) {
        PrintConsole("sal_eXosip2 [DEBUG HAIYUNTONG] inc_new_call cipher:AK:%s\n",h->hvalue );
        op->akey = ms_strdup(h->hvalue);
    }
#endif
	osip_message_get_call_info(ev->request,0,&call_info);
	if(call_info)
	{
		osip_call_info_to_str(call_info,&tmp);
		if( strstr(tmp,"answer-after=") != NULL)
		{
			op->auto_answer_asked=TRUE;
			PrintConsole("sal_eXosip2 The caller asked to automatically answer the call(Emergency?)\n");
		}
		osip_free(tmp);
	}

	op->tid=ev->tid;
	op->cid=ev->cid;
	op->did=ev->did;
	sal_add_call(op->base.root,op);
	sal->callbacks.call_received(op);
}

static void handle_reinvite(Sal *sal,  eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	sdp_message_t *sdp;

	if (op==NULL) {
		PrintConsole("sal_eXosip2 Reinvite for non-existing operation !\n");
		return;
	}
	op->reinvite=TRUE;
	op->tid=ev->tid;
	sdp=eXosip_get_sdp_info(ev->request);
	if (op->base.remote_media){
		sal_media_description_unref(&op->base.remote_media);
        op->base.remote_media=NULL;
	}
	if (op->result){
		sal_media_description_unref(&op->result);
        op->result = NULL;
	}
	if (sdp){
		op->sdp_offering=FALSE;
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media,1);
		sdp_message_free(sdp);
		
	}else {
		op->sdp_offering=TRUE;
	}
	sal->callbacks.call_updating(op);
}

static void handle_ack(Sal *sal,  eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	sdp_message_t *sdp;

	if (op==NULL) {
		PrintConsole("sal_eXosip2 ack for non-existing call !\n");
		return;
	}
	if (op->terminated) {
		PrintConsole("sal_eXosip2 ack for terminated call, ignoring\n");
		return;
	}
	
	if (op->sdp_offering){
		sdp=eXosip_get_sdp_info(ev->ack);
		if (sdp){
			if (op->base.remote_media)
				sal_media_description_unref(&op->base.remote_media);
			op->base.remote_media=sal_media_description_new();
			sdp_to_media_description(sdp,op->base.remote_media);
			sdp_process(op);
			sdp_message_free(sdp);
		}
	}
	if (op->reinvite){
		op->reinvite=FALSE;
	}
	sal->callbacks.call_ack(op);
}

static void update_contact_from_response(SalOp *op, osip_message_t *response){
	const char *received;
	int rport;
	SalTransport transport;
	if (extract_received_rport(response,&received,&rport,&transport)==0){
		const char *contact=sal_op_get_contact(op);
		if (!contact){
			/*no contact given yet, use from instead*/
			contact=sal_op_get_from(op);
		}
		if (contact){
			SalAddress *addr=sal_address_new(contact);
			char *tmp;
			sal_address_set_domain(addr,received);
			sal_address_set_port_int(addr,rport);
			if (transport!=SalTransportUDP)
				sal_address_set_transport(addr,transport);
			tmp=sal_address_as_string(addr);
			PrintConsole("sal_eXosip2 Contact address updated to %s\n",tmp);
			sal_op_set_contact(op,tmp);
			sal_address_destroy(addr);
			ms_free((void **)&tmp);
		}
	}
}

static int call_proceeding(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);

	if (op==NULL || op->terminated==TRUE) {
		PrintConsole("sal_eXosip2 This call has been canceled.\n");
		eXosip_lock();
		eXosip_call_terminate(ev->cid,ev->did);
		eXosip_unlock();
		return -1;
	}
	if (ev->did>0)
		op->did=ev->did;
	op->tid=ev->tid;
	
	/* update contact if received and rport are set by the server
	 note: will only be used by remote for next INVITE, if any...*/
	update_contact_from_response(op,ev->response);

	sal->callbacks.call_proceeding(op);
	return 0;
}

static void call_ringing(Sal *sal, eXosip_event_t *ev){
	sdp_message_t *sdp;
	SalOp *op=find_op(sal,ev);
	if (call_proceeding(sal, ev)==-1) return;

	set_remote_ua(op,ev->response);
//haiyuntong
#ifdef HAIYUNTONG
    set_akey_bkey(op,ev->response, 0);
#endif
	sdp=eXosip_get_sdp_info(ev->response);
	if (sdp){
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		sdp_message_free(sdp);
		if (op->base.local_media)
        {
            sdp_process(op);
        }
	}
	sal->callbacks.call_ringing(op);
}

static void call_accepted(Sal *sal, eXosip_event_t *ev){
	sdp_message_t *sdp;
	osip_message_t *msg=NULL;
	SalOp *op=find_op(sal,ev);
	const char *contact;
	
	if (op==NULL || op->terminated==TRUE) {
		PrintConsole("sal_eXosip2 This call has been already terminated.\n");
		eXosip_lock();
		eXosip_call_terminate(ev->cid,ev->did);
		eXosip_unlock();
		return ;
	}

	op->did=ev->did;
	set_remote_ua(op,ev->response);

	sdp=eXosip_get_sdp_info(ev->response);
    
#ifdef HAIYUNTONG
    if (!strncmp(ev->response->to->url->username, "conf", 4) && 200 == ev->response->status_code && !strcmp(ev->response->cseq->method,"INVITE")) {
        osip_header_t *hh = NULL;
        PrintConsole("sal_eXosip2 [DEBUG HAIYUNTONG] in conf\n");
        osip_message_header_get_byname(ev->response,(const char *)"P-cipher-K",0,&hh);
        if (hh) {
            op->confKey = ms_strdup(hh->hvalue);
        }
        
        ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal);
        
        if (NULL == op->confKey && lc->serphone_haiyuntong_enabled())
        {
            PrintConsole("sal_eXosip2 [WARNING HAIYUNTONG] in conf but no key\n");
            //in encrypt mode, end it when no key.
            eXosip_lock();
            eXosip_call_terminate(ev->cid,ev->did);
            eXosip_unlock();
            return ;
        }
    }
#endif
    
	osip_header_t *h=NULL;
	osip_message_header_get_byname(ev->response,"x-ccp-time",0,&h);
	if( h) {
		PrintConsole("sal_eXosip2 x-ccp-time is %s\n",h->hvalue );
		op->remainTime = atoi(h->hvalue);
	}
	if (sdp){
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
//        print_sdp(sdp);
		sdp_message_free(sdp);
		if (op->base.local_media)
        {
            sdp_process(op);
        }
	}
	eXosip_call_build_ack(ev->did,&msg);
	if (msg==NULL) {
		PrintConsole("sal_eXosip2 This call has been already terminated.\n");
		eXosip_lock();
		eXosip_call_terminate(ev->cid,ev->did);
		eXosip_unlock();
		return ;
	}
	contact=sal_op_get_contact(op);
	if (contact) {
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,contact);
	}
	if (op->sdp_answer){
		set_sdp(msg,op->sdp_answer);
		sdp_message_free(op->sdp_answer);
		op->sdp_answer=NULL;
	}
    
	eXosip_call_send_ack(ev->did,msg);
	sal->callbacks.call_accepted(op);
}

static void call_terminated(Sal *sal, eXosip_event_t *ev){
	char *from=NULL;
	SalOp *op=find_op(sal,ev);
	if (op==NULL){
		PrintConsole("sal_eXosip2 Call terminated for already closed call ?\n");
		return;
	}
	if (ev->request){
		osip_from_to_str(ev->request->from,&from);
	}
	sal->callbacks.call_terminated(op,from!=NULL ? from : sal_op_get_from(op));
	if (from) osip_free(from);
	op->terminated=TRUE;
}

static void call_released(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	if (op==NULL){
		PrintConsole("sal_eXosip2 No op associated to this call_released()\n");
		return;
	}
	if (!op->terminated){
		/* no response received so far */
		call_failure(sal,ev);
	}
	sal->callbacks.call_released(op);
}

static int get_auth_data_from_response(osip_message_t *resp, const char **realm, const char **username){
	const char *prx_realm=NULL,*www_realm=NULL;
	osip_proxy_authenticate_t *prx_auth;
	osip_www_authenticate_t *www_auth;
	
	*username=osip_uri_get_username(resp->from->url);
	prx_auth=(osip_proxy_authenticate_t*)osip_list_get(&resp->proxy_authenticates,0);
	www_auth=(osip_proxy_authenticate_t*)osip_list_get(&resp->www_authenticates,0);
	if (prx_auth!=NULL)
		prx_realm=osip_proxy_authenticate_get_realm(prx_auth);
	if (www_auth!=NULL)
		www_realm=osip_www_authenticate_get_realm(www_auth);

	if (prx_realm){
		*realm=prx_realm;
	}else if (www_realm){
		*realm=www_realm;
	}else{
		return -1;
	}
	return 0;
}

static int get_auth_data_from_request(osip_message_t *msg, const char **realm, const char **username){
	osip_authorization_t *auth=NULL;
	osip_proxy_authorization_t *prx_auth=NULL;
	
	*username=osip_uri_get_username(msg->from->url);
	osip_message_get_authorization(msg, 0, &auth);
	if (auth){
		*realm=osip_authorization_get_realm(auth);
		return 0;
	}
	osip_message_get_proxy_authorization(msg,0,&prx_auth);
	if (prx_auth){
		*realm=osip_proxy_authorization_get_realm(prx_auth);
		return 0;
	}
	return -1;
}

static int get_auth_data(eXosip_event_t *ev, const char **realm, const char **username){
	if (ev->response && get_auth_data_from_response(ev->response,realm,username)==0) return 0;
	if (ev->request && get_auth_data_from_request(ev->request,realm,username)==0) return 0;
	return -1;
}

int sal_op_get_auth_requested(SalOp *op, const char **realm, const char **username){
	if (op->pending_auth){
		return get_auth_data(op->pending_auth,realm,username);
	}
	return -1;
}

static bool_t process_authentication(Sal *sal, eXosip_event_t *ev){
	SalOp *op;
	const char *username,*realm;
	op=find_op(sal,ev);
	if (op==NULL){
		PrintConsole("sal_eXosip2 No operation associated with this authentication !\n");
		return TRUE;
	}
	if (get_auth_data(ev,&realm,&username)==0){
		if (op->pending_auth!=NULL){
			eXosip_event_free(op->pending_auth);
			op->pending_auth=ev;
		}else{
			op->pending_auth=ev;
			sal_add_pending_auth(sal,op);
		}
		
		sal->callbacks.auth_requested(op,realm,username);
		return FALSE;
	}
	return TRUE;
}

static void authentication_ok(Sal *sal, eXosip_event_t *ev){
	SalOp *op;
	const char *username,*realm;
	op=find_op(sal,ev);
	if (op==NULL){
		PrintConsole("sal_eXosip2 No operation associated with this authentication_ok!\n");
		return ;
	}
	if (op->pending_auth){
		eXosip_event_free(op->pending_auth);
		sal_remove_pending_auth(sal,op);
		op->pending_auth=NULL;
	}
	if (get_auth_data(ev,&realm,&username)==0){
		sal->callbacks.auth_success(op,realm,username);
	}
}

static bool_t call_failure(Sal *sal, eXosip_event_t *ev){
	SalOp *op;
	int code=0;
	char* computedReason=NULL;
	const char *reason=NULL;
	SalError error=SalErrorUnknown;
	SalReason sr=SalReasonUnknown;
	

	op=(SalOp*)find_op(sal,ev);

	if (op==NULL) {
		PrintConsole("sal_eXosip2 Call failure reported for a closed call, ignored.\n");
		return TRUE;
	}

	if (ev->response){
		code=osip_message_get_status_code(ev->response);
		reason=osip_message_get_reason_phrase(ev->response);
		osip_header_t *h=NULL;
		if (osip_message_header_get_byname(ev->response
											,"reason"
											,0
											,&h) >= 0) {
			computedReason = ms_strdup_printf("%s %s",reason,osip_header_get_value(h));
			reason = computedReason;

		}
	}
	switch(code)
	{
		case 401:
		case 407:
			return process_authentication(sal,ev);
			break;
		case 400:
			error=SalErrorUnknown;
		break;
		case 404:
			error=SalErrorFailure;
			sr=SalReasonNotFound;
		break;
		case 415:
			error=SalErrorFailure;
			sr=SalReasonMedia;
		break;
		case 422:
			eXosip_default_action(ev);
			return TRUE;
		break;
		case 430:
			error=SalErrorFailure;
			sr=SalReasonNoVoip;
		break;
		case 480:
			error=SalErrorFailure;
			sr=SalReasonTemporarilyUnavailable;
			break;
		case 486:
			error=SalErrorFailure;
			sr=SalReasonBusy;
		break;
		case 487:
		break;
		case 600:
			error=SalErrorFailure;
			sr=SalReasonDoNotDisturb;
		break;
		case 603:
			error=SalErrorFailure;
			sr=SalReasonDeclined;
		break;
		default:
			if (code>0){
				error=SalErrorFailure;
				sr=SalReasonUnknown;
			}else error=SalErrorNoResponse;
	}
	op->terminated=TRUE;
	sal->callbacks.call_failure(op,error,sr,reason,code);
	if (computedReason != NULL){
		ms_free((void **)&computedReason);
	}
	return TRUE;
}

/* Request remote side to send us VFU */
void sal_call_send_vfu_request(SalOp *h){
	osip_message_t *msg=NULL;
	char info_body[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
			 "<media_control>"
			 "  <vc_primitive>"
			 "    <to_encoder>"
			 "      <picture_fast_update></picture_fast_update>"
			 "    </to_encoder>"
			 "  </vc_primitive>"
			 "</media_control>";

	char clen[10];

	eXosip_lock();
	eXosip_call_build_info(h->did,&msg);
	if (msg){
		osip_message_set_body(msg,info_body,strlen(info_body));
		osip_message_set_content_type(msg,"application/media_control+xml");
		snprintf(clen,sizeof(clen),"%lu",(unsigned long)strlen(info_body));
		osip_message_set_content_length(msg,clen);
		eXosip_call_send_request(h->did,msg);
		PrintConsole("sal_eXosip2 Sending VFU request !\n");
	}
	eXosip_unlock();
}

static void process_media_control_xml(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	osip_body_t *body=NULL;

	if (op==NULL){
		PrintConsole("sal_eXosip2 media control xml received without operation context!\n");
		return ;
	}
	
	osip_message_get_body(ev->request,0,&body);
	if (body && body->body!=NULL &&
		strstr(body->body,"picture_fast_update")){
		osip_message_t *ans=NULL;
		PrintConsole("sal_eXosip2 Receiving VFU request !\n");
		if (sal->callbacks.vfu_request){
			sal->callbacks.vfu_request(op);
			eXosip_call_build_answer(ev->tid,200,&ans);
			if (ans)
				eXosip_call_send_answer(ev->tid,200,ans, NULL, 0);
			return;
		}
	}
	/*in all other cases we must say it is not implemented.*/
	{
		osip_message_t *ans=NULL;
		eXosip_lock();
		eXosip_call_build_answer(ev->tid,501,&ans);
		if (ans)
			eXosip_call_send_answer(ev->tid,501,ans, NULL, 0);
		eXosip_unlock();
	}
}

static void process_dtmf_relay(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	osip_body_t *body=NULL;

	if (op==NULL){
		PrintConsole("sal_eXosip2 media dtmf relay received without operation context!\n");
		return ;
	}
	
	osip_message_get_body(ev->request,0,&body);
	if (body && body->body!=NULL){
		osip_message_t *ans=NULL;
		const char *name=strstr(body->body,"Signal");
		if (name==NULL) name=strstr(body->body,"signal");
		if (name==NULL) {
			PrintConsole("sal_eXosip2 Could not extract the dtmf name from the SIP INFO.\n");
		}else{
			char tmp[2];
			name+=strlen("signal");
			if (sscanf(name," = %1s",tmp)==1){
				PrintConsole("sal_eXosip2 Receiving dtmf %s via SIP INFO.\n",tmp);
				if (sal->callbacks.dtmf_received != NULL)
					sal->callbacks.dtmf_received(op, tmp[0]);
			}
		}
		eXosip_lock();
		eXosip_call_build_answer(ev->tid,200,&ans);
		if (ans)
			eXosip_call_send_answer(ev->tid,200,ans, NULL, 0);
		eXosip_unlock();
	}
}

static void fill_options_answer(osip_message_t *options){
	osip_message_set_allow(options,"INVITE, ACK, BYE, CANCEL, OPTIONS, MESSAGE, SUBSCRIBE, NOTIFY, INFO");
	osip_message_set_accept(options,"application/sdp");
}

static void process_refer(Sal *sal, SalOp *op, eXosip_event_t *ev){
	osip_header_t *h=NULL;
	osip_message_t *ans=NULL;
	PrintConsole("sal_eXosip2 Receiving REFER request !\n");
	osip_message_header_get_byname(ev->request,"Refer-To",0,&h);

	if (h){
		osip_from_t *from=NULL;
		char *tmp;
		osip_from_init(&from);
	
		if (osip_from_parse(from,h->hvalue)==0){
			if (op ){
				osip_uri_header_t *uh=NULL;
				osip_header_t *referred_by=NULL;
				osip_uri_header_get_byname(&from->url->url_headers,(char*)"Replaces",&uh);
				if (uh!=NULL && uh->gvalue && uh->gvalue[0]!='\0'){
					PrintConsole("sal_eXosip2 Found replaces in Refer-To");
					if (op->replaces){
						ms_free((void **)&op->replaces);
					}
					op->replaces=ms_strdup(uh->gvalue);
				}
				osip_message_header_get_byname(ev->request,"Referred-By",0,&referred_by);
				if (referred_by && referred_by->hvalue && referred_by->hvalue[0]!='\0'){
					if (op->referred_by)
                    {
                        ms_free((void **)&op->referred_by);
                    }
					op->referred_by=ms_strdup(referred_by->hvalue);
				}
			}
			osip_uri_header_freelist(&from->url->url_headers);
			osip_from_to_str(from,&tmp);
			sal->callbacks.refer_received(sal,op,tmp);
			osip_free(tmp);
			osip_from_free(from);
		}
		eXosip_lock();
		eXosip_call_build_answer(ev->tid,202,&ans);
		if (ans)
			eXosip_call_send_answer(ev->tid,202,ans, NULL, 0);
		eXosip_unlock();
	}
	else
	{
		PrintConsole("sal_eXosip2 cannot do anything with the refer without destination\n");
	}
}

static void process_notify(Sal *sal, eXosip_event_t *ev){
	osip_header_t *h=NULL;
	char *from=NULL;
	SalOp *op=find_op(sal,ev);
	osip_message_t *ans=NULL;

	PrintConsole("sal_eXosip2 Receiving NOTIFY request !\n");
	osip_from_to_str(ev->request->from,&from);
	osip_message_header_get_byname(ev->request,"Event",0,&h);
	if(h){
		osip_body_t *body=NULL;
		//osip_content_type_t *ct=NULL;
		osip_message_get_body(ev->request,0,&body);
		//ct=osip_message_get_content_type(ev->request);
		if (h->hvalue && strncasecmp(h->hvalue,"refer",5)==0){
			/*special handling of refer events*/
			if (body && body->body){
				osip_message_t *msg;
				osip_message_init(&msg);
				if (osip_message_parse_sipfrag(msg,body->body,strlen(body->body))==0){
					int code=osip_message_get_status_code(msg);
					if (code==100){
						sal->callbacks.notify_refer(op,SalReferTrying);
					}else if (code==200){
						sal->callbacks.notify_refer(op,SalReferSuccess);
					}else if (code>=400){
						sal->callbacks.notify_refer(op,SalReferFailed);
					}
				}
				osip_message_free(msg);
			}
		}else{
			/*generic handling*/
			sal->callbacks.notify(op,from,h->hvalue);
		}
	}
	/*answer that we received the notify*/
	eXosip_lock();
	eXosip_call_build_answer(ev->tid,200,&ans);
	if (ans)
    {
        char *ipStart = strchr(((SalOpBase *)op)->contact, '@');
        char *ipEnd = strchr(ipStart, ':');
        osip_contact_t *tmp = (osip_contact_t *)osip_list_get(&ans->contacts, 0);
        memcpy(tmp->url->host, ipStart+1, ipEnd-ipStart-1);
        tmp->url->host[ipEnd-ipStart-1] = '\0';
        char *portEnd = strchr(ipEnd, ';');
        memcpy(tmp->url->port, ipEnd+1, portEnd-ipEnd-1);
        tmp->url->port[portEnd-ipEnd-1] = '\0';
        
		eXosip_call_send_answer(ev->tid,200,ans, NULL, 0);
    }
	eXosip_unlock();
	osip_free(from);
}

static void call_message_new(Sal *sal, eXosip_event_t *ev){
	osip_message_t *ans=NULL;
	if (ev->request){
		if (MSG_IS_INFO(ev->request)){
			osip_content_type_t *ct;
			ct=osip_message_get_content_type(ev->request);
			if (ct && ct->subtype){
				if (strcmp(ct->subtype,"media_control+xml")==0)
					process_media_control_xml(sal,ev);
				else if (strcmp(ct->subtype,"dtmf-relay")==0)
					process_dtmf_relay(sal,ev);
				else {
					PrintConsole("sal_eXosip2 Unhandled SIP INFO.\n");
					/*send an "Not implemented" answer*/
					eXosip_lock();
					eXosip_call_build_answer(ev->tid,501,&ans);
					if (ans)
						eXosip_call_send_answer(ev->tid,501,ans, NULL, 0);
					eXosip_unlock();
				}
			}else{
				/*empty SIP INFO, probably to test we are alive. Send an empty answer*/
				eXosip_lock();
				eXosip_call_build_answer(ev->tid,200,&ans);
				if (ans)
					eXosip_call_send_answer(ev->tid,200,ans, NULL, 0);
				eXosip_unlock();
			}
		}else if(MSG_IS_MESSAGE(ev->request)){
			/* SIP messages could be received into call */
            eXosip_lock();
            eXosip_call_build_answer(ev->tid,200,&ans);
            if (ans)
                eXosip_call_send_answer(ev->tid,200,ans, NULL, 0);
            eXosip_unlock();
			text_received(sal, ev);
			
		}else if(MSG_IS_REFER(ev->request)){
			SalOp *op=find_op(sal,ev);
			
			PrintConsole("sal_eXosip2 Receiving REFER request !\n");
			process_refer(sal,op,ev);
		}else if(MSG_IS_NOTIFY(ev->request)){
			process_notify(sal,ev);
		}else if (MSG_IS_OPTIONS(ev->request)){
			eXosip_lock();
			eXosip_call_build_answer(ev->tid,200,&ans);
			if (ans){
				fill_options_answer(ans);
				eXosip_call_send_answer(ev->tid,200,ans, NULL, 0);
			}
			eXosip_unlock();
		}
	}else PrintConsole("sal_eXosip2 call_message_new: No request ?\n");
}

static void inc_update(Sal *sal, eXosip_event_t *ev){
	osip_message_t *msg=NULL;
	PrintConsole("sal_eXosip2 Processing incoming UPDATE");
	eXosip_lock();
	eXosip_message_build_answer(ev->tid,200,&msg);
	if (msg!=NULL)
		eXosip_message_send_answer(ev->tid,200,msg);
	eXosip_unlock();
}

static bool_t comes_from_local_if(osip_message_t *msg){
	osip_via_t *via=NULL;
	osip_message_get_via(msg,0,&via);
	if (via){
		const char *host;
		host=osip_via_get_host(via);
		if (strcmp(host,"127.0.0.1")==0 || strcmp(host,"::1")==0){
			osip_generic_param_t *param=NULL;
			osip_via_param_get_byname(via,"received",&param);
			if (param==NULL) return TRUE;
			if (param->gvalue &&
				(strcmp(param->gvalue,"127.0.0.1")==0 || strcmp(param->gvalue,"::1")==0)){
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void text_received(Sal *sal, eXosip_event_t *ev){
//	osip_body_t *body=NULL;
//	char *from=NULL,*msg=NULL;
//	osip_content_type_t* content_type;
//	osip_uri_param_t* external_body_url; 
//	char unquoted_external_body_url [256];
//	int external_body_size=0;
//	SalMessage salmsg;
//	char message_id[256]={0};
//	
//	content_type= osip_message_get_content_type(ev->request);
//	if (!content_type) {
//		PrintConsole("sal_eXosip2 Could not get message because no content type");
//		return;
//	}
//	osip_from_to_str(ev->request->from,&from);
//	if (content_type->type 
//		&& strcmp(content_type->type, "text")==0 
//		&& content_type->subtype
//		&& strcmp(content_type->subtype, "plain")==0 ) {
//		osip_message_get_body(ev->request,0,&body);
//		if (body==NULL){
//			PrintConsole("sal_eXosip2 Could not get text message from SIP body");
//			osip_free(from);
//			return;
//		}
//		msg=body->body;
//	}else if (content_type->type 
//		  && strcmp(content_type->type, "message")==0 
//		  && content_type->subtype
//		  && strcmp(content_type->subtype, "external-body")==0 ) {
//		
//		osip_content_type_param_get_byname(content_type, "URL", &external_body_url);
//		/*remove both first and last character*/
//		strncpy(unquoted_external_body_url
//				,&external_body_url->gvalue[1]
//				,external_body_size=MIN(strlen(external_body_url->gvalue)-1,sizeof(unquoted_external_body_url)));
//		unquoted_external_body_url[external_body_size-1]='\0';
//	} else {
//		PrintConsole("sal_eXosip2 Unsupported content type [%s/%s]\n",content_type->type,content_type->subtype);
//		osip_free(from);
//		return;
//	}
//	snprintf(message_id,sizeof(message_id)-1,"%s%s",ev->request->call_id->number,ev->request->cseq->number);
//	
//	salmsg.from=from;
//	salmsg.text=msg;
//	salmsg.url=external_body_size>0 ? unquoted_external_body_url : NULL;
//	salmsg.message_id=message_id;
//	sal->callbacks.text_received(sal,&salmsg);
//	osip_free(from);
    osip_body_t *body=NULL;
	char *from=NULL, *to=NULL, *msg;
	osip_header_t *date = NULL;
    osip_message_get_date(ev->request,0, &date);
	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		PrintConsole("sal_eXosip2 Could not get text message from SIP body\n");
		return;
	}
	msg=body->body;
	osip_from_to_str(ev->request->from,&from);
    osip_to_to_str(ev->request->to, &to);
    if (date) {
        sal->callbacks.text_received(sal,from,to, ev->request->call_id->number,msg, date->hvalue);
    }
    else
    {
        sal->callbacks.text_received(sal,from,to, ev->request->call_id->number,msg,NULL);
    }
	
	osip_free(from);
}



static void other_request(Sal *sal, eXosip_event_t *ev){
	PrintConsole("sal_eXosip2 in other_request");
	if (ev->request==NULL) return;
	if (strcmp(ev->request->sip_method,"MESSAGE")==0){
		text_received(sal,ev);
		eXosip_message_send_answer(ev->tid,200,NULL);
	}else if (strcmp(ev->request->sip_method,"OPTIONS")==0){
		osip_message_t *options=NULL;
		eXosip_options_build_answer(ev->tid,200,&options);
		fill_options_answer(options);
		eXosip_options_send_answer(ev->tid,200,options);
	}else if (strncmp(ev->request->sip_method, "REFER", 5) == 0){
		PrintConsole("sal_eXosip2 Receiving REFER request !\n");
		if (comes_from_local_if(ev->request)) {
			process_refer(sal,NULL,ev);
		}else PrintConsole("sal_eXosip2 Ignored REFER not coming from this local loopback interface.\n");
	}else if (strncmp(ev->request->sip_method, "UPDATE", 6) == 0){
		inc_update(sal,ev);
	}else {
		char *tmp=NULL;
		size_t msglen=0;
		osip_message_to_str(ev->request,&tmp,&msglen);
		if (tmp){
			PrintConsole("sal_eXosip2 Unsupported request received:\n%s\n",tmp);
			osip_free(tmp);
		}
		/*answer with a 501 Not implemented*/
		eXosip_message_send_answer(ev->tid,501,NULL);
	}
}

static void masquerade_via(osip_message_t *msg, const char *ip, const char *port){
	osip_via_t *via=NULL;
	osip_message_get_via(msg,0,&via);
	if (via){
		osip_free(via->port);
		via->port=osip_strdup(port);
		osip_free(via->host);
		via->host=osip_strdup(ip);
	}
}


static bool_t fix_message_contact(SalOp *op, osip_message_t *request,osip_message_t *last_answer, bool_t expire_last_contact) {
	osip_contact_t *ctt=NULL;
	const char *received;
	int rport;
	SalTransport transport;
	char port[20];

	if (extract_received_rport(last_answer,&received,&rport,&transport)==-1) return FALSE;
	osip_message_get_contact(request,0,&ctt);
	if (ctt == NULL) {
		PrintConsole("sal_eXosip2 fix_message_contact(): no contact to update");
		return FALSE;
	}
	if (expire_last_contact){
		osip_contact_t *oldct=NULL,*prevct;
		osip_generic_param_t *param=NULL;
		osip_contact_clone(ctt,&oldct);
		while ((prevct=(osip_contact_t*)osip_list_get(&request->contacts,1))!=NULL){
			osip_contact_free(prevct);
			osip_list_remove(&request->contacts,1);
		}
		osip_list_add(&request->contacts,oldct,1);
		osip_contact_param_get_byname(oldct,"expires",&param);
		if (param){
			if (param->gvalue) osip_free(param->gvalue);
			param->gvalue=osip_strdup("0");
		}else{
			osip_contact_param_add(oldct,osip_strdup("expires"),osip_strdup("0"));
		}
	}
	if (ctt->url->host!=NULL){
		osip_free(ctt->url->host);
	}
	ctt->url->host=osip_strdup(received);
	if (ctt->url->port!=NULL){
		osip_free(ctt->url->port);
	}
	snprintf(port,sizeof(port),"%i",rport);
	ctt->url->port=osip_strdup(port);
	if (op->masquerade_via) masquerade_via(request,received,port);

	if (transport != SalTransportUDP) {
		sal_address_set_param((SalAddress *)ctt, "transport", sal_transport_to_string(transport)); 
	}
	return TRUE;    
}

static bool_t register_again_with_updated_contact(SalOp *op, osip_message_t *orig_request, osip_message_t *last_answer){
	osip_contact_t *ctt=NULL;
	SalAddress* ori_contact_address=NULL;
	const char *received;
	int rport;
	SalTransport transport;
	char* tmp;
	osip_message_t *msg=NULL;
	Sal* sal=op->base.root;
	int i=0;
	bool_t found_valid_contact=FALSE;
	bool_t from_request=FALSE;

	if (sal->double_reg==FALSE ) return FALSE; 

	if (extract_received_rport(last_answer,&received,&rport,&transport)==-1) return FALSE;
	do{
		ctt=NULL;
		osip_message_get_contact(last_answer,i,&ctt);
		if (!from_request && ctt==NULL) {
			osip_message_get_contact(orig_request,0,&ctt);
			from_request=TRUE;
		}
		if (ctt){
			osip_contact_to_str(ctt,&tmp);
			ori_contact_address = sal_address_new(tmp);
	
			/*check if contact is up to date*/
			if (strcmp(sal_address_get_domain(ori_contact_address),received) ==0 
				&& sal_address_get_port_int(ori_contact_address) == rport
			&& sal_address_get_transport(ori_contact_address) == transport) {
				if (!from_request){
					PrintConsole("sal_eXosip2 Register response has up to date contact, doing nothing.\n");
				}else {
					PrintConsole("sal_eXosip2 Register response does not have up to date contact, but last request had.\n"
						"Stupid registrar detected, giving up.\n");
				}
				found_valid_contact=TRUE;
			}
			osip_free(tmp);
			sal_address_destroy(ori_contact_address);
		}else break;
		i++;
	}while(!found_valid_contact);
	if (!found_valid_contact)
		PrintConsole("sal_eXosip2 Contact do not match, resending register.\n");
	else return FALSE;

	eXosip_lock();
	eXosip_register_build_register(op->rid,op->expires,&msg);
	if (msg==NULL){
	    eXosip_unlock();
	    PrintConsole("sal_eXosip2 Fail to create a contact updated register.\n");
	    return FALSE;
	}
	if (fix_message_contact(op,msg,last_answer,op->base.root->expire_old_contact)) {
		eXosip_register_send_register(op->rid,msg);
		eXosip_unlock();  
		PrintConsole("sal_eXosip2 Resending new register with updated contact\n");
		update_contact_from_response(op,last_answer);
		return TRUE;
	} else {
	    PrintConsole("sal_eXosip2 Fail to send updated register.\n");
	    eXosip_unlock();
	    return FALSE;
	}
	eXosip_unlock();
	return FALSE;
}

static void registration_success(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_find_register(sal,ev->rid);
	osip_header_t *h=NULL;
	bool_t registered;
	if (op==NULL){
		PrintConsole("sal_eXosip2 Receiving register response for unknown operation\n");
		return;
	}
	osip_message_get_expires(ev->request,0,&h);
	if (h!=NULL && atoi(h->hvalue)!=0){
		registered=TRUE;
		if (!register_again_with_updated_contact(op,ev->request,ev->response)){
			sal->callbacks.register_success(op,registered);
		}
	}else {
		sal->callbacks.register_success(op,FALSE);
	}
}

static bool_t registration_failure(Sal *sal, eXosip_event_t *ev){
	int status_code=0;
	const char *reason=NULL;
	SalOp *op=sal_find_register(sal,ev->rid);
	SalReason sr=SalReasonUnknown;
	SalError se=SalErrorUnknown;
	
	if (op==NULL){
		PrintConsole("sal_eXosip2 Receiving register failure for unknown operation\n");
		return TRUE;
	}
	if (ev->response){
		status_code=osip_message_get_status_code(ev->response);
		reason=osip_message_get_reason_phrase(ev->response);
	}
	switch(status_code){
		case 401:
		case 407:
			return process_authentication(sal,ev);
			break;
		case 423: /*interval too brief*/
			{/*retry with greater interval */
				osip_header_t *h=NULL;
				osip_message_t *msg=NULL;
				osip_message_header_get_byname(ev->response,"min-expires",0,&h);
				if (h && h->hvalue && h->hvalue[0]!='\0'){
					int val=atoi(h->hvalue);
					if (val>op->expires)
						op->expires=val;
				}else op->expires*=2;
				eXosip_lock();
				eXosip_register_build_register(op->rid,op->expires,&msg);
				eXosip_register_send_register(op->rid,msg);
				eXosip_unlock();
			}
		break;
		case 606: /*Not acceptable, workaround for proxies that don't like private addresses
				 in vias, such as ekiga.net 
				 On the opposite, freephonie.net bugs when via are masqueraded.
				 */
			op->masquerade_via=TRUE;
		default:
			/* if contact is up to date, process the failure, otherwise resend a new register with
				updated contact first, just in case the faillure is due to incorrect contact */
			if (ev->response && register_again_with_updated_contact(op,ev->request,ev->response))
				return TRUE; /*we are retrying with an updated contact*/
			if (status_code==403){
				se=SalErrorFailure;
				sr=SalReasonForbidden;
			}else if (status_code==0){
				se=SalErrorNoResponse;
			}
			sal->callbacks.register_failure(op,se,sr,reason);
	}
	return TRUE;
}

static void other_request_reply(Sal *sal,eXosip_event_t *ev){
//	SalOp *op=find_op(sal,ev);
//	if (op==NULL){
//		PrintConsole("sal_eXosip2 other_request_reply(): Receiving response to unknown request.\n");
//		return;
//	}
//	if (ev->response){
//		PrintConsole("sal_eXosip2 Processing reponse status [%i] for method [%s]\n",ev->response->status_code,osip_message_get_method(ev->request));
//		update_contact_from_response(op,ev->response);
//		if (ev->request && strcmp(osip_message_get_method(ev->request),"OPTIONS")==0)
//			sal->callbacks.ping_reply(op);
//	}
//	if (ev->request && strcmp(osip_message_get_method(ev->request),"MESSAGE")==0) {
//		/*out of call message acknolegment*/
//		SalTextDeliveryStatus status=SalTextDeliveryFailed;
//		if (ev->response){
//			if (ev->response->status_code<200){
//				status=SalTextDeliveryInProgress;
//			}else if (ev->response->status_code<300 && ev->response->status_code>=200){
//				status=SalTextDeliveryDone;
//			}
//		}
//		sal->callbacks.text_delivery_update(op,status);
//	}
    SalOp *op=find_op(sal,ev);
    
	if (op==NULL){
		PrintConsole("sal_eXosip2 other_request_reply(): Receiving response to unknown request.\n");
		return;
	}
	if (ev->response){
		update_contact_from_response(op,ev->response);
		if (ev->request && strcmp(osip_message_get_method(ev->request),"OPTIONS")==0) {
			if(op->check_account_online)
				sal->callbacks.option_reply(op, ev->response->status_code);
			else
				sal->callbacks.ping_reply(op);
		}
		if (ev->request && strcmp(osip_message_get_method(ev->request),"MESSAGE")==0) {
			PrintConsole("sal_eXosip2 Receive MESSAGE response %d\n",ev->response->status_code);

			osip_header_t *date = NULL;
			osip_message_get_date(ev->response,0, &date);
			if(date)
				sal->callbacks.text_send_report(sal,op->msgid,date->hvalue,ev->response->status_code);
			else
				sal->callbacks.text_send_report(sal,op->msgid,"",ev->response->status_code);
		}
	} else {
        if (ev->request && strcmp(osip_message_get_method(ev->request),"MESSAGE")==0) {
			PrintConsole("sal_eXosip2 MESSAGE request time out!\n");
			sal->callbacks.text_send_report(sal,op->msgid,"",-1);
		}
    }
}

static void process_in_call_reply(Sal *sal, eXosip_event_t *ev){
	SalOp *op=find_op(sal,ev);
	if (ev->response){
		if (ev->request && strcmp(osip_message_get_method(ev->request),"NOTIFY")==0){
			if (op->sipfrag_pending){
				send_notify_for_refer(op->did,op->sipfrag_pending);
				op->sipfrag_pending=NULL;
			}
		}
	}
}

static bool_t process_event(Sal *sal, eXosip_event_t *ev){
	PrintConsole("sal_eXosip2 serphone process event get a message %d\n",ev->type);
	switch(ev->type){
		case EXOSIP_CALL_ANSWERED:
			PrintConsole("sal_eXosip2 CALL_ANSWERED\n");
			call_accepted(sal,ev);
			authentication_ok(sal,ev);
			break;
		case EXOSIP_CALL_CLOSED:
		case EXOSIP_CALL_CANCELLED:
			PrintConsole("sal_eXosip2 CALL_CLOSED or CANCELLED\n");
			call_terminated(sal,ev);
			break;
		case EXOSIP_CALL_TIMEOUT:
		case EXOSIP_CALL_NOANSWER:
			PrintConsole("sal_eXosip2 CALL_TIMEOUT or NOANSWER\n");
			return call_failure(sal,ev);
			break;
		case EXOSIP_CALL_REQUESTFAILURE:
		case EXOSIP_CALL_GLOBALFAILURE:
		case EXOSIP_CALL_SERVERFAILURE:
			PrintConsole("sal_eXosip2 CALL_REQUESTFAILURE or GLOBALFAILURE or SERVERFAILURE\n");
			return call_failure(sal,ev);
			break;
		case EXOSIP_CALL_RELEASED:
			PrintConsole("sal_eXosip2 CALL_RELEASED\n");
			call_released(sal, ev);
			break;
		case EXOSIP_CALL_INVITE:
			PrintConsole("sal_eXosip2 CALL_NEW\n");
			inc_new_call(sal,ev);
			break;
		case EXOSIP_CALL_REINVITE:
			handle_reinvite(sal,ev);
			break;
		case EXOSIP_CALL_ACK:
			PrintConsole("sal_eXosip2 CALL_ACK\n");
			handle_ack(sal,ev);
			break;
		case EXOSIP_CALL_REDIRECTED:
			PrintConsole("sal_eXosip2 CALL_REDIRECTED\n");
			eXosip_default_action(ev);
			break;
		case EXOSIP_CALL_PROCEEDING:
			PrintConsole("sal_eXosip2 CALL_PROCEEDING\n");
			call_proceeding(sal,ev);
			break;
		case EXOSIP_CALL_RINGING:
			PrintConsole("sal_eXosip2 CALL_RINGING\n");
			call_ringing(sal,ev);
			authentication_ok(sal,ev);
			break;
		case EXOSIP_CALL_MESSAGE_NEW:
			PrintConsole("sal_eXosip2 EXOSIP_CALL_MESSAGE_NEW\n");
			call_message_new(sal,ev);
			break;
		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
			if (ev->response &&
				(ev->response->status_code==407 || ev->response->status_code==401)){
				 return process_authentication(sal,ev);
			}
			break;
		case EXOSIP_CALL_MESSAGE_ANSWERED:
			PrintConsole("sal_eXosip2 EXOSIP_CALL_MESSAGE_ANSWERED\n");
			process_in_call_reply(sal,ev);
		break;
		case EXOSIP_IN_SUBSCRIPTION_NEW:
			PrintConsole("sal_eXosip2 CALL_IN_SUBSCRIPTION_NEW\n");
			sal_exosip_subscription_recv(sal,ev);
			break;
		case EXOSIP_IN_SUBSCRIPTION_RELEASED:
			PrintConsole("sal_eXosip2 CALL_SUBSCRIPTION_NEW\n");
			sal_exosip_in_subscription_closed(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_UPDATE:
			PrintConsole("sal_eXosip2 CALL_SUBSCRIPTION_UPDATE\n");
			break;
		case EXOSIP_SUBSCRIPTION_NOTIFY:
			PrintConsole("sal_eXosip2 CALL_SUBSCRIPTION_NOTIFY\n");
			sal_exosip_notify_recv(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_ANSWERED:
			PrintConsole("sal_eXosip2 EXOSIP_SUBSCRIPTION_ANSWERED, ev->sid=%i, ev->did=%i\n",ev->sid,ev->did);
			sal_exosip_subscription_answered(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_CLOSED:
			PrintConsole("sal_eXosip2 EXOSIP_SUBSCRIPTION_CLOSED\n");
			sal_exosip_subscription_closed(sal,ev);
			break;
		case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:   /**< announce a request failure      */
			if (ev->response && (ev->response->status_code == 407 || ev->response->status_code == 401)){
				return process_authentication(sal,ev);
			}
		case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
		case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
			sal_exosip_subscription_closed(sal,ev);
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			PrintConsole("sal_eXosip2 REGISTRATION_FAILURE\n");
			return registration_failure(sal,ev);
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			authentication_ok(sal,ev);
			registration_success(sal,ev);
			break;
		case EXOSIP_MESSAGE_NEW:
			other_request(sal,ev);
			break;
		case EXOSIP_MESSAGE_PROCEEDING:
		case EXOSIP_MESSAGE_ANSWERED:
		case EXOSIP_MESSAGE_REDIRECTED:
		case EXOSIP_MESSAGE_SERVERFAILURE:
		case EXOSIP_MESSAGE_GLOBALFAILURE:
			other_request_reply(sal,ev);
			break;
		case EXOSIP_MESSAGE_REQUESTFAILURE:
		case EXOSIP_NOTIFICATION_REQUESTFAILURE:
			if (ev->response) {
				switch (ev->response->status_code) {
					case 407:
					case 401:
						return process_authentication(sal,ev);
					case 412: {
						eXosip_automatic_action ();
						return 1;
					}
				}
			}
			other_request_reply(sal,ev);
			break;
        case EXOSIP_STUN_PACKET:
            sal->callbacks.stun_packet(sal, ev->call_id, ev->stun_event, ev->is_video);
            break;
		default:
			PrintConsole("sal_eXosip2 Unhandled exosip event ! %i\n",ev->type);
			break;
	}
	return TRUE;
}

int sal_iterate(Sal *sal){
	eXosip_event_t *ev;
	while((ev=eXosip_event_wait(0,100))!=NULL){
        if (sal && sal->m_parentLock) {
            sal->m_parentLock->Enter();
        }
		if (process_event(sal,ev))
			eXosip_event_free(ev);
        if (sal && sal->m_parentLock) {
            sal->m_parentLock->Leave();
        }
        
	}
#ifdef HAVE_EXOSIP_TRYLOCK
	if (eXosip_trylock()==0){
		eXosip_automatic_refresh();
		eXosip_unlock();
	}else{
		ms_warning("eXosip_trylock busy.");
	}
#else
	eXosip_lock();
	//eXosip_automatic_refresh();
	eXosip_automatic_action();
	eXosip_unlock();
#endif
	return 0;
}

static void register_set_contact(osip_message_t *msg, const char *contact){
	osip_uri_param_t *param = NULL;
	osip_contact_t *ct=NULL;
	char *line=NULL;
	/*we get the line parameter choosed by exosip, and add it to our own contact*/
	osip_message_get_contact(msg,0,&ct);
	if (ct!=NULL){
		osip_uri_uparam_get_byname(ct->url, "line", &param);
		if (param && param->gvalue)
			line=osip_strdup(param->gvalue);
	}
	_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
	osip_message_set_contact(msg,contact);
	osip_message_get_contact(msg,0,&ct);
	osip_uri_uparam_add(ct->url,osip_strdup("line"),line);
}

static void sal_register_add_route(osip_message_t *msg, const char *proxy){
	osip_route_t *route;

	osip_list_special_free(&msg->routes,(void (*)(void*))osip_route_free);
	
	osip_route_init(&route);
	if (osip_route_parse(route,proxy)==0){
		osip_uri_param_t *lr_param = NULL;
		osip_uri_uparam_get_byname(route->url, "lr", &lr_param);
		if (lr_param == NULL){
			osip_uri_uparam_add(route->url,osip_strdup("lr"),NULL);
		}
		osip_list_add(&msg->routes,route,0);
		return;
	}
	osip_route_free(route);
}


int sal_register(SalOp *h, const char *proxy, const char *from, int expires){
	osip_message_t *msg;
	const char *contact=sal_op_get_contact(h);

	sal_op_set_route(h,proxy);
	if (h->rid==-1){
		SalAddress *from_parsed=sal_address_new(from);
		char domain[256];
		char *uri, *domain_ptr = NULL;
		if (from_parsed==NULL) {
			PrintConsole("sal_eXosip2 sal_register() bad from %s\n",from);
			return -1;
		}
		/* Get domain using sal_address_as_string_uri_only() and stripping the username part instead of
		   using sal_address_get_domain() because to have a properly formatted domain with IPv6 proxy addresses. */
		uri = sal_address_as_string_uri_only(from_parsed);
		if (uri) domain_ptr = strchr(uri, '@');
		if (domain_ptr) {
			snprintf(domain,sizeof(domain),"sip:%s",domain_ptr+1);
		} else {
			snprintf(domain,sizeof(domain),"sip:%s",sal_address_get_domain(from_parsed));
		}
		if (uri) {ms_free((void **)&uri);uri = NULL;}
		sal_address_destroy(from_parsed);
		eXosip_lock();
		h->rid=eXosip_register_build_initial_register(from,domain,NULL,expires,&msg);
		if (msg){
			if (contact) register_set_contact(msg,contact);
			sal_register_add_route(msg,proxy);
			sal_add_register(h->base.root,h);
		}else{
			PrintConsole("sal_eXosip2 Could not build initial register.\n");
			eXosip_unlock();
			return -1;
		}
	}else{
		eXosip_lock();
		eXosip_register_build_register(h->rid,expires,&msg);
		sal_register_add_route(msg,proxy);
	}
	if (msg){
        ServiceCore *tmp = (ServiceCore*)sal_get_user_pointer(sal_op_get_sal(h));
        if (tmp->serphone_get_groupID()) {
            osip_message_set_header(msg,"P-CallNetWork",tmp->serphone_get_groupID());
        }
        if (tmp->serphone_core_get_temp_auth()) {
            osip_message_set_header(msg,"P-Call-UserData",tmp->serphone_core_get_registerUserdata());
        }
		eXosip_register_send_register(h->rid,msg);
	}
	eXosip_unlock();
	h->expires=expires;
	return (msg != NULL) ? 0 : -1;
}

int sal_register_refresh(SalOp *op, int expires){
	osip_message_t *msg=NULL;
	const char *contact=sal_op_get_contact(op);
	
	if (op->rid==-1){
		PrintConsole("sal_eXosip2 Unexistant registration context, not possible to refresh.\n");
		return -1;
	}
#ifdef HAVE_EXOSIP_TRYLOCK
	{
		int tries=0;
		/*iOS hack: in the keep alive handler, we have no more than 10 seconds to refresh registers, otherwise the application is suspended forever.
		* In order to prevent this case that can occur when the exosip thread is busy with DNS while network isn't in a good shape, we try to take
		* the exosip lock in a non blocking way, and give up if it takes too long*/
		while (eXosip_trylock()!=0){
			ms_usleep(100000);
			if (tries>30) {/*after 3 seconds, give up*/
				ms_warning("Could not obtain exosip lock in a reasonable time, giving up.");
				return -1;
			}
		}
	}
#else
	eXosip_lock();
#endif
	eXosip_register_build_register(op->rid,expires,&msg);
	if (msg!=NULL){
		if (contact) register_set_contact(msg,contact);
		sal_register_add_route(msg,sal_op_get_route(op));
		eXosip_register_send_register(op->rid,msg);
	}else PrintConsole("sal_eXosip2 Could not build REGISTER refresh message.\n");
	eXosip_unlock();
	return (msg != NULL) ? 0 : -1;
}


int sal_unregister(SalOp *h){
	osip_message_t *msg=NULL;
	eXosip_lock();
	eXosip_register_build_register(h->rid,0,&msg);
    
    if (traceFile) {
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        fwrite(" Unregister called\n", strlen(" Unregister called\n"), 1, traceFile);
        fflush(traceFile);
    }
    
	if (msg) eXosip_register_send_register(h->rid,msg);
	else PrintConsole("sal_eXosip2 Could not build unREGISTER !\n");
	eXosip_unlock();
	return 0;
}

SalAddress * sal_address_new(const char *uri){
	osip_from_t *from;
	osip_from_init(&from);

	// Remove front spaces
	while (uri[0]==' ') {
		uri++;
	}
		
	if (osip_from_parse(from,uri)!=0){
		osip_from_free(from);
		return NULL;
	}
	if (from->displayname!=NULL && from->displayname[0]=='"'){
		char *unquoted=osip_strdup_without_quote(from->displayname);
		osip_free(from->displayname);
		from->displayname=unquoted;
	}
	return (SalAddress*)from;
}

SalAddress * sal_address_clone(const SalAddress *addr){
	osip_from_t *ret=NULL;
	osip_from_clone((osip_from_t*)addr,&ret);
	return (SalAddress*)ret;
}

#define null_if_empty(s) (((s)!=NULL && (s)[0]!='\0') ? (s) : NULL )

const char *sal_address_get_scheme(const SalAddress *addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->scheme);
}

const char *sal_address_get_display_name(const SalAddress* addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->displayname);
}

const char *sal_address_get_username(const SalAddress *addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->username);
}

const char *sal_address_get_domain(const SalAddress *addr){
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->host);
}

void sal_address_set_display_name(SalAddress *addr, const char *display_name){
	osip_from_t *u=(osip_from_t*)addr;
	if (u->displayname!=NULL){
		osip_free(u->displayname);
		u->displayname=NULL;
	}
	if (display_name!=NULL && display_name[0]!='\0'){
		u->displayname=osip_strdup(display_name);
	}
}

void sal_address_set_username(SalAddress *addr, const char *username){
	osip_from_t *uri=(osip_from_t*)addr;
	if (uri->url->username!=NULL){
		osip_free(uri->url->username);
		uri->url->username=NULL;
	}
	if (username)
		uri->url->username=osip_strdup(username);
}

void sal_address_set_domain(SalAddress *addr, const char *host){
	osip_from_t *uri=(osip_from_t*)addr;
	if (uri->url->host!=NULL){
		osip_free(uri->url->host);
		uri->url->host=NULL;
	}
	if (host)
		uri->url->host=osip_strdup(host);
}

void sal_address_set_port(SalAddress *addr, const char *port){
	osip_from_t *uri=(osip_from_t*)addr;
	if (uri->url->port!=NULL){
		osip_free(uri->url->port);
		uri->url->port=NULL;
	}
	if (port)
		uri->url->port=osip_strdup(port);
}

void sal_address_set_port_int(SalAddress *uri, int port){
	char tmp[12];
	if (port==5060){
		/*this is the default, special case to leave the port field blank*/
		sal_address_set_port(uri,NULL);
		return;
	}
	snprintf(tmp,sizeof(tmp),"%i",port);
	sal_address_set_port(uri,tmp);
}

void sal_address_clean(SalAddress *addr){
	osip_generic_param_freelist(& ((osip_from_t*)addr)->gen_params);
	osip_uri_param_freelist(& ((osip_from_t*)addr)->url->url_params);
}

char *sal_address_as_string(const SalAddress *u){
	char *tmp,*ret;
	osip_from_t *from=(osip_from_t *)u;
	char *old_displayname=NULL;
	/* hack to force use of quotes around the displayname*/
	if (from->displayname!=NULL
	    && from->displayname[0]!='"'){
		old_displayname=from->displayname;
		from->displayname=osip_enquote(from->displayname);
	}
	osip_from_to_str(from,&tmp);
	if (old_displayname!=NULL){
		ms_free((void **)&from->displayname);
		from->displayname=old_displayname;
	}
	ret=ms_strdup(tmp);
	osip_free(tmp);
	return ret;
}

char *sal_address_as_string_uri_only(const SalAddress *u){
	char *tmp=NULL,*ret;
	osip_uri_to_str(((osip_from_t*)u)->url,&tmp);
	ret=ms_strdup(tmp);
	osip_free(tmp);
	return ret;
}
void sal_address_set_param(SalAddress *u,const char* name,const char* value) {
	osip_uri_param_t *param=NULL;
    osip_uri_uparam_get_byname(((osip_from_t*)u)->url,(char*)name,&param);
    if (param == NULL){
        osip_uri_uparam_add	(((osip_from_t*)u)->url,ms_strdup(name),value ? ms_strdup(value) : NULL);
    } else {
        osip_free(param->gvalue);
        param->gvalue=value ? osip_strdup(value) : NULL;
    }
    
}

void sal_address_destroy(SalAddress *u){
	osip_from_free((osip_from_t*)u);
}

void sal_send_keep_alive()
{
	eXosip_send_keep_alive();
}

void sal_use_tcp_tls_keepalive(Sal *ctx, bool_t enabled) {
	ctx->tcp_tls_keepalive = enabled;
}

void sal_set_keepalive_period(Sal *ctx,unsigned int value) {
//	switch (ctx->transport) {
//		case SalTransportUDP:
//			ctx->keepalive_period = value;
//			break;
//		case SalTransportTCP:
//            ctx->keepalive_period = value;
//            break;
//		case SalTransportTLS:
//			if (ctx->tcp_tls_keepalive) ctx->keepalive_period = value;
//			else ctx->keepalive_period = -1;
//			break;
//		default:
//			break;
//	}
    if (!ctx) {
        return;
    }
    ctx->keepalive_period = value;
	eXosip_set_option (EXOSIP_OPT_UDP_KEEP_ALIVE, &ctx->keepalive_period);
}
unsigned int sal_get_keepalive_period(Sal *ctx) {
	return ctx->keepalive_period;
}

const char * sal_address_get_port(const SalAddress *addr) {
	const osip_from_t *u=(const osip_from_t*)addr;
	return null_if_empty(u->url->port);
}

int sal_address_get_port_int(const SalAddress *uri) {
	const char* port = sal_address_get_port(uri);
	if (port != NULL) {
		return atoi(port);
	} else {
		return 5060;
	}
}
SalTransport sal_address_get_transport(const SalAddress* addr) {
    const osip_from_t *u=(const osip_from_t*)addr;
    osip_uri_param_t *transport_param=NULL;
    osip_uri_uparam_get_byname(u->url,"transport",&transport_param);
    if (transport_param == NULL){
        return SalTransportUDP;
    }  else {
        return sal_transport_parse(transport_param->gvalue);
    }
}
void sal_address_set_transport(SalAddress* addr,SalTransport transport) {
    sal_address_set_param(addr, "transport", sal_transport_to_string(transport));
}

/* sends a reinvite. Local media description may have changed by application since call establishment*/
int sal_call_update(SalOp *h, const char *subject){
	int err=0;
	osip_message_t *reinvite=NULL;

	eXosip_lock();
	if(eXosip_call_build_request(h->did,"INVITE",&reinvite) != 0 || reinvite==NULL){
		eXosip_unlock();
		return -1;
	}
	eXosip_unlock();
	osip_message_set_subject(reinvite,subject);
	osip_message_set_allow(reinvite, "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
	if (h->base.contact){
		_osip_list_set_empty(&reinvite->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(reinvite,h->base.contact);
	}
	if (h->base.root->session_expires!=0){
		char expires[16]  = {'\0'};
		sprintf(expires, "%d", h->base.root->session_expires);
		osip_message_set_header(reinvite, "Session-expires", expires);
		osip_message_set_supported(reinvite, "timer");
	}
	if (h->base.local_media){
		h->sdp_offering=TRUE;
		set_sdp_from_desc(reinvite,h->base.local_media);
	}else h->sdp_offering=FALSE;
	eXosip_lock();
	err = eXosip_call_send_request(h->did, reinvite);
	eXosip_unlock();
	return err;
}
void sal_reuse_authorization(Sal *ctx, bool_t value) {
	ctx->reuse_authorization=value;
}

void print_sdp(sdp_message_t *sdp)
{
    PrintConsole("sal_eXosip2 check sdp:\n");
    char *msg=NULL;
    sdp_message_to_str(sdp, &msg);
    PrintConsole("sal_eXosip2 %s",msg);
    PrintConsole("sal_eXosip2 \ncheck sdp end\n");
}
