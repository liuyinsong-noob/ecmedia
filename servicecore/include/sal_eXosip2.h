
#ifndef sal_exosip2_h
#define sal_exosip2_h

#include "salpr.h"
#include <eXosip2/eXosip.h>
#include "critical_section_wrapper.h"


sdp_message_t *media_description_to_sdp(const SalMediaDescription *sal);
int sdp_to_media_description(sdp_message_t *sdp, SalMediaDescription *desc, bool_t isreinvite = 0);

struct Sal{
	SalCallbacks callbacks;
    SalTransport transport;
	MSList *calls; /*MSList of SalOp */
	MSList *registers;/*MSList of SalOp */
	MSList *out_subscribes;/*MSList of SalOp */
	MSList *in_subscribes;/*MSList of SalOp */
	MSList *pending_auths;/*MSList of SalOp */
	MSList *other_transactions; /*MSList of SalOp */
	int running;
	int session_expires;
	int keepalive_period;
	void *up; /*user pointer*/
	char* rootCa; /* File _or_ folder containing root CA */
	bool_t one_matching_codec;
	bool_t double_reg;
	bool_t use_rports;
	bool_t use_101;
	bool_t reuse_authorization;
	bool_t verify_server_certs;
	cloopenwebrtc::CriticalSectionWrapper *m_parentLock;
    
    bool_t verify_server_cn;
	bool_t expire_old_contact;
	bool_t add_dates;
	bool_t tcp_tls_keepalive;
    int dscp;
};

struct SalOp{
	SalOpBase base;
	int cid;
	int did;
	int tid;
	int rid;
	int sid;
	int nid;
	int mid;
    char *msgid;
	int expires;
	SalMediaDescription *result;
	sdp_message_t *sdp_answer;
	eXosip_event_t *pending_auth;
	osip_call_id_t *call_id; /*used for out of calls transaction in order
	 			to retrieve the operation when receiving a response*/
	char *replaces;
	char *referred_by;
	char *invite_userdata;
    char *group_id;
//haiyuntong
#ifdef HAIYUNTONG
    char *akey;
    char *bkey;
    char *confKey;
#endif
    
    
	const SalAuthInfo *auth_info;
	const char *sipfrag_pending;
	bool_t supports_session_timers;
	bool_t sdp_offering;
	bool_t reinvite;
	bool_t masquerade_via;
	bool_t auto_answer_asked;
	bool_t check_account_online;
	bool_t terminated;
	int remainTime;
};

void sal_remove_out_subscribe(Sal *sal, SalOp *op);
void sal_remove_in_subscribe(Sal *sal, SalOp *op);
void sal_add_other(Sal *sal, SalOp *op,  osip_message_t *request);

void sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev);
void sal_exosip_subscription_answered(Sal *sal,eXosip_event_t *ev);
void sal_exosip_notify_recv(Sal *sal,eXosip_event_t *ev);
void sal_exosip_subscription_closed(Sal *sal,eXosip_event_t *ev);

void sal_exosip_in_subscription_closed(Sal *sal, eXosip_event_t *ev);
SalOp * sal_find_out_subscribe(Sal *sal, int sid);
SalOp * sal_find_in_subscribe(Sal *sal, int nid);
void sal_exosip_fix_route(SalOp *op);

void _osip_list_set_empty(osip_list_t *l, void (*freefunc)(void*));

#endif
