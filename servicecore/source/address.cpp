

#include "sometools.h"
#include "servicecore.h"
#include "salpr.h"
#include "serphoneinterface.h"
#include "ECMedia.h"

SerphoneAddress * serphone_address_new(const char *addr){
	SalAddress *saddr=sal_address_new(addr);
	if (saddr==NULL) PrintConsole("Cannot create LinphoneAddress, bad uri [%s]\n",addr);
	return saddr;
}

/**
 * Clones a LinphoneAddress object.
**/
SerphoneAddress * serphone_address_clone(const SerphoneAddress *addr){
	return sal_address_clone(addr);
}

/**
 * Returns the address scheme, normally "sip".
**/
const char *serphone_address_get_scheme(const SerphoneAddress *u){
	return sal_address_get_scheme(u);
}

/**
 * Returns the display name.
**/
const char *serphone_address_get_display_name(const SerphoneAddress* u){
	return sal_address_get_display_name(u);
}

/**
 * Returns the username.
**/
const char *serphone_address_get_username(const SerphoneAddress *u){
	return sal_address_get_username(u);
}

/**
 * Returns the domain name.
**/
const char *serphone_address_get_domain(const SerphoneAddress *u){
	return sal_address_get_domain(u);
}

/**
 * Sets the display name.
**/
void serphone_address_set_display_name(SerphoneAddress *u, const char *display_name){
	sal_address_set_display_name(u,display_name);
}

/**
 * Sets the username.
**/
void serphone_address_set_username(SerphoneAddress *uri, const char *username){
	sal_address_set_username(uri,username);
}

/**
 * Sets the domain.
**/
void serphone_address_set_domain(SerphoneAddress *uri, const char *host){
	sal_address_set_domain(uri,host);
}

/**
 * Sets the port number.
**/
void serphone_address_set_port(SerphoneAddress *uri, const char *port){
	sal_address_set_port(uri,port);
}

/**
 * Sets the port number.
**/
void serphone_address_set_port_int(SerphoneAddress *uri, int port){
	sal_address_set_port_int(uri,port);
}

/**
 * Removes address's tags and uri headers so that it is displayable to the user.
**/
void serphone_address_clean(SerphoneAddress *uri){
	sal_address_clean(uri);
}

/**
 * Returns the address as a string.
 * The returned char * must be freed by the application. Use ms_free().
**/
char *serphone_address_as_string(const SerphoneAddress *u){
	return sal_address_as_string(u);
}

/**
 * Returns the SIP uri only as a string, that is display name is removed.
 * The returned char * must be freed by the application. Use ms_free().
**/
char *serphone_address_as_string_uri_only(const SerphoneAddress *u){
	return sal_address_as_string_uri_only(u);
}

static bool_t strings_equals(const char *s1, const char *s2){
	if (s1==NULL && s2==NULL) return TRUE;
	if (s1!=NULL && s2!=NULL && strcmp(s1,s2)==0) return TRUE;
	return FALSE;
}

/**
 * Compare two LinphoneAddress ignoring tags and headers, basically just domain, username, and port.
 * Returns TRUE if they are equal.
**/
bool_t serphone_address_weak_equal(const SerphoneAddress *a1, const SerphoneAddress *a2){
	const char *u1,*u2;
	const char *h1,*h2;
	int p1,p2;
	u1=serphone_address_get_username(a1);
	u2=serphone_address_get_username(a2);
	p1=serphone_address_get_port_int(a1);
	p2=serphone_address_get_port_int(a2);
	h1=serphone_address_get_domain(a1);
	h2=serphone_address_get_domain(a2);
	return strings_equals(u1,u2) && strings_equals(h1,h2) && p1==p2;
}

/**
 * Destroys a LinphoneAddress object.
**/
void serphone_address_destroy(SerphoneAddress *u){
	sal_address_destroy(u);
}

int serphone_address_get_port_int(const SerphoneAddress *u) {
	return sal_address_get_port_int(u);
}

const char* serphone_address_get_port(const SerphoneAddress *u) {
	return sal_address_get_port(u);
}