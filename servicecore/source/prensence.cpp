
#include "servicecore.h"
#include "serphonecall.h"
#include "friends.h"
#include "lpconfig.h"
#include "enum.h"
#include "ECMedia.h"

void ServiceCore::serphone_subscription_new(SalOp *op, const char *from)
{
	SerphoneFriend *lf=NULL;
	char *tmp;
	SerphoneAddress *uri;
	SerphoneProxyConfig *cfg;
	const char *fixed_contact;
	
	uri=serphone_address_new(from);
	serphone_address_clean(uri);
	tmp=serphone_address_as_string(uri);

	cfg=serphone_core_lookup_known_proxy(uri);
	if (cfg!=NULL){
		if (cfg->op){
			fixed_contact=sal_op_get_contact(cfg->op);
			if (fixed_contact) {
				sal_op_set_contact (op,fixed_contact);
			}
		}
	}
	/* check if we answer to this subscription */
	if (serphone_find_friend(friends,uri,&lf)!=NULL){
		lf->insub=op;
		lf->inc_subscribe_pending=TRUE;
		sal_subscribe_accept(op);
		lf->serphone_friend_done();	/*this will do all necessary actions */
	}else{
		/* check if this subscriber is in our black list */
		if (serphone_find_friend(this->subscribers,uri,&lf)){
			if (lf->pol==LinphoneSPDeny){
				WriteLogToFile("Rejecting %s because we already rejected it once.\n",from);
				sal_subscribe_decline(op);
			}
			else {
				/* else it is in wait for approval state, because otherwise it is in the friend list.*/
				WriteLogToFile("New subscriber found in friend list, in %s state.\n",__policy_enum_to_str(lf->pol));
			}
		}else {
			sal_subscribe_accept(op);
			serphone_core_add_subscriber(tmp,op);
		}
	}
	serphone_address_destroy(uri);
	ms_free((void **)&tmp);
    tmp = NULL;
}

void ServiceCore::serphone_notify_recv(SalOp *op, SalSubscribeStatus ss, SalPresenceStatus sal_status)
{
	char *tmp;
	SerphoneFriend *lf;
	SerphoneAddress *myfriend=NULL;
	SerphoneOnlineStatus estatus=LinphoneStatusOffline;
	
	switch(sal_status){
		case SalPresenceOffline:
			estatus=LinphoneStatusOffline;
		break;
		case SalPresenceOnline:
			estatus=LinphoneStatusOnline;
		break;
		case SalPresenceBusy:
			estatus=LinphoneStatusBusy;
		break;
		case SalPresenceBerightback:
			estatus=LinphoneStatusBeRightBack;
		break;
		case SalPresenceAway:
			estatus=LinphoneStatusAway;
		break;
		case SalPresenceOnthephone:
			estatus=LinphoneStatusOnThePhone;
		break;
		case SalPresenceOuttolunch:
			estatus=LinphoneStatusOutToLunch;
		break;
		case SalPresenceDonotdisturb:
			estatus=LinphoneStatusDoNotDisturb;
		break;
		case SalPresenceMoved:
		case SalPresenceAltService:
			estatus=LinphoneStatusMoved;
		break;
	}
	lf=serphone_find_friend_by_out_subscribe(friends,op);
	if (lf!=NULL){
		myfriend=lf->uri;
		tmp=serphone_address_as_string(myfriend);
		lf->status=estatus;
		lf->subscribe_active=TRUE;
		if (vtable.notify_presence_recv)
			vtable.notify_presence_recv(this,(SerphoneFriend*)lf);
		ms_free((void **)&tmp);
        tmp = NULL;
	}else{
		WriteLogToFile("But this person is not part of our friend list, so we don't care.\n");
	}
	if (ss==SalSubscribeTerminated){
		sal_op_release(op);
		if (lf){
			lf->outsub=NULL;
			lf->subscribe_active=FALSE;
		}
	}
}

void ServiceCore::serphone_subscription_closed(SalOp *op)
{
	SerphoneFriend *lf;
	lf=serphone_find_friend_by_inc_subscribe(friends,op);
	sal_op_release(op);
	if (lf!=NULL){
		lf->insub=NULL;
	}else{
		WriteLogToFile("Receiving unsuscribe for unknown in-subscribtion from %s\n", sal_op_get_from(op));
	}
}

void ServiceCore::serphone_core_send_initial_subscribes()
{
	const MSList *elem;
	for(elem=friends;elem!=NULL;elem=elem->next){
		SerphoneFriend *f=(SerphoneFriend*)elem->data;
		if (f->commit)
			f->serphone_friend_apply(this);
	}
}

void ServiceCore::serphone_core_add_subscriber(const char *subscriber, SalOp *op)
{
	SerphoneFriend *fl=serphone_friend_new_with_addr(subscriber);
	if (fl==NULL) return ;
	fl->insub=op;
	fl->serphone_friend_set_inc_subscribe_policy(LinphoneSPAccept);
	fl->inc_subscribe_pending=TRUE;
	subscribers=ms_list_append(subscribers,(void *)fl);
	if (vtable.new_subscription_request!=NULL) {
		char *tmp=serphone_address_as_string(fl->uri);
		vtable.new_subscription_request(this,fl,tmp);
		ms_free((void **)&tmp);
        tmp = NULL;
	}
}

void ServiceCore::serphone_core_reject_subscriber(SerphoneFriend *lf){
	lf->serphone_friend_set_inc_subscribe_policy(LinphoneSPDeny);
}

void ServiceCore::serphone_core_add_friend(SerphoneFriend *lf)
{
	ms_return_if_fail(lf->lc==NULL);
	ms_return_if_fail(lf->uri!=NULL);
	if (ms_list_find(this->friends,lf)!=NULL){
		char *tmp=NULL;
		const SerphoneAddress *addr=lf->serphone_friend_get_address();
		if (addr) tmp=serphone_address_as_string(addr);
		WriteLogToFile("Friend %s already in list, ignored.\n", tmp ? tmp : "unknown");
		if (tmp) {ms_free((void **)&tmp);tmp = NULL;}
		return ;
	}
	this->friends=ms_list_append(this->friends,lf);
	if ( serphone_core_ready()) lf->serphone_friend_apply(this);
	else lf->commit=TRUE;
	return ;
}
/**
 *  notify all friends that have subscribed
 * @param os #SerphoneOnlineStatus to notify
 *  */
void ServiceCore::serphone_core_notify_all_friends(SerphoneOnlineStatus os)
{
	MSList *elem;
	WriteLogToFile("Notifying all friends that we are in status %i\n",os);
	for(elem=friends;elem!=NULL;elem=elem->next){
		SerphoneFriend *lf=(SerphoneFriend *)elem->data;
		if (lf->insub){
			lf->serphone_friend_notify(os);
		}
	}
}
