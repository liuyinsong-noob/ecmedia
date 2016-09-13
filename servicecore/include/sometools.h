#ifndef SOMETOOLS_H
#define SOMETOOLS_H

#ifdef __cplusplus
extern "C"
{
#endif
    
#if defined(WEBRTC_MAC)
#include <malloc/malloc.h>
#include <string.h>
#include <netdb.h>
#else
#include <malloc.h>
#endif

#if defined(WEBRTC_LINUX)|| defined(WEBRTC_MAC) || defined(WEBRTC_ANROID)
#include <netdb.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <string.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define snprintf _snprintf
	typedef SOCKET ortp_socket_t;
	typedef int socklen_t;
	typedef  unsigned __int64 uint64_t;
	typedef  __int64 int64_t;

#define vsnprintf	_vsnprintf
#define srandom		srand
#define random		rand

#define SOCKET_OPTION_VALUE	char *

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if defined(_XOPEN_SOURCE_EXTENDED) || !defined(__hpux)
#include <arpa/inet.h>
#endif



#include <sys/time.h>

#ifdef ORTP_INET6
#include <netdb.h>
#endif

typedef int ortp_socket_t;

#define SOCKET_OPTION_VALUE	void *

#endif

#ifdef HAVE_GETTEXT
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#ifndef _
#define _(something)	(something)
#endif
#ifndef ngettext
#define ngettext(singular, plural, number)	(((number)==1)?(singular):(plural))
#endif
#endif

/* path of liblinphone plugins */
#define LINPHONE_PLUGINS_DIR "/plugins"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif


#undef MIN
#undef MAX

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

typedef unsigned char bool_t;
#undef TRUE
#undef FALSE
#define TRUE 1
#define FALSE 0

    
#define MAX_RTP_SIZE	UDP_MAX_SIZE

struct _MSList {
	struct _MSList *next;
	struct _MSList *prev;
	void *data;
};


#include "rtpsession.h"
//////////////////////////////////// RTP /////////////////////////////////////////
//typedef struct rtp_stats          //for compiling project,add this by zdm
//{
//	uint64_t packet_sent;
//	uint64_t sent;		/* bytes sent */
//	uint64_t recv; 		/* bytes of payload received and delivered in time to the application */
//	uint64_t hw_recv;		/* bytes of payload received */
//	uint64_t packet_recv;	/* number of packets received */
//	uint64_t outoftime;		/* number of packets that were received too late */
//	uint64_t cum_packet_loss; /* cumulative number of packet lost */
//	uint64_t bad;			/* packets that did not appear to be RTP */
//	uint64_t discarded;		/* incoming packets discarded because the queue exceeds its max size */
//	uint64_t sent_rtcp_packets;	/* sent RTCP packets counter (only packets that embed a report block are considered) */
//} rtp_stats_t;
//
//typedef enum {                         //for compiling project,add this by zdm
//	ORTP_DEBUG=1,
//	ORTP_MESSAGE=1<<1,
//	ORTP_WARNING=1<<2,
//	ORTP_ERROR=1<<3,
//	ORTP_FATAL=1<<4,
//	ORTP_TRACE=1<<5,
//	ORTP_LOGLEV_END=1<<6
//} OrtpLogLevel;
//
//typedef void (*OrtpLogFunc)(OrtpLogLevel lev, const char *fmt, va_list args);  //for compiling project,add this by zdm
//
int ccp_ortp_file_exist(const char *pathname);

//////////////////////////////////end  RTP /////////////////////////////////////////
typedef bool_t (*SerphoneCoreIterateHook)(void *data);


typedef struct _MSList MSList;

typedef int (*MSCompareFunc)(const void *a, const void *b);

#define ms_return_val_if_fail(_expr_,_ret_)\
	if (!(_expr_)) { PrintConsole("assert "#_expr_ "failed"); return (_ret_);}

#define ms_return_if_fail(_expr_) \
	if (!(_expr_)){ PrintConsole("assert "#_expr_ "failed"); return ;}

#define ms_list_next(elem) ((elem)->next)
MSList * ms_list_append(MSList *elem, void * data);
MSList * ms_list_prepend(MSList *elem, void * data);
MSList * ms_list_free(MSList *elem);
MSList * ms_list_concat(MSList *first, MSList *second);
MSList * ms_list_remove(MSList *first, void *data);
int ms_list_size(const MSList *first);
void ms_list_for_each(const MSList *list, void (*func)(void **));
void ms_list_for_each2(const MSList *list, void (*func)(void *, void *), void *user_data);
MSList *ms_list_remove_link(MSList *list, MSList *elem);
MSList *ms_list_find(MSList *list, void *data);
MSList *ms_list_find_custom(MSList *list, MSCompareFunc compare_func, const void *user_data);
void * ms_list_nth_data(const MSList *list, int index);
int ms_list_position(const MSList *list, MSList *elem);
int ms_list_index(const MSList *list, void *data);
MSList *ms_list_insert_sorted(MSList *list, void *data, MSCompareFunc compare_func);
MSList *ms_list_insert(MSList *list, MSList *before, void *data);
MSList *ms_list_copy(const MSList *list);


/////////////////////////////////// MS stream ///////////////////////////////////////////
#if defined(_MSC_VER)
//#define MS2_PUBLIC	__declspec(dllexport)
#define MS2_PUBLIC 
#else
#define MS2_PUBLIC
#endif

void* ser_malloc(size_t sz);
void ser_free(void **ptr);
void* ser_realloc(void *ptr, size_t sz);
void* ser_malloc0(size_t sz);
char * ser_strdup(const char *tmp);
char *ser_strndup(const char *str,int n);
char *ser_strdup_printf(const char *fmt,...);
char *ser_strdup_vprintf(const char *fmt, va_list ap);


#define ms_malloc	      ser_malloc
#define ms_malloc0	      ser_malloc0
#define ms_realloc	      ser_realloc
#define ms_new(type,count)	(type*)ser_malloc(sizeof(type)*(count))
#define ms_new0(type,count)	(type*)ser_malloc0(sizeof(type)*(count))
#define ms_free ser_free
#define ms_strdup ser_strdup
#define ms_strdup_printf	ser_strdup_printf


void ms_set_mtu(int mtu);

#if defined(_WIN32_WCE)
time_t ms_time (time_t *t);
#else
#define ms_time time
#endif


MS2_PUBLIC int ms_get_payload_max_size();

MS2_PUBLIC void ms_set_payload_max_size(int size);

/**
 * Returns the network Max Transmission Unit to reach destination_host.
 * This will attempt to send one or more big packets to destination_host, to a random port.
 * Those packets are filled with zeroes.
**/
MS2_PUBLIC int ms_discover_mtu(const char *destination_host);

/**
 * Load plugins from a specific directory.
 * This method basically loads all libraries in the specified directory and attempts to call a C function called
 * \<libraryname\>_init. For example if a library 'libdummy.so' or 'libdummy.dll' is found, then the loader tries to locate
 * a C function called 'libdummy_init()' and calls it if it exists.
 * ms_load_plugins() can be used to load non-mediastreamer2 plugins as it does not expect mediastreamer2 specific entry points.
 *
 * @param directory   A directory where plugins library are available.
 *
 * Returns: >0 if successfull, 0 if not plugins loaded, -1 otherwise.
 */
MS2_PUBLIC int ms_load_plugins(const char *directory);

///////////////////////////////////end MS stream ///////////////////////////////////////////

enum ccportp_srtp_crypto_suite_t {
	CCPAES_128_SHA1_80 = 1,
	CCPAES_128_SHA1_32,
    CCPAES_256_SHA1_80,
    CCPAES_256_SHA1_32,
	CCPAES_128_NO_AUTH,
	CCPNO_CIPHER_SHA1_80
};

#define UDP_HDR_SZ 8
#define RTP_HDR_SZ 12
#define IP4_HDR_SZ 20   /*20 is the minimum, but there may be some options*/


#define SRTP_KEY_SZ 100
/* flags for PayloadType::flags */

//#define	PAYLOAD_TYPE_ALLOCATED (1)
//	/* private flags for future use by ortp */
//#define	PAYLOAD_TYPE_PRIV1 (1<<1)
//#define	PAYLOAD_TYPE_PRIV2 (1<<2)
//#define	PAYLOAD_TYPE_PRIV3 (1<<3)
//	/* user flags, can be used by the application on top of oRTP */
#define	PAYLOAD_TYPE_USER_FLAG_0 (1<<4)
//#define	PAYLOAD_TYPE_USER_FLAG_1 (1<<5)
//#define	PAYLOAD_TYPE_USER_FLAG_2 (1<<6)
//	/* ask for more if you need*/
//
//#define PAYLOAD_AUDIO_CONTINUOUS 0
//#define PAYLOAD_AUDIO_PACKETIZED 1
//#define PAYLOAD_VIDEO 2
//#define PAYLOAD_TEXT 4
//#define PAYLOAD_OTHER 3  /* ?? */
//
//struct _PayloadType
//{
//	int type; /**< one of PAYLOAD_* macros*/
//	int clock_rate; /**< rtp clock rate*/
//	char bits_per_sample;	/* in case of continuous audio data */
//	char *zero_pattern;
//	int pattern_length;
//	/* other useful information for the application*/
//	int normal_bitrate;	/*in bit/s */
//	char *mime_type; /**<actually the submime, ex: pcm, pcma, gsm*/
//	int channels; /**< number of channels of audio */
//	char *recv_fmtp; /* various format parameters for the incoming stream */
//	char *send_fmtp; /* various format parameters for the outgoing stream */
//	int flags;
//	void *user_data;
//};

//typedef struct _PayloadType PayloadType;

//PayloadType *payload_type_new(void);
//PayloadType *payload_type_clone(PayloadType *payload);
//char *payload_type_get_rtpmap(PayloadType *pt);
//void payload_type_destroy(PayloadType *pt);
//void payload_type_set_recv_fmtp(PayloadType *pt, const char *fmtp);
//void payload_type_set_send_fmtp(PayloadType *pt, const char *fmtp);
//void payload_type_append_recv_fmtp(PayloadType *pt, const char *fmtp);
//void payload_type_append_send_fmtp(PayloadType *pt, const char *fmtp);

bool payload_type_enabled(const PayloadType *pt);

//#define payload_type_get_bitrate(pt)	((pt)->normal_bitrate)
//#define payload_type_get_rate(pt)		((pt)->clock_rate)
//#define payload_type_get_mime(pt)		((pt)->mime_type)

static inline int get_min_bandwidth(int dbw, int ubw){
	if (dbw<=0) return ubw;
	if (ubw<=0) return dbw;
	return MIN(dbw,ubw);
}

static inline bool_t bandwidth_is_greater(int bw1, int bw2){
	if (bw1<0) return TRUE;
	else if (bw2<0) return FALSE;
	else return bw1>=bw2;
}

static inline int get_video_bandwidth(int total, int audio){
	if (total<=0) return 0;
	return total-audio-10;
}

static inline void set_string(char **dest, const char *src){
	if (*dest){
		ms_free((void **)dest);
		*dest=NULL;
	}
	if (src)
		*dest=ms_strdup(src);
}

#define PAYLOAD_TYPE_ENABLED	PAYLOAD_TYPE_USER_FLAG_0

//#define payload_type_set_flag(pt,flag) (pt)->flags|=((int)flag)

///* audio */
//extern PayloadType payload_type_pcmu8000;
//extern PayloadType payload_type_amr;
//extern PayloadType payload_type_amrwb;
//extern PayloadType payload_type_g7231;
//extern PayloadType payload_type_g729;
//extern PayloadType payload_type_g722;
//extern PayloadType payload_type_g7221;
//extern PayloadType payload_type_gsm;
//extern PayloadType payload_type_ilbc;
//extern PayloadType payload_type_silk8k;
//extern PayloadType payload_type_silk12k;
//extern PayloadType payload_type_silk16k;
//
//	/* video */
//extern PayloadType payload_type_vp8;
//extern PayloadType payload_type_h264;

/* telephone-event */
extern PayloadType ccp_payload_type_telephone_event;

/////////////////////////////////////////////////////////////////
//bool_t fmtp_get_value(const char *fmtp, const char *param_name, char *result, size_t result_len);
//#define RTP_PROFILE_MAX_PAYLOADS 128

//static unsigned int __ortp_log_mask=ORTP_WARNING|ORTP_ERROR|ORTP_FATAL;

//#define ortp_log_level_enabled(level)	(__ortp_log_mask & (level))

/**
 * The RTP profile is a table RTP_PROFILE_MAX_PAYLOADS entries to make the matching
 * between RTP payload type number and the PayloadType that defines the type of
 * media.
**/
//struct _RtpProfile
//{
//	char *name;
//	PayloadType *payload[RTP_PROFILE_MAX_PAYLOADS];
//};


//typedef struct _RtpProfile RtpProfile;

//extern RtpProfile av_profile;

//#define payload_type_set_user_data(pt,p)	(pt)->user_data=(p)
//#define payload_type_get_user_data(pt)		((pt)->user_data)
//
//#define rtp_profile_get_name(profile) 	(const char*)((profile)->name)

//void rtp_profile_set_payload(RtpProfile *prof, int idx, PayloadType *pt);

/**
 *	Set payload type number @index unassigned in the profile.
 *
 *@param profile an RTP profile
 *@param index	the payload type number
**/
//#define rtp_profile_clear_payload(profile,index) \
//	rtp_profile_set_payload(profile,index,NULL)	

/* I prefer have this function inlined because it is very often called in the code */
/**
 *
 *	Gets the payload description of the payload type @index in the profile.
 *
 *@param profile an RTP profile (a #RtpProfile object)
 *@param index	the payload type number
 *@return the payload description (a PayloadType object)
**/
//static PayloadType * rtp_profile_get_payload(RtpProfile *prof, int idx){
//	if (idx<0 || idx>=RTP_PROFILE_MAX_PAYLOADS) {
//		return NULL;
//	}
//	return prof->payload[idx];
//}

//void av_profile_init(RtpProfile *profile);

//void rtp_profile_clear_all(RtpProfile *prof);
//void rtp_profile_set_name(RtpProfile *prof, const char *name);
//PayloadType * rtp_profile_get_payload_from_mime(RtpProfile *profile,const char *mime);
//PayloadType * rtp_profile_get_payload_from_rtpmap(RtpProfile *profile, const char *rtpmap);
//int rtp_profile_get_payload_number_from_mime(RtpProfile *profile,const char *mime);
//int rtp_profile_get_payload_number_from_rtpmap(RtpProfile *profile, const char *rtpmap);
//int rtp_profile_find_payload_number(RtpProfile *prof,const char *mime,int rate, int channels);
//PayloadType * rtp_profile_find_payload(RtpProfile *prof,const char *mime,int rate, int channels);
//int rtp_profile_move_payload(RtpProfile *prof,int oldpos,int newpos);

PayloadType * find_payload(RtpProfile *prof, const char *mime_type, int clock_rate, const char *recv_fmtp);

//RtpProfile * rtp_profile_new(const char *name);
/* clone a profile, payload are not cloned */
//RtpProfile * rtp_profile_clone(RtpProfile *prof);


/*clone a profile and its payloads (ie payload type are newly allocated, not reusing payload types of the reference profile) */
//RtpProfile * rtp_profile_clone_full(RtpProfile *prof);
/* frees the profile and all its PayloadTypes*/
//void rtp_profile_destroy(RtpProfile *prof);


/* some payload types */
bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret);
int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen);


/* getaddrinfo constants */
#ifndef EAI_FAIL
#define EAI_FAIL 4
#endif

#ifndef EAI_FAMILY
#define EAI_FAMILY 5
#endif

#ifndef EAI_NONAME
#define EAI_NONAME 8
#endif

#ifndef AI_PASSIVE
#define AI_PASSIVE 1
#endif

#ifndef AI_CANONNAME
#define AI_CANONNAME 2
#endif

#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 4
#endif

#ifndef NI_NOFQDN
#define NI_NOFQDN 1
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 2
#endif

#ifndef NI_NAMERQD
#define NI_NAMERQD 4
#endif

#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 8
#endif

#ifndef NI_DGRAM
#define NI_DGRAM 16
#endif

int ff_inet_aton (const char * str, struct in_addr * add);

//#ifdef WIN32
//int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
//void freeaddrinfo(struct addrinfo *res);
//int getnameinfo(const struct sockaddr *sa, int salen,char *host, int hostlen,
//                   char *serv, int servlen, int flags);
//const char *gai_strerror(int ecode);
//#endif

//#define getSocketError() strerror(errno)

int ccp_set_non_blocking_socket (ortp_socket_t sock);
ortp_socket_t create_socket(int local_port);
int ccp_close_socket(ortp_socket_t sock);

////////////////////////////////////////////////////////////////////////////////////

int serphone_core_get_local_ip_for(int type, const char *dest, char *result);
#ifdef __cplusplus
}
#endif


#endif
