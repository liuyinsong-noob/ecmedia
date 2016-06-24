#define   _CRTDBG_MAP_ALLOC 
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "sipsetup.h"
#include "servicecore.h"
#include "serprivate.h"

#if defined(WEBRTC_MAC)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

extern SipSetup linphone_sip_login;
#ifdef BUILD_WIZARD
extern SipSetup linphone_sip_wizard;
#endif
static SipSetup *all_sip_setups[]={
	&linphone_sip_login,
#ifdef BUILD_WIZARD
	&linphone_sip_wizard,
#endif
	NULL
};

static MSList *registered_sip_setups=NULL;

void sip_setup_register(SipSetup *ss){
	registered_sip_setups=ms_list_append(registered_sip_setups,ss);
}

void sip_setup_register_all(void){
	SipSetup **p=all_sip_setups;
	while(*p!=NULL){
		sip_setup_register(*p);
		p++;
	}
}

const MSList * linphone_core_get_sip_setups(ServiceCore *lc){
	return registered_sip_setups;
}

SipSetup *sip_setup_lookup(const char *type_name){
	MSList *elem;
	for(elem=registered_sip_setups;elem!=NULL;elem=elem->next){
		SipSetup *ss=(SipSetup*)elem->data;
		if ( strcasecmp(ss->name,type_name)==0){
			if (!ss->initialized){
				if (ss->init!=NULL) ss->init();
				ss->initialized=TRUE;
				if (ss->capabilities==0){
					PrintConsole("%s SipSetup isn't capable of anything ?\n",ss->name);
				}
			}
			return ss;
		}
	}
	PrintConsole("no %s setup manager declared.\n",type_name);
	return NULL;
}

void sip_setup_unregister_all(void){
	MSList *elem;
	for(elem=registered_sip_setups;elem!=NULL;elem=elem->next){
		SipSetup *ss=(SipSetup*)elem->data;
		if (ss->initialized){
			if (ss->exit) ss->exit();
			ss->initialized=FALSE;
		}
	}
	ms_list_free(registered_sip_setups);
	registered_sip_setups = NULL;
}

void buddy_lookup_request_set_key(BuddyLookupRequest *req, const char *key){
	if (req->key!=NULL) {
		ms_free((void **)&req->key);
	}
	if (key){
		req->key=ms_strdup(key);
	}
}

void buddy_lookup_request_set_max_results(BuddyLookupRequest *req, int ncount){
	req->max_results=ncount;
}

void buddy_lookup_request_free(BuddyLookupRequest *req){
	if (req->key) {ms_free((void **)&(req->key));}
	if (req->results){
		ms_list_for_each(req->results,(void (*)(void**))buddy_info_free);
		ms_list_free(req->results);
	}
	ms_free((void **)&req);
}

SerphoneProxyConfig *sip_setup_context_get_proxy_config(const SipSetupContext *ctx){
	return ctx->cfg;
}

SipSetupContext *sip_setup_context_new(SipSetup *s, SerphoneProxyConfig *cfg){
//	SipSetupContext *obj=(SipSetupContext*)ms_new0(SipSetupContext,1);  //ms_new0
	SipSetupContext *obj=(SipSetupContext*)malloc(sizeof(SipSetupContext)*1);  //ms_new0
	memset((void *)obj,0,sizeof(SipSetupContext)*1);



	obj->funcs=s;
	obj->data=NULL;
	obj->cfg=cfg;
	if (obj->funcs->init_instance){
		obj->funcs->init_instance(obj);
	}
	return obj;
}

unsigned int sip_setup_get_capabilities(SipSetup *s){
	return s->capabilities;
}

int sip_setup_context_get_capabilities(SipSetupContext *ctx){
	return ctx->funcs->capabilities;
}

int sip_setup_context_create_account(SipSetupContext * ctx, const char *uri, const char *passwd, const char *email, int suscribe){
	if (ctx->funcs->create_account)
		return ctx->funcs->create_account(ctx, uri, passwd, email, suscribe);
	else return -1;
}

int sip_setup_context_account_exists(SipSetupContext *ctx, const char *uri){
	if (ctx->funcs->account_exists)
		return ctx->funcs->account_exists(ctx,uri);
	return -1;
}

int sip_setup_context_account_validated(SipSetupContext *ctx, const char *uri){
	if (ctx->funcs->account_validated)
		return ctx->funcs->account_validated(ctx,uri);
	return -1;
}

int sip_setup_context_login_account(SipSetupContext * ctx, const char *uri, const char *passwd){
	SerphoneAddress *from=serphone_address_new(uri);
	if (from==NULL) {
		PrintConsole("Fail to parse %s\n",uri);
		return -1;
	}
	strncpy(ctx->domain,serphone_address_get_domain(from),sizeof(ctx->domain));
	strncpy(ctx->username,serphone_address_get_username(from),sizeof(ctx->username));
	serphone_address_destroy(from);
	if (ctx->funcs->login_account)
		return ctx->funcs->login_account(ctx,uri,passwd);
	return -1;
}

int sip_setup_context_get_proxy(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz){
	if (ctx->funcs->get_proxy)
		return ctx->funcs->get_proxy(ctx,domain ? domain : ctx->domain,proxy,sz);
	return -1;
}

int sip_setup_context_get_stun_servers(SipSetupContext *ctx, char *stun1, char *stun2, size_t size){
	if (ctx->funcs->get_stun_servers)
		return ctx->funcs->get_stun_servers(ctx,stun1,stun2,size);
	return -1;
}

int sip_setup_context_get_relay(SipSetupContext *ctx,char *relay, size_t size){
	if (ctx->funcs->get_relay)
		return ctx->funcs->get_relay(ctx,relay,size);
	return -1;
}

BuddyLookupRequest *sip_setup_context_create_buddy_lookup_request(SipSetupContext *ctx){
	if (ctx->funcs->buddy_lookup_funcs)
		return ctx->funcs->buddy_lookup_funcs->request_create(ctx);
	return NULL;
}

int sip_setup_context_buddy_lookup_submit(SipSetupContext *ctx , BuddyLookupRequest *req){
	if (ctx->funcs->buddy_lookup_funcs)
		return ctx->funcs->buddy_lookup_funcs->request_submit(ctx,req);
	return -1;
}

int sip_setup_context_buddy_lookup_free(SipSetupContext *ctx , BuddyLookupRequest *req){
	if (ctx->funcs->buddy_lookup_funcs)
		return ctx->funcs->buddy_lookup_funcs->request_free(ctx,req);
	return -1;
}

const char * sip_setup_context_get_notice(SipSetupContext *ctx){
	if (ctx->funcs->get_notice)
		return ctx->funcs->get_notice(ctx);
	return NULL;
}

const char ** sip_setup_context_get_domains(SipSetupContext *ctx){
	if (ctx->funcs->get_domains)
		return ctx->funcs->get_domains(ctx);
	return NULL;
}


int sip_setup_context_logout(SipSetupContext *ctx){
	if (ctx->funcs->logout_account){
		return ctx->funcs->logout_account(ctx);
	}
	return -1;
}

void sip_setup_context_free(SipSetupContext *ctx){
	if (ctx->funcs->uninit_instance){
		ctx->funcs->uninit_instance(ctx);
	}
	ms_free((void **)&ctx);
}


BuddyInfo *buddy_info_new(){
//	return ms_new0(BuddyInfo,1);  //ms_new0
	BuddyInfo * p =(BuddyInfo *)malloc(sizeof(BuddyInfo)*1);
	memset((void *)p,0,sizeof(BuddyInfo)*1);
	return p;
}

void buddy_info_free(BuddyInfo **info){
	if ((*info)->image_data!=NULL)
    {
        ms_free((void **)&(*info)->image_data);
    }
	ms_free((void **)info);
}
