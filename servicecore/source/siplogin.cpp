
#include "servicecore.h"
#include "serprivate.h"
#include <ctype.h>


static void sip_login_init_instance(SipSetupContext *ctx){
	SerphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	/*disable registration until the user logs in*/
	cfg->serphone_proxy_config_enable_register(FALSE);
}

static void guess_display_name(SerphoneAddress *from){
	char *dn=(char*)malloc(strlen(serphone_address_get_username(from))+3);
	const char *it;
	char *wptr=dn;
	bool_t begin=TRUE;
	bool_t surname=0;
	for(it=serphone_address_get_username(from);*it!='\0';++it){
		if (begin){
			*wptr=toupper(*it);
			begin=FALSE;
		}else if (*it=='.'){
			if (surname) break;
			*wptr=' ';
			begin=TRUE;
			surname=TRUE;
		}else *wptr=*it;
		wptr++;
	}
	serphone_address_set_display_name(from,dn);
	ms_free((void **)&dn);
}

static int sip_login_do_login(SipSetupContext * ctx, const char *uri, const char *passwd){
	SerphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	ServiceCore *lc=cfg->serphone_proxy_config_get_core();
	SerphoneAuthInfo *auth;
	SerphoneAddress *parsed_uri;
	char *tmp;

	parsed_uri=serphone_address_new(uri);
	if (parsed_uri==NULL){
		return -1;
	}
	if (serphone_address_get_display_name(parsed_uri)!=NULL){
		guess_display_name(parsed_uri);
	}
	tmp=serphone_address_as_string(parsed_uri);
	cfg->serphone_proxy_config_set_identity(tmp);
	if (passwd ) {
		auth=serphone_auth_info_new(serphone_address_get_username(parsed_uri),NULL,passwd,NULL,NULL);
		lc->serphone_core_add_auth_info(auth);
	}
	cfg->serphone_proxy_config_enable_register(TRUE);
	cfg->serphone_proxy_config_done();
	ms_free((void **)&tmp);
	serphone_address_destroy(parsed_uri);
	PrintConsole("SipLogin: done");
	return 0;
}

static int sip_login_do_logout(SipSetupContext * ctx){
	SerphoneProxyConfig *cfg=sip_setup_context_get_proxy_config(ctx);
	cfg->serphone_proxy_config_enable_register(FALSE);
	cfg->serphone_proxy_config_done();
	return 0;
}

/* a simple SipSetup built-in plugin to allow specify the user/password for proxy config at runtime*/
/*
#ifndef _MSC_VER

SipSetup linphone_sip_login={
	.name="SipLogin",
	.capabilities=SIP_SETUP_CAP_LOGIN,
	.init_instance=sip_login_init_instance,
	.login_account=sip_login_do_login,
	.logout_account=sip_login_do_logout
};

#else*/  //commented by zdm
SipSetup linphone_sip_login={
	"SipLogin",
	SIP_SETUP_CAP_LOGIN,
	0,
	NULL,
	NULL,
	sip_login_init_instance,
	NULL,
	NULL,
	NULL,
	sip_login_do_login,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
/*
	NULL,
	NULL,
	NULL,
	sip_login_do_logout
*/// modified by zdm
    sip_login_do_logout,
	NULL,
	NULL
};



//#endif  commented by zdm
