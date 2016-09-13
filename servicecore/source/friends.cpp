
#include "serprivate.h"
#include "sometools.h"
#include "salpr.h"
#include "servicecore.h"
#include "friends.h"
#include "serphonecall.h"
#include "ECMedia.h"

#define key_compare(s1,s2)	strcmp(s1,s2)


const char *__policy_enum_to_str(SerphoneSubscribePolicy pol){
	switch(pol){
		case LinphoneSPAccept:
			return "accept";
			break;
		case LinphoneSPDeny:
			return "deny";
			break;
		case LinphoneSPWait:
			return "wait";
			break;
	}
	PrintConsole("Invalid policy enum value.\n");
	return "wait";
}

SerphoneSubscribePolicy __policy_str_to_enum(const char* pol)
{
	if (key_compare("accept",pol)==0){
		return LinphoneSPAccept;
	}
	if (key_compare("deny",pol)==0){
		return LinphoneSPDeny;
	}
	if (key_compare("wait",pol)==0){
		return LinphoneSPWait;
	}
	PrintConsole("Unrecognized subscribe policy: %s\n",pol);
	return LinphoneSPWait;
}

void serphone_friend_write_to_config_file(LpConfig *config, SerphoneFriend *lf, int index)
{
	char key[50];
	char *tmp;
	const char *refkey;
	
	sprintf(key,"friend_%i",index);
	
	if (lf==NULL){
		lp_config_clean_section(config,key);
		return;
	}
	if (lf->uri!=NULL){
		tmp=serphone_address_as_string(lf->uri);
		if (tmp==NULL) {
			return;
		}
		lp_config_set_string(config,key,"url",tmp);
		ms_free((void **)&tmp);
        tmp = NULL;
	}
	lp_config_set_string(config,key,"pol",__policy_enum_to_str(lf->pol));
	lp_config_set_int(config,key,"subscribe",lf->subscribe);

	refkey=lf->serphone_friend_get_ref_key();
	if (refkey){
		lp_config_set_string(config,key,"refkey",refkey);
	}
}


SerphoneFriend * serphone_friend_new()
{
//	SerphoneFriend *obj=ms_new(SerphoneFriend,1);
	SerphoneFriend *obj= new SerphoneFriend;
	obj->pol=LinphoneSPAccept;
	obj->status=LinphoneStatusOffline;
	obj->subscribe=TRUE;
	return obj;	
}

SerphoneFriend * serphone_friend_new_from_config_file(ServiceCore *core, int index)
{
	const char *tmp;
	char item[50];
	int a;
	SerphoneFriend *lf;
	LpConfig *config=core->config;
	
	sprintf(item,"friend_%i",index);
	
	if (!lp_config_has_section(config,item)){
		return NULL;
	}
	
	tmp=lp_config_get_string(config,item,"url",NULL);
	if (tmp==NULL) {
		return NULL;
	}
	lf=serphone_friend_new_with_addr(tmp);
	if (lf==NULL) {
		return NULL;
	}
	tmp=lp_config_get_string(config,item,"pol",NULL);
	if (tmp==NULL) lf->serphone_friend_set_inc_subscribe_policy(LinphoneSPWait);
	else{
		lf->serphone_friend_set_inc_subscribe_policy(__policy_str_to_enum(tmp));
	}
	a=lp_config_get_int(config,item,"subscribe",0);
	lf->serphone_friend_send_subscribe(a);
		
	lf->serphone_friend_set_ref_key(lp_config_get_string(config,item,"refkey",NULL));
	return lf;
}

SerphoneFriend *serphone_friend_new_with_addr(const char *addr)
{
	SerphoneAddress* serphone_address = serphone_address_new(addr);
	if (serphone_address == NULL) {
		PrintConsole("Cannot create friend for address [%s]\n",addr?addr:"null");
		return NULL;
	}
	SerphoneFriend *fr=serphone_friend_new();
	if (fr->serphone_friend_set_addr(serphone_address)<0){
		serphone_friend_destroy(&fr);
        fr = NULL;
		return NULL;
	}
	return fr;
}

void serphone_friend_destroy(SerphoneFriend **lf)
{
	if ((*lf)->insub) {
		sal_op_release((*lf)->insub);
		(*lf)->insub=NULL;
	}
	if ((*lf)->outsub){
		sal_op_release((*lf)->outsub);
		(*lf)->outsub=NULL;
	}
	if ((*lf)->uri!=NULL) serphone_address_destroy((*lf)->uri);
	if ((*lf)->info!=NULL) buddy_info_free(&(*lf)->info);
//	ms_free(lf);
	delete *lf;
    *lf = NULL;
}

SerphoneFriend *serphone_find_friend_by_inc_subscribe(MSList *l, SalOp *op)
{
	MSList *elem;
	for (elem=l;elem!=NULL;elem=elem->next){
		SerphoneFriend *lf=(SerphoneFriend*)elem->data;
		if (lf->insub==op) return lf;
	}
	return NULL;
}

SerphoneFriend *serphone_find_friend_by_out_subscribe(MSList *l, SalOp *op)
{
	MSList *elem;
	for (elem=l;elem!=NULL;elem=elem->next){
		SerphoneFriend *lf=(SerphoneFriend*)elem->data;
		if (lf->outsub==op) return lf;
	}
	return NULL;
}

MSList *serphone_find_friend(MSList *fl, const SerphoneAddress *myfriend, SerphoneFriend **lf)
{
	MSList *res=NULL;
	SerphoneFriend dummy;
	if (lf!=NULL) *lf=NULL;
	dummy.uri=(SerphoneAddress*)myfriend;
	res=ms_list_find_custom(fl,friend_compare,&dummy);
	if (lf!=NULL && res!=NULL) *lf=(SerphoneFriend*)res->data;
	return res;
}

int friend_compare(const void * a, const void * b)
{
	SerphoneAddress *fa=((SerphoneFriend*)a)->uri;
	SerphoneAddress *fb=((SerphoneFriend*)b)->uri;
	if (serphone_address_weak_equal (fa,fb)) return 0;
	return 1;
}

const char *serphone_online_status_to_string(SerphoneOnlineStatus ss)
{
	const char *str=NULL;
	switch(ss){
		case LinphoneStatusOnline:
		str=_("Online");
		break;
		case LinphoneStatusBusy:
		str=_("Busy");
		break;
		case LinphoneStatusBeRightBack:
		str=_("Be right back");
		break;
		case LinphoneStatusAway:
		str=_("Away");
		break;
		case LinphoneStatusOnThePhone:
		str=_("On the phone");
		break;
		case LinphoneStatusOutToLunch:
		str=_("Out to lunch");
		break;
		case LinphoneStatusDoNotDisturb:
		str=_("Do not disturb");
		break;
		case LinphoneStatusMoved:
		str=_("Moved");
		break;
		case LinphoneStatusAltService:
		str=_("Using another messaging service");
		break;
		case LinphoneStatusOffline:
		str=_("Offline");
		break;
		case LinphoneStatusPending:
		str=_("Pending");
		break;
		default:
		str=_("Unknown-bug");
	}
	return str;
}

void serphone_friend_close_subscriptions(SerphoneFriend **lf)
{
	(*lf)->serphone_friend_unsubscribe();
	if ((*lf)->insub){
		sal_notify_close((*lf)->insub);
	}
}

SerphoneFriend::SerphoneFriend()
{
	uri = NULL;
	insub = NULL;
	outsub = NULL;
	pol = (SerphoneSubscribePolicy)0;
	status = (SerphoneOnlineStatus)0;
	lc = NULL;
	info = NULL;
	refkey = NULL;
	subscribe = FALSE;
	subscribe_active =FALSE;
	inc_subscribe_pending = FALSE;
	commit = FALSE;
}

SerphoneFriend::~SerphoneFriend()
{
}

void SerphoneFriend::serphone_friend_notify(SerphoneOnlineStatus os)
{
	char *addr=serphone_address_as_string(serphone_friend_get_address());
	PrintConsole("Want to notify %s, insub=%p\n",addr,insub);
	ms_free((void **)&addr);
    addr = NULL;
	if (insub!=NULL){
		sal_notify_presence(insub,serphone_online_status_to_sal(os),NULL);
	}
}

void SerphoneFriend::serphone_friend_unsubscribe()
{
	if (outsub!=NULL) {
		sal_unsubscribe(outsub);
		subscribe_active=FALSE;
	}
}

/**
 * get address of this friend
  * @return #SerphoneAddress
 */
const SerphoneAddress *SerphoneFriend::serphone_friend_get_address()
{
	return uri;
}

/**
 * Configure #SerphoneFriend to subscribe to presence information
 * @param val if TRUE this friend will receive subscription message
 */
int SerphoneFriend::serphone_friend_enable_subscribes(bool_t val)
{
	subscribe=val;
	return 0;
}


/**
 * Configure incoming subscription policy for this friend.
 * @param pol #SerphoneSubscribePolicy policy to apply.
 */
int SerphoneFriend::serphone_friend_set_inc_subscribe_policy(SerphoneSubscribePolicy pol)
{
	this->pol=pol;
	return 0;
}

/**
 * get current subscription policy for this #LinphoneFriend
 * @return #SerphoneSubscribePolicy
 *
 */
SerphoneSubscribePolicy SerphoneFriend::serphone_friend_get_inc_subscribe_policy()
{
	return pol;
}

void SerphoneFriend::serphone_friend_edit()
{
}

void SerphoneFriend::serphone_friend_done()
{
	if (lc==NULL) return;
	serphone_friend_apply(lc);
}

SerphoneOnlineStatus SerphoneFriend::serphone_friend_get_status()
{
	return status;
}

BuddyInfo * SerphoneFriend::serphone_friend_get_info()
{
	return info;
}

void SerphoneFriend::serphone_friend_set_ref_key(const char *key)
{
	if (refkey!=NULL){
		ms_free((void **)&refkey);
		refkey=NULL;
	}
	if (key)
		refkey=ms_strdup(key);
	if (lc)
		lc->serphone_core_write_friends_config();
}

const char *SerphoneFriend::serphone_friend_get_ref_key()
{
	return refkey;
}

bool_t SerphoneFriend::serphone_friend_in_list()
{
	return lc!=NULL;
}

void SerphoneFriend::serphone_friend_apply(ServiceCore *lc)
{
	if (uri==NULL) {
		PrintConsole("No sip url defined.\n");
		return;
	}
	this->lc=lc;
	
	lc->serphone_core_write_friends_config();

	if (inc_subscribe_pending){
		switch(pol){
			case LinphoneSPWait:
				serphone_friend_notify(LinphoneStatusPending);
				break;
			case LinphoneSPAccept:
				if (this->lc!=NULL)
				  {
					serphone_friend_notify(this->lc->presence_mode);
				  }
				break;
			case LinphoneSPDeny:
				serphone_friend_notify(LinphoneStatusOffline);
				break;
		}
		inc_subscribe_pending=FALSE;
	}
	if (subscribe && subscribe_active==FALSE){
		__serphone_friend_do_subscribe();
	}
	lc->bl_refresh=TRUE;
	commit=FALSE;
}

void SerphoneFriend::__serphone_friend_do_subscribe()
{
	char *myfriend=NULL;
	const char *route=NULL;
	const char *from=NULL;
	const char *fixed_contact=NULL;
	SerphoneProxyConfig *cfg;
	
	myfriend=serphone_address_as_string(uri);
	cfg=lc->serphone_core_lookup_known_proxy(serphone_friend_get_address());
	if (cfg!=NULL){
		route=cfg->serphone_proxy_config_get_route();
		from=cfg->serphone_proxy_config_get_identity();
		if (cfg->op){
			fixed_contact=sal_op_get_contact(cfg->op);
			if (fixed_contact) {
				PrintConsole("Contact for subscribe has been fixed using proxy to %s\n",fixed_contact);
			}
		}
	}else from=lc->serphone_core_get_primary_contact();
	if (outsub==NULL){
		/* people for which we don't have yet an answer should appear as offline */
		status=LinphoneStatusOffline;
		/*
		if (lc->vtable.notify_recv)
			lc->vtable.notify_recv(fr->lc,(LinphoneFriend*)fr);
		 */
	}else{
		sal_op_release(outsub);
		outsub=NULL;
	}
	outsub=sal_op_new(lc->sal);
	sal_op_set_route(outsub,route);
	sal_op_set_contact(outsub,fixed_contact);
	sal_subscribe_presence(outsub,from,myfriend);
	subscribe_active=TRUE;
	ms_free((void **)&myfriend);
    myfriend = NULL;
}

int SerphoneFriend::serphone_friend_set_addr(const SerphoneAddress *addr)
{
	SerphoneAddress *fr=serphone_address_clone(addr);
	serphone_address_clean(fr);
	if (uri!=NULL) serphone_address_destroy(uri);	
	uri=fr;
	return 0;
}

int SerphoneFriend::serphone_friend_set_name(const char *name)
{
	SerphoneAddress *fr=uri;
	if (fr==NULL){
		PrintConsole("serphone_friend_set_sip_addr() must be called before serphone_friend_set_name().\n");
		return -1;
	}
	serphone_address_set_display_name(fr,name);
	return 0;
}

bool_t SerphoneFriend::serphone_friend_get_send_subscribe()
{
	return subscribe;
}
