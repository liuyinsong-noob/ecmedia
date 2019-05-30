
#define   _CRTDBG_MAP_ALLOC 
#include <ctype.h>
#include "sometools.h"
#include "serprivate.h"
#include "servicecore.h"
#include "sipsetup.h"
#include "eXtransport.h"
#include "ECMedia.h"

#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

extern FILE *traceFile;
extern std::string timetodate(time_t const timer);

const char *sphone_registration_state_to_string(SerphoneRegistrationState cs)
{
	 switch(cs){
		case LinphoneRegistrationCleared:
			 return "RegistrationCleared";
		break;
		case LinphoneRegistrationNone:
			 return "RegistrationNone";
		break;
		case LinphoneRegistrationProgress:
			return "RegistrationProgress";
		break;
		case LinphoneRegistrationOk:
			 return "RegistrationOk";
		break;
		case LinphoneRegistrationFailed:
			 return "RegistrationFailed";
		break;
	 }
	 return NULL;
}

static bool_t is_a_phone_number(const char *username){
	const char *p;
	for(p=username;*p!='\0';++p){
		if (isdigit(*p) || 
		    *p==' ' ||
		    *p=='-' ||
		    *p==')' ||
			*p=='(' ||
			*p=='/' ||
			*p=='+') continue;
		else return FALSE;
	}
	return TRUE;
}

static char *flatten_number(const char *number){
	char *result=(char *)malloc(strlen(number)+1); //ms_malloc0
	memset((void *)result,0,strlen(number)+1);


	char *w=result;
	const char *r;
	for(r=number;*r!='\0';++r){
		if (*r=='+' || isdigit(*r)){
			*w++=*r;
		}
	}
	*w++='\0';
	return result;
}

static void copy_result(const char *src, char *dest, size_t destlen, bool_t escape_plus){
	unsigned int i=0;
	
	if (escape_plus && src[0]=='+' && destlen>2){
		dest[0]='0';
		dest[1]='0';
		src++;
		i=2;
	}
	
	for(;(i<destlen-1) && *src!='\0';++i){
		dest[i]=*src;
		src++;
	}
	dest[i]='\0';
}


static char *append_prefix(const char *number, const char *prefix){
	char *res=(char *)malloc(strlen(number)+strlen(prefix)+1);
	strcpy(res,prefix);
	return strcat(res,number);
}

/**
 * Creates an empty proxy config.
**/
SerphoneProxyConfig *serphone_proxy_config_new()
{
	SerphoneProxyConfig *obj=NULL;
//	obj=ms_new(SerphoneProxyConfig,1);  commented by zdm
	obj = new SerphoneProxyConfig;
	obj->serphone_proxy_config_init();
	return obj;
}

SerphoneProxyConfig *serphone_proxy_config_new_from_config_file(LpConfig *config, int index)
{
	const char *tmp;
	const char *identity;
	const char *proxy;
	SerphoneProxyConfig *cfg;
	char key[50];
	
	sprintf(key,"proxy_%i",index);

	if (!lp_config_has_section(config,key)){
		return NULL;
	}

	cfg=serphone_proxy_config_new();

	identity=lp_config_get_string(config,key,"reg_identity",NULL);	
	proxy=lp_config_get_string(config,key,"reg_proxy",NULL);
	
	cfg->serphone_proxy_config_set_identity(identity);
	cfg->serphone_proxy_config_set_server_addr(proxy);
	
	tmp=lp_config_get_string(config,key,"reg_route",NULL);
	if (tmp!=NULL) cfg->serphone_proxy_config_set_route(tmp);

	cfg->serphone_proxy_config_expires(lp_config_get_int(config,key,"reg_expires",600));
	cfg->serphone_proxy_config_enableregister(lp_config_get_int(config,key,"reg_sendregister",0));
	
	cfg->serphone_proxy_config_enable_publish(lp_config_get_int(config,key,"publish",0));

	cfg->serphone_proxy_config_set_dial_escape_plus(lp_config_get_int(config,key,"dial_escape_plus",0));
	cfg->serphone_proxy_config_set_dial_prefix(lp_config_get_string(config,key,"dial_prefix",NULL));
	
	tmp=lp_config_get_string(config,key,"type",NULL);
	if (tmp!=NULL && strlen(tmp)>0) 
		cfg->serphone_proxy_config_set_sip_setup(tmp);

	return cfg;
}

void serphone_proxy_config_update(SerphoneProxyConfig **cfg)
{
	ServiceCore *lc=(*cfg)->lc;
	if ((*cfg)->commit){
		if ((*cfg)->type && (*cfg)->ssctx==NULL){
			(*cfg)->serphone_proxy_config_activate_sip_setup();
		}
		if (!lc->sip_conf.register_only_when_network_is_up || lc->network_reachable)
			(*cfg)->serphone_proxy_config_register();
		if ((*cfg)->publish && (*cfg)->publish_op==NULL){
			(*cfg)->serphone_proxy_config_send_publish(lc->presence_mode);
		}
		(*cfg)->commit=FALSE;
	}
}

/**
 * Destroys a proxy config.
 * 
 * @note: LinphoneProxyConfig that have been removed from LinphoneCore with
 * linphone_core_remove_proxy_config() must not be freed.
**/
void serphone_proxy_config_destroy(SerphoneProxyConfig **obj)
{
	if ((*obj)->reg_proxy!=NULL) {ms_free((void **)&(*obj)->reg_proxy);(*obj)->reg_proxy = NULL;}
	if ((*obj)->reg_identity!=NULL) {ms_free((void **)&(*obj)->reg_identity);(*obj)->reg_identity = NULL;}
	if ((*obj)->reg_route!=NULL) {ms_free((void **)&(*obj)->reg_route);(*obj)->reg_route = NULL;}
	if ((*obj)->ssctx!=NULL) sip_setup_context_free((*obj)->ssctx);
	if ((*obj)->realm!=NULL) {ms_free((void **)&(*obj)->realm);(*obj)->realm = NULL;}
	if ((*obj)->type!=NULL) {ms_free((void **)&(*obj)->type); (*obj)->type = NULL;}
	if ((*obj)->dial_prefix!=NULL) {ms_free((void **)&(*obj)->dial_prefix);(*obj)->dial_prefix = NULL;}
	if ((*obj)->op) sal_op_release((*obj)->op);
	if ((*obj)->publish_op) sal_op_release((*obj)->publish_op);
	delete (*obj);   //this code added by zdm,linphone code is without this line
    *obj = NULL;
}

_SerphoneProxyConfig::_SerphoneProxyConfig()
{
	magic = 0;
	lc = NULL;
	reg_proxy = NULL;
	reg_identity = NULL;
	reg_route = NULL;
	realm = NULL;
	expires = 0;
	reg_time = 0;
	op = NULL;
	type = NULL;
	ssctx = NULL;
	auth_failures = 0;
	dial_prefix = NULL;
	state = (SerphoneRegistrationState)0;
	publish_op = NULL;
	commit = FALSE;
	reg_sendregister = FALSE;
	publish = FALSE;
	dial_escape_plus = FALSE;
	user_data = NULL;
	deletion_date = 0;
	error = (SerphoneReason)0;
}

_SerphoneProxyConfig::~_SerphoneProxyConfig()
{
}

void _SerphoneProxyConfig::serphone_proxy_config_init()
{
//	memset(obj,0,sizeof(LinphoneProxyConfig));
	magic=serphone_proxy_config_magic;
	expires=3600;
}

/**
 * Sets the user identity as a SIP address.
 *
 * This identity is normally formed with display name, username and domain, such 
 * as:
 * Alice <sip:alice@example.net>
 * The REGISTER messages will have from and to set to this identity.
 *
**/
int _SerphoneProxyConfig::serphone_proxy_config_set_identity(const char *identity)
{
	SerphoneAddress *addr;
	if (identity!=NULL && strlen(identity)>0){
		addr=serphone_address_new(identity);
		if (!addr || serphone_address_get_username(addr)==NULL){
			WriteLogToFile("Invalid sip identity: %s\n",identity);
			if (addr)
				serphone_address_destroy(addr);
			return -1;
		}else{
			if (this->reg_identity!=NULL) {
				ms_free((void **)&this->reg_identity);
			}
			this->reg_identity=ms_strdup(identity);
			if (this->realm){
				ms_free((void **)&this->realm);
			}
			this->realm=ms_strdup(serphone_address_get_domain(addr));
            this->port = serphone_address_get_port_int(addr);
			serphone_address_destroy(addr);
			return 0;
		}
	}
	return -1;
}

void _SerphoneProxyConfig::serphone_proxy_config_enable_publish(bool_t val)
{
	publish=val;
}

/**
 * Sets whether libserphone should replace "+" by "00" in dialed numbers (passed to
 * #serphone_core_invite ).
 *
**/
void _SerphoneProxyConfig::serphone_proxy_config_set_dial_escape_plus(bool_t val)
{
	dial_escape_plus=val;
}

/**
 * Sets a dialing prefix to be automatically prepended when inviting a number with 
 * #serphone_core_invite.
 *
**/
void _SerphoneProxyConfig::serphone_proxy_config_set_dial_prefix(const char *prefix)
{
	if (dial_prefix!=NULL){
		ms_free((void **)&dial_prefix);
		dial_prefix=NULL;
	}
	if (prefix && prefix[0]!='\0'){
		dial_prefix=ms_strdup(prefix);
	}
}

SerphoneRegistrationState _SerphoneProxyConfig::serhone_proxy_config_get_state()
{
	return state;
}

void _SerphoneProxyConfig::serphone_proxy_config_set_state(SerphoneRegistrationState rstate, const char *message)
{
	WriteLogToFile("[Register] %s ---> %s\n",sphone_registration_state_to_string(this->state),
				sphone_registration_state_to_string(rstate));
	ServiceCore *core=lc;
	this->state=rstate;
	if (core && core->vtable.registration_state_changed){
		core->vtable.registration_state_changed(lc,this,state,message);
	}
}

/**
 * Returns a boolean indicating that the user is sucessfully registered on the proxy.
**/
bool_t _SerphoneProxyConfig::serphone_proxy_config_is_registered()
{
		return state == LinphoneRegistrationOk;
}

const char *_SerphoneProxyConfig::serphone_proxy_config_get_domain()
{
	return realm;
}

/**
 * Sets a SIP route.
 * When a route is set, all outgoing calls will go to the route's destination if this proxy
 * is the default one (see linphone_core_set_default_proxy() ).
**/
int _SerphoneProxyConfig::serphone_proxy_config_set_route(const char *route)
{
	if (this->reg_route!=NULL){
		ms_free((void **)&this->reg_route);
		this->reg_route=NULL;
	}
	if (route!=NULL){
		SalAddress *addr;
		char *tmp;
		/*try to prepend 'sip:' */
		if (strstr(route,"sip:")==NULL){
			tmp=ms_strdup_printf("sip:%s",route);
		}else tmp=ms_strdup(route);
		addr=sal_address_new(tmp);
		if (addr!=NULL){
			sal_address_destroy(addr);
		}else{
			ms_free((void **)&tmp);
			tmp=NULL;
		}
		this->reg_route=tmp;
	}
	return 0;
}

/**
 * Returns the route set for this proxy configuration.
**/
const char *_SerphoneProxyConfig::serphone_proxy_config_get_route()
{
	return reg_route;
}

/**
 * Returns the SIP identity that belongs to this proxy configuration.
 *
 * The SIP identity is a SIP address (Display Name <sip:username@domain> )
**/
const char *_SerphoneProxyConfig::serphone_proxy_config_get_identity()
{
	return reg_identity;
}

/**
 * Returns TRUE if PUBLISH request is enabled for this proxy.
**/
bool_t _SerphoneProxyConfig::serphone_proxy_config_publish_enabled()
{
	return publish;
}

/**
 * Returns the proxy's SIP address.
**/
const char *_SerphoneProxyConfig::serphone_proxy_config_get_addr()
{
	return reg_proxy;
}

/**
 * Sets the registration expiration time in seconds.
**/
void _SerphoneProxyConfig::serphone_proxy_config_expires(int val)
{
	if (val<0) val=600;
	this->expires=val;
}

/**
 * Returns the duration of registration.
**/
int _SerphoneProxyConfig::serphone_proxy_config_get_expires( )
{
	return expires;
}

void _SerphoneProxyConfig::serphone_proxy_config_enableregister(bool_t val)
{
	reg_sendregister=val;
}

/**
 * Returns TRUE if registration to the proxy is enabled.
**/
bool_t _SerphoneProxyConfig::serphone_proxy_config_register_enabled()
{
	return reg_sendregister;
}

/**
 * Refresh a proxy registration.
 * This is useful if for example you resuming from suspend, thus IP address may have changed.
**/
void _SerphoneProxyConfig::serphone_proxy_config_refresh_register()
{
	if (reg_sendregister && op){
		if (sal_register_refresh(op,expires) == 0) {
			serphone_proxy_config_set_state( LinphoneRegistrationProgress, "Refresh registration");
		}
	}
}

class ServiceCore * _SerphoneProxyConfig::serphone_proxy_config_get_core()
{
	return lc;
}

/**
 * Returns whether libserphone should replace "+" by "00" in dialed numbers (passed to
 * #serphone_core_invite ).
 *
**/
bool_t _SerphoneProxyConfig::serphone_proxy_config_get_dial_escape_plus()
{
	return dial_escape_plus;
}

/**
 * Returns dialing prefix.
 *
 * 
**/
const char * _SerphoneProxyConfig::serphone_proxy_config_get_dial_prefix()
{
	return dial_prefix;
}

SerphoneReason _SerphoneProxyConfig::linphone_proxy_config_get_error()
{
	return error;
}

void _SerphoneProxyConfig::serphone_proxy_config_set_sip_setup(const char *type)
{
	if (this->type)
    {
        ms_free((void **)&this->type);
        this->type = NULL;
    }
	this->type=ms_strdup(type);
	if (serphone_proxy_config_get_addr()==NULL){
		/*put a placeholder so that the sip setup gets saved into the config */
		serphone_proxy_config_set_server_addr("sip:undefined");
	}
}

/**
 * Sets the proxy address
 *
 * Examples of valid sip proxy address are:
 * - IP address: sip:87.98.157.38
 * - IP address with port: sip:87.98.157.38:5062
 * - hostnames : sip:sip.example.net
**/
int _SerphoneProxyConfig::serphone_proxy_config_set_server_addr(const char *server_addr)
{
	SerphoneAddress *addr=NULL;
	char *modified=NULL;
	
	if (reg_proxy!=NULL)
    {
        ms_free((void **)&reg_proxy);
    }
	
	if (server_addr!=NULL && strlen(server_addr)>0){
		if (strstr(server_addr,"sip:")==NULL){
			modified=ms_strdup_printf("sip:%s",server_addr);
			addr=serphone_address_new(modified);
			ms_free((void **)&modified);
            modified = NULL;
		}
		if (addr==NULL)
			addr=serphone_address_new(server_addr);
		if (addr){
			reg_proxy=serphone_address_as_string_uri_only(addr);
			serphone_address_destroy(addr);
		}else{
			WriteLogToFile("Could not parse %s\n",server_addr);
			return -1;
		}
	}
	return 0;
}

int _SerphoneProxyConfig::serphone_proxy_config_normalize_number(const char *username, char *result, size_t result_len)
{
	char *flatten;
	int numlen;
	if (is_a_phone_number(username)){
		flatten=flatten_number(username);
		WriteLogToFile("Flattened number is '%s'\n",flatten);
		numlen=strlen(flatten);
		if (numlen>10 || flatten[0]=='+' || dial_prefix==NULL || dial_prefix[0]=='\0'){
			WriteLogToFile("No need to add a prefix\n");
			/* prefix is already there */
			copy_result(flatten,result,result_len,dial_escape_plus);
			ms_free((void **)&flatten);
            flatten = NULL;
			return 0;
		}else if (dial_prefix && dial_prefix[0]!='\0'){
			char *prefixed;
			int skipped=0;
			WriteLogToFile("Need to prefix with %s\n",dial_prefix);
			if (numlen==10){
				/*remove initial number before prepending prefix*/
				skipped=1;
			}
			prefixed=append_prefix(flatten+skipped,dial_prefix);
			ms_free((void **)&flatten);
            flatten = NULL;
			copy_result(prefixed,result,result_len,dial_escape_plus);
			ms_free((void **)&prefixed);
            prefixed = NULL;
		}
	}else strncpy(result,username,result_len);
	return 0;
}

void _SerphoneProxyConfig::serphone_proxy_config_set_user_data(void * ud)
{
	user_data=ud;
}

void * _SerphoneProxyConfig::serphone_proxy_config_get_user_data()
{
	return user_data;
}

SerphoneReason _SerphoneProxyConfig::serphone_proxy_config_get_error() {
	return error;
}

void _SerphoneProxyConfig::serphone_proxy_config_set_error(SerphoneReason error) {
	this->error = error;
}

void _SerphoneProxyConfig::serphone_proxy_config_edit()
{
	if (reg_sendregister){
		/* unregister */
		if (state != LinphoneRegistrationNone && state != LinphoneRegistrationCleared) {
            if (traceFile) {
                time_t temp = time(NULL);
                std::string strTime = timetodate(temp);
                fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
                fwrite(" Something goes wrong real Unregister called111\n", strlen(" Something goes wrong real Unregister called111\n"), 1, traceFile);
                fflush(traceFile);
            }
			sal_unregister(op);
//            if (reg_proxy) {
//                //stop keep alive
//                char *ipstart = strstr(reg_proxy, ":");
//                char *ipend = strstr(ipstart+1, ":");
//                char *ip = new char[ipend - ipstart];
//                memcpy(ip, ipstart+1, ipend-ipstart-1);
//                ip[ipend - ipstart-1] = '\0';
//                char *portc = new char[strlen(ipend+1) + 1];
//                memcpy(portc, ipend+1, strlen(ipend+1));
//                portc[strlen(ipend+1)] = '\0';
//                int portn = atoi(portc);
//                eXosip_stop_specified_keepalive((const char *)ip, portn);
//            }
		}
	}
}

/**
 * Commits modification made to the proxy configuration.
**/
int _SerphoneProxyConfig::serphone_proxy_config_done()
{
    if (!lc) {
        return -1;
    }
	if (!lc->serphone_proxy_config_check(this)) return -1;
	commit=TRUE;
	//lc->serphone_proxy_config_write_all_to_config_file();
	return 0;
}

void _SerphoneProxyConfig::serphone_proxy_config_apply(ServiceCore *lc)
{
	this->lc=lc;
	serphone_proxy_config_done();
}

void _SerphoneProxyConfig::serphone_proxy_config_activate_sip_setup()
{
	SipSetupContext *ssc;
	SipSetup *ss=sip_setup_lookup(this->type);
	ServiceCore *lc=serphone_proxy_config_get_core();
	unsigned int caps;
	if (!ss) return ;
	ssc=sip_setup_context_new(ss,this);
	this->ssctx=ssc;
	if (this->reg_identity==NULL){
		WriteLogToFile("Invalid identity for this proxy configuration.\n");
		return;
	}
	caps=sip_setup_context_get_capabilities(ssc);
	if (caps & SIP_SETUP_CAP_ACCOUNT_MANAGER){
		if (sip_setup_context_login_account(ssc,this->reg_identity,NULL)!=0){
			if (lc->vtable.display_warning){
				char *tmp=ms_strdup_printf(_("Could not login as %s"),this->reg_identity);
				lc->vtable.display_warning(lc,tmp);
				ms_free((void **)&tmp);
                tmp = NULL;
			}
			return;
		}
	}
	if (caps & SIP_SETUP_CAP_PROXY_PROVIDER){
		char proxy[256];
		if (sip_setup_context_get_proxy(ssc,NULL,proxy,sizeof(proxy))==0){
			serphone_proxy_config_set_server_addr(proxy);
		}else{
			WriteLogToFile("Could not retrieve proxy uri !\n");
		}
	}
	
}

void _SerphoneProxyConfig::serphone_proxy_config_register()
{
	if (this->reg_sendregister){
		char *contact;
		if (this->op)
			sal_op_release(this->op);
		this->op=sal_op_new(this->lc->sal);
		contact=guess_contact_for_register();
		sal_op_set_contact(op,contact);
		ms_free((void **)&contact);
        contact = NULL;
		sal_op_set_user_pointer(op,this);
		if (sal_register(op,reg_proxy,reg_identity,expires)==0) {
			serphone_proxy_config_set_state(LinphoneRegistrationProgress,"Registration in progress");
		} else {
			serphone_proxy_config_set_state(LinphoneRegistrationFailed,"Registration failed");
		}
	}
}

char *_SerphoneProxyConfig::guess_contact_for_register()
{
	SerphoneAddress *proxy=serphone_address_new(reg_proxy);
	char *ret=NULL;
	const char *host;
	if (proxy==NULL) return NULL;
	host=serphone_address_get_domain (proxy);
	if (host!=NULL){
		SerphoneAddress *contact;
		char localip[SERPHONE_IPADDR_SIZE];
		LCSipTransports tr;
		
		lc->serphone_core_get_local_ip(host,localip);
		contact=serphone_address_new(reg_identity);
		serphone_address_set_domain (contact,localip);
		serphone_address_set_port_int(contact,lc->serphone_core_get_sip_port());
		serphone_address_set_display_name(contact,NULL);
		
		lc->serphone_core_get_sip_transports(&tr);
		if (tr.udp_port <= 0) {
			if (tr.tcp_port>0) {
				sal_address_set_param(contact,"transport","tcp");
			} else if (tr.tls_port>0) {
				sal_address_set_param(contact,"transport","tls");
			}
		}
		ret=serphone_address_as_string(contact);
		serphone_address_destroy(contact);
	}
	serphone_address_destroy (proxy);
	return ret;
}

int _SerphoneProxyConfig::serphone_proxy_config_send_publish(SerphoneOnlineStatus presence_mode)
{
	int err;
	SalOp *op=sal_op_new(lc->sal);
	err=sal_publish(op,serphone_proxy_config_get_identity(),
	    serphone_proxy_config_get_identity(),serphone_online_status_to_sal(presence_mode));
	if (publish_op!=NULL)
		sal_op_release(publish_op);
	publish_op=op;
	return err;
}
