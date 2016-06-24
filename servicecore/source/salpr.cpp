

/** 
 This header files defines the Signaling Abstraction Layer.
 The purpose of this layer is too allow experiment different call signaling 
 protocols and implementations under linphone, for example SIP, JINGLE...
**/

#define   _CRTDBG_MAP_ALLOC 
#include "salpr.h"

#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

const char* sal_transport_to_string(SalTransport transport) {
	switch (transport) {
		case SalTransportUDP:return "udp";
		case SalTransportTCP: return "tcp";
		case SalTransportTLS:return "tls";
		case SalTransportDTLS:return "dtls";
		default: {
			PrintConsole("Unexpected transport [%i]\n",transport);
			return NULL;
		}    
	}
}

SalTransport sal_transport_parse(const char* param) {
	if (strcasecmp("udp",param)==0) return SalTransportUDP;
	if (strcasecmp("tcp",param)==0) return SalTransportTCP;
	if (strcasecmp("tls",param)==0) return SalTransportTLS;
	if (strcasecmp("dtls",param)==0) return SalTransportDTLS;
	PrintConsole("Unknown transport type[%s], returning UDP\n", param);
	return SalTransportUDP;
}

SalMediaDescription *sal_media_description_new(){
//	SalMediaDescription *md=ms_new0(SalMediaDescription,1);  //ms_new0
	SalMediaDescription *md=(SalMediaDescription *)malloc(sizeof(SalMediaDescription)*1);  //ms_new0
    if (md == NULL) {
        PrintConsole("ERROR: Memory alloc error, maybe not enough memory!\n");
        return NULL;
    }
	memset((void *)md,0,sizeof(SalMediaDescription)*1);


	md->refcount=1;
	return md;
}

static void sal_media_description_destroy(SalMediaDescription *md){
	int i;
	for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;i++){
        MSList *tempList = md->streams[i].payloads;
        for(;tempList!=NULL;tempList=tempList->next){
            payload_type_destroy((PayloadType *)tempList->data);
        }
		ms_list_free(md->streams[i].payloads);
		md->streams[i].payloads=NULL;
	}
	ms_free((void **)&md);
}

void sal_media_description_ref(SalMediaDescription *md){
	md->refcount++;
}

void sal_media_description_unref(SalMediaDescription **md){
	(*md)->refcount--;
	if ((*md)->refcount==0){
		sal_media_description_destroy (*md);
        *md = NULL;
	}
}

SalStreamDescription *sal_media_description_find_stream(SalMediaDescription *md,
    SalMediaProto proto, SalStreamType type){
	int i;
	for(i=0;i<md->nstreams;++i){
		SalStreamDescription *ss=&md->streams[i];
		if (ss->proto==proto && ss->type==type) return ss;
	}
	return NULL;
}

bool_t sal_media_description_empty(const SalMediaDescription *md){
	int i;
	for(i=0;i<md->nstreams;++i){
		const SalStreamDescription *ss=&md->streams[i];
//        printf("i = %d, ss->port = %d\n",i,ss->port);
		if (ss->port!=0) return FALSE;
	}
	return TRUE;
}

void sal_media_description_set_dir(SalMediaDescription *md, SalStreamDir stream_dir){
	int i;
	for(i=0;i<md->nstreams;++i){
		SalStreamDescription *ss=&md->streams[i];
		ss->dir=stream_dir;
	}
}


static bool_t is_null_address(const char *addr){
	return strcmp(addr,"0.0.0.0")==0 || strcmp(addr,"::0")==0;
}

/*check for the presence of at least one stream with requested direction */
static bool_t has_dir(const SalMediaDescription *md, SalStreamDir stream_dir){
	int i;

	/* we are looking for at least one stream with requested direction, inactive streams are ignored*/
	for(i=0;i<md->nstreams;++i){
		const SalStreamDescription *ss=&md->streams[i];
		if (ss->dir==stream_dir) return TRUE;
		/*compatibility check for phones that only used the null address and no attributes */
		if (ss->dir==SalStreamSendRecv && stream_dir==SalStreamSendOnly && (is_null_address(md->addr) || is_null_address(ss->addr)))
			return TRUE;
	}
	return FALSE;
}

bool_t sal_media_description_has_dir(const SalMediaDescription *md, SalStreamDir stream_dir){
	if (stream_dir==SalStreamRecvOnly){
		if (has_dir(md,SalStreamSendOnly) || has_dir(md,SalStreamSendRecv)) return FALSE;
		else return TRUE;
	}else if (stream_dir==SalStreamSendOnly){
		if (has_dir(md,SalStreamRecvOnly) || has_dir(md,SalStreamSendRecv)) return FALSE;
		else return TRUE;
	}else if (stream_dir==SalStreamSendRecv){
		return has_dir(md,SalStreamSendRecv);
	}else{
		/*SalStreamInactive*/
		if (has_dir(md,SalStreamSendOnly) || has_dir(md,SalStreamSendRecv)  || has_dir(md,SalStreamRecvOnly))
			return FALSE;
		else return TRUE;
	}
	return FALSE;
}

/*
static bool_t fmtp_equals(const char *p1, const char *p2){
	if (p1 && p2 && strcmp(p1,p2)==0) return TRUE;
	if (p1==NULL && p2==NULL) return TRUE;
	return FALSE;
}
*/

static bool_t payload_type_equals(const PayloadType *p1, const PayloadType *p2){
	if (p1->type!=p2->type) return FALSE;
	if (strcmp(p1->mime_type,p2->mime_type)!=0) return FALSE;
	if (p1->clock_rate!=p2->clock_rate) return FALSE;
	if (p1->channels!=p2->channels) return FALSE;
	/*
	 Do not compare fmtp right now: they are modified internally when the call is started
	*/
	/*
	if (!fmtp_equals(p1->recv_fmtp,p2->recv_fmtp) ||
	    !fmtp_equals(p1->send_fmtp,p2->send_fmtp))
		return FALSE;
	*/
	return TRUE;
}

static bool_t payload_list_equals(const MSList *l1, const MSList *l2){
	const MSList *e1,*e2;
	for(e1=l1,e2=l2;e1!=NULL && e2!=NULL; e1=e1->next,e2=e2->next){
		PayloadType *p1=(PayloadType*)e1->data;
		PayloadType *p2=(PayloadType*)e2->data;
		if (!payload_type_equals(p1,p2))
			return FALSE;
	}
	if (e1!=NULL || e2!=NULL){
		/*means one list is longer than the other*/
		return FALSE;
	}
	return TRUE;
}

bool_t sal_stream_description_equals(const SalStreamDescription *sd1, const SalStreamDescription *sd2){
  
    /*update begin------------------Sean20130729----------for video ice------------*/
//	if (sd1->proto!=sd2->proto) return FALSE;
//	if (sd1->type!=sd2->type) return FALSE;
//	if (strcmp(sd1->addr,sd2->addr)!=0) return FALSE;
//	if (sd1->port!=sd2->port) return FALSE;
//	if (!payload_list_equals(sd1->payloads,sd2->payloads)) return FALSE;
//	if (sd1->bandwidth!=sd2->bandwidth) return FALSE;
//	if (sd1->ptime!=sd2->ptime) return FALSE;
//	/* compare candidates: TODO */
//	if (sd1->dir!=sd2->dir) return FALSE;
//	return TRUE;
    
    int result = SAL_MEDIA_DESCRIPTION_UNCHANGED;
	int i;
    
	/* A different proto should result in SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED but the encryption change
     needs a stream restart for now, so use SAL_MEDIA_DESCRIPTION_CODEC_CHANGED */
	if (sd1->proto != sd2->proto) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	for (i = 0; i < SAL_CRYPTO_ALGO_MAX; i++) {
		if ((sd1->crypto[i].tag != sd2->crypto[i].tag)
			|| (sd1->crypto[i].algo != sd2->crypto[i].algo)
			|| (strncmp(sd1->crypto[i].master_key, sd2->crypto[i].master_key, sizeof(sd1->crypto[i].master_key) - 1))) {
			result |= SAL_MEDIA_DESCRIPTION_CRYPTO_CHANGED;
		}
	}
    
	if (sd1->type != sd2->type)
        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (strcmp(sd1->rtp_addr, sd2->rtp_addr) != 0)
        result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (sd1->rtp_port != sd2->rtp_port) {
		if ((sd1->rtp_port == 0) || (sd2->rtp_port == 0))
            result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
		else result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	}
    if (strcmp(sd1->addr, sd2->addr) != 0) {
        result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
    }
	if (strcmp(sd1->rtcp_addr, sd2->rtcp_addr) != 0)
        result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (sd1->rtcp_port != sd2->rtcp_port)
        result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (!payload_list_equals(sd1->payloads, sd2->payloads))
        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->bandwidth != sd2->bandwidth)
        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->ptime != sd2->ptime)
        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->dir != sd2->dir)
        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->nack_support != sd2->nack_support)
        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
    
	return result;
    /*update end--------------------Sean20130729----------for video ice------------*/
}

bool_t sal_media_description_equals(const SalMediaDescription *md1, const SalMediaDescription *md2){
    
    /*update begin------------------Sean20140116----------for pause resume------------*/
//	int i;
	
//	if (strcmp(md1->addr,md2->addr)!=0) return FALSE;
//	if (md1->nstreams!=md2->nstreams) return FALSE;
//	if (md1->bandwidth!=md2->bandwidth) return FALSE;
//	for(i=0;i<md1->nstreams;++i){
//		if (!sal_stream_description_equals(&md1->streams[i],&md2->streams[i]))
//			return FALSE;
//	}
//	return TRUE;
    int result = SAL_MEDIA_DESCRIPTION_UNCHANGED;
	int i;
    
	if (strcmp(md1->addr, md2->addr) != 0) result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
//	if (md1->n_total_streams != md2->n_total_streams)
//    {
//        result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
//    }
    
    if (md1->n_active_streams != md2->n_active_streams) {
        if (md1->n_active_streams > md2->n_active_streams) {
            result |= SAL_MEDIA_DESCRIPTION_MEDIA_DECREASED;
        }
        else
            result |= SAL_MEDIA_DESCRIPTION_MEDIA_INCREASED;
    }
    
	if (md1->bandwidth != md2->bandwidth) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	for(i = 0; i < md1->n_total_streams; ++i){
		result |= sal_stream_description_equals(&md1->streams[i], &md2->streams[i]);
	}
	return result;
    /*update end------------------Sean20140116----------for pause resume------------*/
}

static void assign_string(char **str, const char *arg){
	if (*str){
		ms_free((void **)str);
		*str=NULL;
	}
	if (arg)
		*str=ms_strdup(arg);
}

void sal_op_set_contact(SalOp *op, const char *contact){
	assign_string(&((SalOpBase*)op)->contact,contact);
}

void sal_op_set_route(SalOp *op, const char *route){
	assign_string(&((SalOpBase*)op)->route,route);
}

void sal_op_set_from(SalOp *op, const char *from){
	assign_string(&((SalOpBase*)op)->from,from);
}

void sal_op_set_to(SalOp *op, const char *to){
	assign_string(&((SalOpBase*)op)->to,to);
}

void sal_op_set_user_pointer(SalOp *op, void *up){
	((SalOpBase*)op)->user_pointer=up;
}

Sal *sal_op_get_sal(const SalOp *op){
	return ((SalOpBase*)op)->root;
}

const char *sal_op_get_from(const SalOp *op){
	return ((SalOpBase*)op)->from;
}

const char *sal_op_get_to(const SalOp *op){
	return ((SalOpBase*)op)->to;
}

const char *sal_op_get_contact(const SalOp *op){
	return ((SalOpBase*)op)->contact;
}

const char *sal_op_get_route(const SalOp *op){
	return ((SalOpBase*)op)->route;
}

const char *sal_op_get_remote_ua(const SalOp *op){
	return ((SalOpBase*)op)->remote_ua;
}

void *sal_op_get_user_pointer(const SalOp *op){
	return ((SalOpBase*)op)->user_pointer;
}

const char *sal_op_get_remote_sip(const SalOp *op)
{
    return ((SalOpBase*)op)->to;
}

const char *sal_op_get_proxy(const SalOp *op){
	return ((SalOpBase*)op)->route;
}

const char *sal_op_get_network_origin(const SalOp *op){
	return ((SalOpBase*)op)->origin;
}

void __sal_op_init(SalOp *b, Sal *sal){
	memset(b,0,sizeof(SalOpBase));
	((SalOpBase*)b)->root=sal;
}

void __sal_op_set_network_origin(SalOp *op, const char *origin){
	assign_string(&((SalOpBase*)op)->origin,origin);
}


void __sal_op_free(SalOp *op){
	SalOpBase *b=(SalOpBase *)op;
	if (b->from) {
		ms_free((void **)&b->from);
	}
	if (b->to) {
		ms_free((void **)&b->to);
	}
	if (b->route) {
		ms_free((void **)&b->route);
	}
	if (b->contact) {
		ms_free((void **)&b->contact);
	}
	if (b->origin){
		ms_free((void **)&b->origin);
	}
	if (b->remote_ua){
		ms_free((void **)&b->remote_ua);
	}
	if (b->local_media)
		sal_media_description_unref(&b->local_media);
	if (b->remote_media)
		sal_media_description_unref(&b->remote_media);
	ms_free((void **)&op);
}

SalAuthInfo* sal_auth_info_new() {
//	return ms_new0(SalAuthInfo,1);  //ms_new0
	SalAuthInfo* p =(SalAuthInfo *) malloc(sizeof(SalAuthInfo)*1);
	memset((void *)p,0,sizeof(SalAuthInfo)*1);
	return p;
}

SalAuthInfo* sal_auth_info_clone(const SalAuthInfo* auth_info) {
	SalAuthInfo* new_auth_info=sal_auth_info_new();
	new_auth_info->username=auth_info->username?ms_strdup(auth_info->username):NULL;
	new_auth_info->userid=auth_info->userid?ms_strdup(auth_info->userid):NULL;
	new_auth_info->realm=auth_info->realm?ms_strdup(auth_info->realm):NULL;
	new_auth_info->password=auth_info->password?ms_strdup(auth_info->password):NULL;
	return new_auth_info;
}

void sal_auth_info_delete(const SalAuthInfo* auth_info) {
	if (auth_info->username) ms_free((void **)&auth_info->username);
	if (auth_info->userid) ms_free((void **)&auth_info->userid);
	if (auth_info->realm) ms_free((void **)&auth_info->realm);
	if (auth_info->password) ms_free((void **)&auth_info->password);
	ms_free((void **)&auth_info);
}

