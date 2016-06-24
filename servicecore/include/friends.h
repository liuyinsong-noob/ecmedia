#ifndef FRIENDS_H_
#define FRIENDS_H_

#include "serprivate.h"
#include "lpconfig.h"
#include "sipsetup.h"

/**
 * return humain readable presence status
 * @param ss
 */
const char *serphone_online_status_to_string(SerphoneOnlineStatus ss);

class SerphoneFriend{
public:
	SerphoneFriend();
	~SerphoneFriend();
public:
	 int serphone_friend_set_name(const char *name);
	 int serphone_friend_set_addr(const SerphoneAddress *addr);
     const SerphoneAddress *serphone_friend_get_address();
	 bool_t serphone_friend_get_send_subscribe();
     SerphoneSubscribePolicy serphone_friend_get_inc_subscribe_policy();

     void serphone_friend_notify(SerphoneOnlineStatus os);
	 void serphone_friend_unsubscribe();

     int serphone_friend_enable_subscribes(bool_t val);
     int serphone_friend_set_inc_subscribe_policy(SerphoneSubscribePolicy pol);

     void serphone_friend_edit();
     void serphone_friend_done();

     SerphoneOnlineStatus serphone_friend_get_status();
     BuddyInfo * serphone_friend_get_info();
     void serphone_friend_set_ref_key(const char *key);
     const char *serphone_friend_get_ref_key();
     bool_t serphone_friend_in_list();
	 void serphone_friend_apply(ServiceCore *lc);

#define serphone_friend_send_subscribe serphone_friend_enable_subscribes

protected:
	 void __serphone_friend_do_subscribe();
public:
	SerphoneAddress *uri;
	SalOp *insub;
	SalOp *outsub;
	SerphoneSubscribePolicy pol;
	SerphoneOnlineStatus status;
	class ServiceCore *lc;
	BuddyInfo *info;
	char *refkey;
	bool_t subscribe;
	bool_t subscribe_active;
	bool_t inc_subscribe_pending;
	bool_t commit;
};

SerphoneFriend * serphone_friend_new();
/**
 * Contructor same as linphone_friend_new() + linphone_friend_set_addr()
 * @param addr a buddy address, must be a sip uri like sip:joe@sip.linphone.org
 * @return a new #LinphoneFriend with \link linphone_friend_get_address() address initialized \endlink
 */
void serphone_friend_close_subscriptions(SerphoneFriend **lf);
SerphoneFriend *serphone_friend_new_with_addr(const char *addr);
SerphoneFriend * serphone_friend_new_from_config_file(ServiceCore *core, int index);
const char *__policy_enum_to_str(SerphoneSubscribePolicy pol);
SerphoneSubscribePolicy __policy_str_to_enum(const char* pol);

/**
 * Destructor
 * @param lf #SerphoneFriend object
 */
void serphone_friend_destroy(SerphoneFriend **lf);

SerphoneFriend *serphone_find_friend_by_inc_subscribe(MSList *l, SalOp *op);
SerphoneFriend *serphone_find_friend_by_out_subscribe(MSList *l, SalOp *op);
MSList *serphone_find_friend(MSList *fl, const SerphoneAddress *myfriend, SerphoneFriend **lf);
int friend_compare(const void * a, const void * b);
void serphone_friend_write_to_config_file(LpConfig *config, SerphoneFriend *lf, int index);

#endif
