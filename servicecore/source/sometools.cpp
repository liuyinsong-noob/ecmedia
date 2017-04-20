
#define   _CRTDBG_MAP_ALLOC
#include "sometools.h"
#include "serprivate.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ECMedia.h"

#if !defined(_WIN32_WCE) && !defined(_WIN32)
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif /*_WIN32*/

#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h>
#include   <crtdbg.h>
#endif


#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif





//RtpProfile av_profile;

void* ser_malloc(size_t sz)
{
    return malloc(sz);
}

void ser_free(void **ptr)
{
    if (*ptr == NULL) {
        return;
    }
	free(*ptr);
    *ptr = NULL;
}

void* ser_realloc(void *ptr, size_t sz)
{
	return realloc(ptr,sz);
}

void* ser_malloc0(size_t sz)
{
	void *ptr=ser_malloc(sz);
	memset(ptr,0,sz);
	return ptr;
}

char * ser_strdup(const char *tmp)
{
	size_t sz;
	char *ret;
	if (tmp==NULL)
	  return NULL;

	sz=strlen(tmp)+1;
	ret=(char*)malloc(sz);
	strcpy(ret,tmp);
	ret[sz-1]='\0';
	return ret;
}

char *ser_strndup(const char *str,int n)
{
	int min=MIN((int)strlen(str),n)+1;
	char *ret=(char*)malloc(min);
	strncpy(ret,str,min);
	ret[min-1]='\0';
	return ret;
}

char *ser_strdup_printf(const char *fmt,...)
{
	char *ret;
	va_list args;
	va_start (args, fmt);
	ret=ser_strdup_vprintf(fmt, args);
 	va_end (args);
	return ret;
}

char *ser_strdup_vprintf(const char *fmt, va_list ap)
{
	/* Guess we need no more than 100 bytes. */
	int n, size = 200;
	char *p,*np;
#ifndef WIN32
	va_list cap;/*copy of our argument list: a va_list cannot be re-used (SIGSEGV on linux 64 bits)*/
#endif
	if ((p = (char *) malloc (size)) == NULL)
		return NULL;
	while (1)
	{
		/* Try to print in the allocated space. */
#ifndef WIN32
		va_copy(cap,ap);
		n = vsnprintf (p, size, fmt, cap);
		va_end(cap);
#else
		/*this works on 32 bits, luckily*/
		n = vsnprintf (p, size, fmt, ap);
#endif
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			return p;
		//printf("Reallocing space.\n");
		/* Else try again with more space. */
		if (n > -1)	/* glibc 2.1 */
			size = n + 1;	/* precisely what is needed */
		else		/* glibc 2.0 */
			size *= 2;	/* twice the old size */
		if ((np = (char *) ser_realloc (p, size)) == NULL)
		  {
		    free(p);
		    return NULL;
		  }
		else
		  {
		    p = np;
		  }
	}
}

void ms_set_mtu(int mtu){
	/*60= IPv6+UDP+RTP overhead */
	if (mtu>60){
		if (mtu>1500) mtu=1500;/*limit to 1500, the mediastreamer2 buffer are not large enough anyway*/
		ms_set_payload_max_size(mtu-60);
	}else ms_set_payload_max_size(0);
}

/**
 * The max payload size allowed.
 * Filters that generate data that can be sent through RTP should make packets
 * whose size is below ms_get_payload_max_size().
 * The default value is 1440 computed as the standard internet MTU minus IPv6 header,
 * UDP header and RTP header. As IPV4 header is smaller than IPv6 header, this
 * value works for both.
 *
**/
#define DEFAULT_MAX_PAYLOAD_SIZE 1440

static int max_payload_size=DEFAULT_MAX_PAYLOAD_SIZE;

int ms_get_payload_max_size(){
	return max_payload_size;
}

void ms_set_payload_max_size(int size){
	if (size<=0) size=DEFAULT_MAX_PAYLOAD_SIZE;
	max_payload_size=size;
}

#if defined(WIN32) && !defined(_WIN32_WCE)

HINSTANCE m_IcmpInst = NULL;

typedef struct ip_option_information {
    UCHAR   Ttl;
    UCHAR   Tos;
    UCHAR   Flags;
    UCHAR   OptionsSize;
    PUCHAR  OptionsData;
} IP_OPTION_INFORMATION, * PIP_OPTION_INFORMATION;

typedef BOOL (WINAPI *ICMPCLOSEHANDLE)(HANDLE IcmpHandle);
typedef HANDLE (WINAPI *ICMPCREATEFILE)(VOID);
typedef DWORD (WINAPI *ICMPSENDECHO)(HANDLE IcmpHandle,ULONG DestinationAddress, LPVOID RequestData, WORD RequestSize, PIP_OPTION_INFORMATION RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);

ICMPCLOSEHANDLE pIcmpCloseHandle = NULL;
ICMPCREATEFILE pIcmpCreateFile = NULL;
ICMPSENDECHO pIcmpSendEcho = NULL;

#define IP_FLAG_DF      0x2         // Don't fragment this packet.
#define IP_OPT_ROUTER_ALERT 0x94  // Router Alert Option

#define IP_STATUS_BASE              11000
#define IP_PACKET_TOO_BIG           (IP_STATUS_BASE + 9)
#define IP_REQ_TIMED_OUT            (IP_STATUS_BASE + 10)

static int mtus[] = {
  1500,   // Ethernet, Point-to-Point (default)
  1492,   // IEEE 802.3
  1006,   // SLIP, ARPANET
  576,    // X.25 Networks
  544,    // DEC IP Portal
  512,    // NETBIOS
  508,    // IEEE 802/Source-Rt Bridge, ARCNET
  296,    // Point-to-Point (low delay)
  68,     // Official minimum
  0
};

int ms_discover_mtu(const char *host)
{
  int i;

	struct addrinfo hints,*ai=NULL;
	char port[10];
  char ipaddr[INET6_ADDRSTRLEN];
  int err;

  HANDLE hIcmp;
  unsigned long target_addr;

  struct ip_option_information ip_opts;
  unsigned char reply_buffer[10000];

  if (!m_IcmpInst)
	{
		m_IcmpInst = LoadLibrary(TEXT("icmp.dll"));
		if (m_IcmpInst)
		{
			pIcmpCloseHandle = (ICMPCLOSEHANDLE)GetProcAddress(m_IcmpInst, "IcmpCloseHandle");
			pIcmpCreateFile  = (ICMPCREATEFILE) GetProcAddress(m_IcmpInst, "IcmpCreateFile");
			pIcmpSendEcho =	   (ICMPSENDECHO)   GetProcAddress(m_IcmpInst, "IcmpSendEcho");
		}
	}

  hIcmp = pIcmpCreateFile();

	memset(&hints,0,sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	snprintf(port,sizeof(port),"0");
	err=getaddrinfo(host,port,&hints,&ai);
	if (err!=0){
    pIcmpCloseHandle( hIcmp );
		PrintConsole("getaddrinfo(): error\n");
		return -1;
	}
  getnameinfo (ai->ai_addr, ai->ai_addrlen, ipaddr, sizeof (ipaddr), port,
               sizeof (port), NI_NUMERICHOST | NI_NUMERICSERV);
	freeaddrinfo(ai);

  target_addr=inet_addr(ipaddr);


  /* Prepare the IP options */
  memset(&ip_opts,0,sizeof(ip_opts));
  ip_opts.Ttl=30;
  ip_opts.Flags = IP_FLAG_DF | IP_OPT_ROUTER_ALERT;


  // ignore icmpbuff data contents
  for (i=0;mtus[i]!=0;i++)
  {
    char icmpbuff[2048];
    char *icmp_data = icmpbuff;

    int status = -1;
    if (pIcmpSendEcho)
      status=pIcmpSendEcho(hIcmp,
                          target_addr,
                          (LPVOID)icmp_data,
                          mtus[i]-60, /* icmp_data_size */
                          &ip_opts,
                          reply_buffer,
                          sizeof(reply_buffer),
                          3000L); // 3 seconds
    if (status || GetLastError() == IP_REQ_TIMED_OUT)
    {
      pIcmpCloseHandle( hIcmp );
      return mtus[i];
    }
  }

  pIcmpCloseHandle( hIcmp );

  return -1;
}

#elif defined(__linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

#ifndef IP_MTU
#define IP_MTU 14
#endif

int ms_discover_mtu(const char *host){
	int sock;
	int err,mtu=0,new_mtu;
	socklen_t optlen;
	char port[10];
	struct addrinfo hints,*ai=NULL;
	int rand_port;
	int retry=0;
	struct timeval tv;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	gettimeofday(&tv,NULL);
	srandom(tv.tv_usec);
	rand_port=random() & 0xFFFF;
	if (rand_port<1000) rand_port+=1000;
	snprintf(port,sizeof(port),"%i",rand_port);
	err=getaddrinfo(host,port,&hints,&ai);
	if (err!=0){
		PrintConsole("getaddrinfo(): %s\n",gai_strerror(err));
		return -1;
	}
	sock=socket(PF_INET,SOCK_DGRAM,0);
	if(sock < 0)
	{
		PrintConsole("socket(): %s\n",strerror(errno));
		return sock;
	}
	mtu=IP_PMTUDISC_DO;
	optlen=sizeof(mtu);
	err=setsockopt(sock,IPPROTO_IP,IP_MTU_DISCOVER,&mtu,optlen);
	if (err!=0){
		PrintConsole("setsockopt(): %s\n",strerror(errno));
		err = close(sock);
		if (err!=0)
			PrintConsole("close(): %s\n", strerror(errno));
		return -1;
	}
	err=connect(sock,ai->ai_addr,ai->ai_addrlen);
	freeaddrinfo(ai);
	if (err!=0){
		PrintConsole("connect(): %s\n",strerror(errno));
		err = close(sock);
		if (err !=0)
			PrintConsole("close(): %s\n", strerror(errno));
		return -1;
	}
	mtu=1500;
	do{
		int send_returned;
		int datasize=mtu-28;/*minus IP+UDP overhead*/
		char *buf=(char *)ms_malloc0(datasize);

		send_returned = send(sock,buf,datasize,0);
		if (send_returned==-1){
			/*ignore*/
		}
		ms_free((void **)&buf);
        buf = NULL;
		usleep(500000);/*wait for an icmp message come back */
		err=getsockopt(sock,IPPROTO_IP,IP_MTU,&new_mtu,&optlen);
		if (err!=0){
			PrintConsole("getsockopt(): %s\n",strerror(errno));
			err = close(sock);
			if (err!=0)
				PrintConsole("close(): %s\n", strerror(errno));
			return -1;
		}else{
			PrintConsole("Partial MTU discovered : %i\n",new_mtu);
			if (new_mtu==mtu) break;
			else mtu=new_mtu;
		}
		retry++;
	}while(retry<10);

	PrintConsole("mtu to %s is %i\n",host,mtu);

	err = close(sock);
	if (err!=0)
		PrintConsole("close() %s\n", strerror(errno));
	return mtu;
}

#else

int ms_discover_mtu(const char*host){
	PrintConsole("mtu discovery not implemented.\n");
	return -1;
}

#endif


////////////////////////////////////////////////////////////////

MSList *ms_list_new(void *data){
//	MSList *new_elem=(MSList *)ms_new(MSList,1);  //ms_new
	MSList *new_elem=(MSList *)malloc(sizeof(MSList)*1);  //ms_new


	new_elem->prev=new_elem->next=NULL;
	new_elem->data=data;
	return new_elem;
}

MSList * ms_list_append(MSList *elem, void * data){
	MSList *new_elem=ms_list_new(data);
	MSList *it=elem;
	if (elem==NULL) return new_elem;
	while (it->next!=NULL) it=ms_list_next(it);
	it->next=new_elem;
	new_elem->prev=it;
	return elem;
}

MSList * ms_list_prepend(MSList *elem, void *data){
	MSList *new_elem=ms_list_new(data);
	if (elem!=NULL) {
		new_elem->next=elem;
		elem->prev=new_elem;
	}
	return new_elem;
}


MSList * ms_list_concat(MSList *first, MSList *second){
	MSList *it=first;
	if (it==NULL) return second;
	while(it->next!=NULL) it=ms_list_next(it);
	it->next=second;
	second->prev=it;
	return first;
}

MSList * ms_list_free(MSList *list){
	MSList *elem = list;
	MSList *tmp;
	if (list==NULL) return NULL;
	while(elem->next!=NULL) {
		tmp = elem;
		elem = elem->next;
		ms_free((void **)&tmp);
	}
	ms_free((void **)&elem);
	return NULL;
}

MSList * ms_list_remove(MSList *first, void *data){
	MSList *it;
	it=ms_list_find(first,data);
	if (it) return ms_list_remove_link(first,it);
	else {
		PrintConsole("ms_list_remove: no element with %p data was in the list\n", data);
		return first;
	}
}

int ms_list_size(const MSList *first){
	int n=0;
	while(first!=NULL){
		++n;
		first=first->next;
	}
	return n;
}

void ms_list_for_each(const MSList *list, void (*func)(void **)){
	for(;list!=NULL;list=list->next){
		func((void **)&(list->data));
	}
}

void ms_list_for_each2(const MSList *list, void (*func)(void *, void *), void *user_data){
	for(;list!=NULL;list=list->next){
		func(list->data,user_data);
	}
}

MSList *ms_list_remove_link(MSList *list, MSList *elem){
	MSList *ret;
	if (elem==list){
		ret=elem->next;
		elem->prev=NULL;
		elem->next=NULL;
		if (ret!=NULL) ret->prev=NULL;
		ms_free((void **)&elem);
		return ret;
	}
	elem->prev->next=elem->next;
	if (elem->next!=NULL) elem->next->prev=elem->prev;
	elem->next=NULL;
	elem->prev=NULL;
	ms_free((void **)&elem);
	return list;
}

MSList *ms_list_find(MSList *list, void *data){
	for(;list!=NULL;list=list->next){
		if (list->data==data) return list;
	}
	return NULL;
}

MSList *ms_list_find_custom(MSList *list, int (*compare_func)(const void *, const void*), const void *user_data){
	for(;list!=NULL;list=list->next){
		if (compare_func(list->data,user_data)==0) return list;
	}
	return NULL;
}

void * ms_list_nth_data(const MSList *list, int index){
	int i;
	for(i=0;list!=NULL;list=list->next,++i){
		if (i==index) return list->data;
	}
	return NULL;
}

int ms_list_position(const MSList *list, MSList *elem){
	int i;
	for(i=0;list!=NULL;list=list->next,++i){
		if (elem==list) return i;
	}
	return -1;
}

int ms_list_index(const MSList *list, void *data){
	int i;
	for(i=0;list!=NULL;list=list->next,++i){
		if (data==list->data) return i;
	}
	return -1;
}

MSList *ms_list_insert_sorted(MSList *list, void *data, int (*compare_func)(const void *, const void*)){
	MSList *it,*previt=NULL;
	MSList *nelem;
	MSList *ret=list;
	if (list==NULL) return ms_list_append(list,data);
	else{
		nelem=ms_list_new(data);

		for(it=list;it!=NULL;it=it->next){
			previt=it;
			if (compare_func(data,it->data)<=0){
				nelem->prev=it->prev;
				nelem->next=it;
				if (it->prev!=NULL)
					it->prev->next=nelem;
				else{
					ret=nelem;
				}
				it->prev=nelem;
				return ret;
			}
		}
		previt->next=nelem;
		nelem->prev=previt;
	}
	return ret;
}

MSList *ms_list_insert(MSList *list, MSList *before, void *data){
	MSList *elem;
	if (list==NULL || before==NULL) return ms_list_append(list,data);
	for(elem=list;elem!=NULL;elem=ms_list_next(elem)){
		if (elem==before){
			if (elem->prev==NULL)
				return ms_list_prepend(list,data);
			else{
				MSList *nelem=ms_list_new(data);
				nelem->prev=elem->prev;
				nelem->next=elem;
				elem->prev->next=nelem;
				elem->prev=nelem;
			}
		}
	}
	return list;
}

MSList *ms_list_copy(const MSList *list){
	MSList *copy=NULL;
	const MSList *iter;
	for(iter=list;iter!=NULL;iter=ms_list_next(iter)){
		copy=ms_list_append(copy,iter->data);
	}
	return copy;
}


#ifdef __APPLE__
PayloadType payload_type_opus8k={
    PAYLOAD_AUDIO_PACKETIZED,
    8000,
    0,
    NULL,
    0,
    20000,
    "opus",
    1,
    NULL,
    NULL,
    0,
    NULL
};

PayloadType payload_type_opus16k={
    PAYLOAD_AUDIO_PACKETIZED,
    16000,
    0,
    NULL,
    0,
    25000,
    "opus",
    1,
    NULL,
    NULL,
    0,
    NULL
};
#endif
///////////////////////////////////////////////////////////////
//
//char offset127=127;
//char offset0xD5=(char)0xD5;
//char offset0[4] = {0x00, 0x00, 0x00, 0x00};
//
///*
// * IMPORTANT : some compiler don't support the tagged-field syntax. Those
// * macros are there to trap the problem This means that if you want to keep
// * portability, payload types must be defined with their fields in the right
// * order.
// */
//#if defined(_ISOC99_SOURCE)
//// ISO C99's tagged syntax
//#define TYPE(val)		.type=(val)
//#define CLOCK_RATE(val)		.clock_rate=(val)
//#define BITS_PER_SAMPLE(val)	.bits_per_sample=(val)
//#define ZERO_PATTERN(val)	.zero_pattern=(val)
//#define PATTERN_LENGTH(val)	.pattern_length=(val)
//#define NORMAL_BITRATE(val)	.normal_bitrate=(val)
//#define MIME_TYPE(val)		.mime_type=(val)
//#define CHANNELS(val)		.channels=(val)
//#define FMTP(val)		.FMTP=(val)
//#elif defined(__GNUC__)
//// GCC's legacy tagged syntax (even old versions have it)
//#define TYPE(val)		type: (val)
//#define CLOCK_RATE(val)		clock_rate: (val)
//#define BITS_PER_SAMPLE(val)	bits_per_sample: (val)
//#define ZERO_PATTERN(val)	zero_pattern: (val)
//#define PATTERN_LENGTH(val)	pattern_length: (val)
//#define NORMAL_BITRATE(val)	normal_bitrate: (val)
//#define MIME_TYPE(val)		mime_type: (val)
//#define CHANNELS(val)		channels: (val)
//#define FMTP(val)		FMTP: (val)
//#else
//// No tagged syntax supported
//#define TYPE(val)		(val)
//#define CLOCK_RATE(val)		(val)
//#define BITS_PER_SAMPLE(val)	(val)
//#define ZERO_PATTERN(val)	(val)
//#define PATTERN_LENGTH(val)	(val)
//#define NORMAL_BITRATE(val)	(val)
//#define MIME_TYPE(val)		(val)
//#define CHANNELS(val)		(val)
//#define FMTP(val)		(val)
//
//#endif
//
//PayloadType payload_type_pcmu8000={
//	TYPE( PAYLOAD_AUDIO_CONTINUOUS),
//	CLOCK_RATE( 8000),
//	BITS_PER_SAMPLE(8),
//	ZERO_PATTERN( &offset127),
//	PATTERN_LENGTH( 1),
//	NORMAL_BITRATE( 64000),
//	MIME_TYPE ("PCMU"),
//	CHANNELS(1)
//};
//
//PayloadType payload_type_amr={
//	TYPE(PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(8000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(12200),
//	MIME_TYPE ("AMR"),
//	CHANNELS(1)
//};
//
//PayloadType payload_type_amrwb={
//	TYPE(PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(16000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(23850),
//	MIME_TYPE ("AMR-WB"),
//	CHANNELS(1)
//};
//PayloadType payload_type_g7231=
//{
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(8000),
//	BITS_PER_SAMPLE( 0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH( 0),
//	NORMAL_BITRATE( 6300),
//	MIME_TYPE ("G723"),
//	CHANNELS(1)
//};
//
//PayloadType payload_type_g729={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(8000),
//	BITS_PER_SAMPLE( 0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH( 0),
//	NORMAL_BITRATE( 8000),
//	MIME_TYPE ("G729"),
//	CHANNELS(1)
//};
//
//PayloadType payload_type_g722={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(16000),
//	BITS_PER_SAMPLE( 0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH( 0),
//	NORMAL_BITRATE( 64000),
//	MIME_TYPE ("G722"),
//	CHANNELS(1)
//};
//
//PayloadType payload_type_g7221={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(16000),
//	BITS_PER_SAMPLE( 0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH( 0),
//	NORMAL_BITRATE( 24000),
//	MIME_TYPE ("G7221"),
//	CHANNELS(1)
//};
//PayloadType payload_type_gsm=
//{
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(8000),
//	BITS_PER_SAMPLE( 0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH( 0),
//	NORMAL_BITRATE( 13500),
//	MIME_TYPE ("GSM"),
//	CHANNELS(1)
//};
//PayloadType payload_type_ilbc={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(8000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(13300), /* the minimum, with 30ms frames */
//	MIME_TYPE ("iLBC"),
//	CHANNELS(1),
//};
//
//PayloadType payload_type_silk8k={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(8000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(20000),
//	MIME_TYPE ("SILK"),
//	CHANNELS(1),
//};
//
//PayloadType payload_type_silk12k={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(12000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(20000),
//	MIME_TYPE ("SILK"),
//	CHANNELS(1),
//};
//
//PayloadType payload_type_silk16k={
//	TYPE( PAYLOAD_AUDIO_PACKETIZED),
//	CLOCK_RATE(16000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(20000),
//	MIME_TYPE ("SILK"),
//	CHANNELS(1),
//};
//
//PayloadType payload_type_vp8={
//	TYPE( PAYLOAD_VIDEO),
//	CLOCK_RATE(90000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(256000),
//	MIME_TYPE ("VP8"),
//	CHANNELS(0)
//};
//PayloadType payload_type_h264={
//	TYPE( PAYLOAD_VIDEO),
//	CLOCK_RATE(90000),
//	BITS_PER_SAMPLE(0),
//	ZERO_PATTERN(NULL),
//	PATTERN_LENGTH(0),
//	NORMAL_BITRATE(256000),
//	MIME_TYPE ("H264"),
//	CHANNELS(0)
//};
PayloadType	ccp_payload_type_telephone_event={
	PAYLOAD_AUDIO_PACKETIZED, /*type */
	8000,	/*clock rate */
	0,		/* bytes per sample N/A */
	NULL,	/* zero pattern N/A*/
	0,		/*pattern_length N/A */
	0,		/*	normal_bitrate */
	"telephone-event",	/* MIME subtype */
	1,		/* Audio Channels */
    NULL,
    NULL,
	0,		/*flags */
    NULL
};

PayloadType	payload_type_cn8k={
	PAYLOAD_AUDIO_PACKETIZED, /*type */
	8000,	/*clock rate */
	0,		/* bytes per sample N/A */
	NULL,	/* zero pattern N/A*/
	0,		/*pattern_length N/A */
	0,		/*	normal_bitrate */
	"CN",	/* MIME subtype */
	1,		/* Audio Channels */
    NULL,
    NULL,
	0,		/*flags */
    NULL
};

PayloadType	payload_type_red8k={
    PAYLOAD_AUDIO_PACKETIZED, /*type */
    8000,	/*clock rate */
    0,		/* bytes per sample N/A */
    NULL,	/* zero pattern N/A*/
    0,		/*pattern_length N/A */
    0,		/*	normal_bitrate */
    "red",	/* MIME subtype */
    1,		/* Audio Channels */
    NULL,
    NULL,
    0,		/*flags */
    NULL
};

//char *payload_type_get_rtpmap(PayloadType *pt)
//{
//	int len=(int)strlen(pt->mime_type)+15;
//	char *rtpmap=(char *) malloc(len);
//	if (pt->channels>0)
//		snprintf(rtpmap,len,"%s/%i/%i",pt->mime_type,pt->clock_rate,pt->channels);
//	else
//		 snprintf(rtpmap,len,"%s/%i",pt->mime_type,pt->clock_rate);
//	return rtpmap;
//}
//
//PayloadType *payload_type_new()
//{
////	PayloadType *newpayload=(PayloadType *)ms_new0(PayloadType,1);  //ms_new0
//	PayloadType *newpayload=(PayloadType *)malloc(sizeof(PayloadType)*1);  //ms_new0
//	memset((void *)newpayload,0,sizeof(PayloadType)*1);
//
//
//
//	newpayload->flags|=PAYLOAD_TYPE_ALLOCATED;
//	return newpayload;
//}
//
//
//PayloadType *payload_type_clone(PayloadType *payload)
//{
////	PayloadType *newpayload=(PayloadType *)ms_new0(PayloadType,1);  //ms_new0
//	PayloadType *newpayload=(PayloadType *)malloc(sizeof(PayloadType)*1);  //ms_new0
//	memset((void *)newpayload,0,sizeof(PayloadType)*1);
//
//
//
//	memcpy(newpayload,payload,sizeof(PayloadType));
//	newpayload->mime_type=ser_strdup(payload->mime_type);
//
//	if (payload->recv_fmtp!=NULL)
//		newpayload->recv_fmtp=ser_strdup(payload->recv_fmtp);
//	if (payload->send_fmtp!=NULL)
//		newpayload->send_fmtp=ser_strdup(payload->send_fmtp);
//
//	newpayload->flags|=PAYLOAD_TYPE_ALLOCATED;
//	return newpayload;
//}
//
//static bool_t canWrite(PayloadType *pt){
//	if (!(pt->flags & PAYLOAD_TYPE_ALLOCATED)) {
//		PrintConsole("Cannot change parameters of statically defined payload types: make your"
//			" own copy using payload_type_clone() first.");
//		return FALSE;
//	}
//	return TRUE;
//}
//
///**
// * Sets a recv parameters (fmtp) for the PayloadType.
// * This method is provided for applications using RTP with SDP, but
// * actually the ftmp information is not used for RTP processing.
//**/
//void payload_type_set_recv_fmtp(PayloadType *pt, const char *fmtp){
//	if (canWrite(pt)){
//		if (pt->recv_fmtp!=NULL) ser_free(pt->recv_fmtp);
//		if (fmtp!=NULL)
//			pt->recv_fmtp=ser_strdup(fmtp);
//		else pt->recv_fmtp=NULL;
//	}
//}
//
///**
// * Sets a send parameters (fmtp) for the PayloadType.
// * This method is provided for applications using RTP with SDP, but
// * actually the ftmp information is not used for RTP processing.
//**/
//void payload_type_set_send_fmtp(PayloadType *pt, const char *fmtp){
//	if (canWrite(pt)){
//		if (pt->send_fmtp!=NULL) ser_free(pt->send_fmtp);
//		if (fmtp!=NULL){
//			pt->send_fmtp=ser_strdup(fmtp);
//		}
//		else pt->send_fmtp=NULL;
//	}
//}
//
//
//
//void payload_type_append_recv_fmtp(PayloadType *pt, const char *fmtp){
//	if (canWrite(pt)){
//		if (pt->recv_fmtp==NULL)
//			pt->recv_fmtp=ser_strdup(fmtp);
//		else{
//			char *tmp=ser_strdup_printf("%s;%s",pt->recv_fmtp,fmtp);
//			ser_free(pt->recv_fmtp);
//			pt->recv_fmtp=tmp;
//		}
//	}
//}
//
//void payload_type_append_send_fmtp(PayloadType *pt, const char *fmtp){
//	if (canWrite(pt)){
//		if (pt->send_fmtp==NULL)
//			pt->send_fmtp=ser_strdup(fmtp);
//		else{
//			char *tmp=ser_strdup_printf("%s;%s",pt->send_fmtp,fmtp);
//			ser_free(pt->send_fmtp);
//			pt->send_fmtp=tmp;
//		}
//	}
//}
//
bool payload_type_enabled(const PayloadType *pt)
{
	return (((pt)->flags & PAYLOAD_TYPE_ENABLED)!=0);
}
//
///**
// * Frees a PayloadType.
//**/
//void payload_type_destroy(PayloadType *pt)
//{
//	if (pt->mime_type) ser_free(pt->mime_type);
//	if (pt->recv_fmtp) ser_free(pt->recv_fmtp);
//	if (pt->send_fmtp) ser_free(pt->send_fmtp);
//	ser_free(pt);
//}

/**
 * Parses a fmtp string such as "profile=0;level=10", finds the value matching
 * parameter param_name, and writes it into result.
 * Despite fmtp strings are not used anywhere within oRTP, this function can
 * be useful for people using RTP streams described from SDP.
 * @param fmtp the fmtp line (format parameters)
 * @param param_name the parameter to search for
 * @param result the value given for the parameter (if found)
 * @param result_len the size allocated to hold the result string
 * @return TRUE if the parameter was found, else FALSE.
**/
//bool_t fmtp_get_value(const char *fmtp, const char *param_name, char *result, size_t result_len){
//	const char *pos=strstr(fmtp,param_name);
//	memset(result, '\0', result_len);
//	if (pos){
//		const char *equal=strchr(pos,'=');
//		if (equal){
//			int copied;
//			const char *end=strchr(equal+1,';');
//			if (end==NULL) end=fmtp+strlen(fmtp); /*assuming this is the last param */
//			copied=MIN(result_len-1,end-(equal+1));
//			strncpy(result,equal+1,copied);
//			result[copied]='\0';
//			return TRUE;
//		}
//	}
//	return FALSE;
//}
//
//int rtp_profile_get_payload_number_from_mime(RtpProfile *profile,const char *mime)
//{
//	PayloadType *pt;
//	int i;
//	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++)
//	{
//		pt=rtp_profile_get_payload(profile,i);
//		if (pt!=NULL)
//		{
//			if (strcasecmp(pt->mime_type,mime)==0){
//				return i;
//			}
//		}
//	}
//	return -1;
//}
//
//int rtp_profile_find_payload_number(RtpProfile*profile,const char *mime,int rate,int channels)
//{
//	int i;
//	PayloadType *pt;
//	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++)
//	{
//		pt=rtp_profile_get_payload(profile,i);
//		if (pt!=NULL)
//		{
//			if (strcasecmp(pt->mime_type,mime)==0 &&
//			    pt->clock_rate==rate &&
//			    (pt->channels==channels || channels<=0 || pt->channels<=0)) {
//				/*we don't look at channels if it is undefined
//				ie a negative or zero value*/
//
//				return i;
//			}
//		}
//	}
//	return -1;
//}
//
//int rtp_profile_get_payload_number_from_rtpmap(RtpProfile *profile,const char *rtpmap)
//{
//	int clock_rate, channels, ret;
//	char* subtype = ser_strdup( rtpmap );
//	char* rate_str = NULL;
//	char* chan_str = NULL;
//
//
//	/* find the slash after the subtype */
//	rate_str = strchr(subtype, '/');
//	if (rate_str && strlen(rate_str)>1) {
//		*rate_str = 0;
//		rate_str++;
//
//		/* Look for another slash */
//		chan_str = strchr(rate_str, '/');
//		if (chan_str && strlen(chan_str)>1) {
//			*chan_str = 0;
//			chan_str++;
//		} else {
//			chan_str = NULL;
//		}
//	} else {
//		rate_str = NULL;
//	}
//
//	// Use default clock rate if none given
//	if (rate_str) clock_rate = atoi(rate_str);
//	else clock_rate = 8000;
//
//	// Use default number of channels if none given
//	if (chan_str) channels = atoi(chan_str);
//	else channels = -1;
//
//	//printf("Searching for payload %s at freq %i with %i channels\n",subtype,clock_rate,ch1annels);
//	ret=rtp_profile_find_payload_number(profile,subtype,clock_rate,channels);
//	ser_free(subtype);
//	return ret;
//}
//
//PayloadType * rtp_profile_find_payload(RtpProfile *prof,const char *mime,int rate,int channels)
//{
//	int i;
//	i=rtp_profile_find_payload_number(prof,mime,rate,channels);
//	if (i>=0) return rtp_profile_get_payload(prof,i);
//	return NULL;
//}
//
//
//PayloadType * rtp_profile_get_payload_from_mime(RtpProfile *profile,const char *mime)
//{
//	int pt;
//	pt=rtp_profile_get_payload_number_from_mime(profile,mime);
//	if (pt==-1) return NULL;
//	else return rtp_profile_get_payload(profile,pt);
//}

//
//PayloadType * rtp_profile_get_payload_from_rtpmap(RtpProfile *profile, const char *rtpmap)
//{
//	int pt = rtp_profile_get_payload_number_from_rtpmap(profile,rtpmap);
//	if (pt==-1) return NULL;
//	else return rtp_profile_get_payload(profile,pt);
//}
//
//int rtp_profile_move_payload(RtpProfile *prof,int oldpos,int newpos){
//	prof->payload[newpos]=prof->payload[oldpos];
//	prof->payload[oldpos]=NULL;
//	return 0;
//}
//
//RtpProfile * rtp_profile_new(const char *name)
//{
////	RtpProfile *prof=(RtpProfile*)ms_new0(RtpProfile,1);  //ms_new0
//	RtpProfile *prof=(RtpProfile*)malloc(sizeof(RtpProfile)*1);  //ms_new0
//	memset((void *)prof,0,sizeof(RtpProfile)*1);
//
//
//
//	rtp_profile_set_name(prof,name);
//	return prof;
//}

///**
// *	Assign payload type number index to payload type desribed in pt for the RTP profile profile.
// * @param profile a RTP profile
// * @param idx the payload type number
// * @param pt the payload type description
// *
//**/
//void rtp_profile_set_payload(RtpProfile *prof, int idx, PayloadType *pt){
//	if (idx<0 || idx>=RTP_PROFILE_MAX_PAYLOADS) {
//		PrintConsole("Bad index %i\n",idx);
//		return;
//	}
//	prof->payload[idx]=pt;
//}

///**
// * Initialize the profile to the empty profile (all payload type are unassigned).
// *@param profile a RTP profile
// *
//**/
//void rtp_profile_clear_all(RtpProfile *obj){
//	int i;
//	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++){
//		obj->payload[i]=0;
//	}
//}
//
//void av_profile_init(RtpProfile *profile)
//{
//	rtp_profile_clear_all(profile);
//	profile->name="AV profile";
////	rtp_profile_set_payload(profile,0,&payload_type_pcmu8000);
//}


///**
// * Set a name to the rtp profile. (This is not required)
// * @param profile a rtp profile object
// * @param nm a string
// *
//**/
//void rtp_profile_set_name(RtpProfile *obj, const char *name){
//	if (obj->name!=NULL) ser_free(obj->name);
//	obj->name=ser_strdup(name);
//}
//
///* ! payload are not cloned*/
//RtpProfile * rtp_profile_clone(RtpProfile *prof)
//{
//	int i;
//	PayloadType *pt;
//	RtpProfile *newprof=rtp_profile_new(prof->name);
//	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++){
//		pt=rtp_profile_get_payload(prof,i);
//		if (pt!=NULL){
//			rtp_profile_set_payload(newprof,i,pt);
//		}
//	}
//	return newprof;
//}
//
//
///*clone a profile and its payloads */
//RtpProfile * rtp_profile_clone_full(RtpProfile *prof)
//{
//	int i;
//	PayloadType *pt;
//	RtpProfile *newprof=rtp_profile_new(prof->name);
//	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++){
//		pt=rtp_profile_get_payload(prof,i);
//		if (pt!=NULL){
//			rtp_profile_set_payload(newprof,i,payload_type_clone(pt));
//		}
//	}
//	return newprof;
//}
//
//void rtp_profile_destroy(RtpProfile *prof)
//{
//	int i;
//	PayloadType *payload;
//	if (prof->name) {
//		ser_free(prof->name);
//		prof->name = NULL;
//	}
//	for (i=0;i<RTP_PROFILE_MAX_PAYLOADS;i++)
//	{
//		payload=rtp_profile_get_payload(prof,i);
//		if (payload!=NULL && (payload->flags & PAYLOAD_TYPE_ALLOCATED))
//			payload_type_destroy(payload);
//	}
//	ser_free(prof);
//}

bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret){
#if !defined(_WIN32_WCE)
	FILE *f=popen(command,"r");
	if (f!=NULL){
		int err;
		*result=(char *)malloc(4096);
		err=fread(*result,1,4096-1,f);
		if (err<0){
			PrintConsole("Error reading command output:%s\n",strerror(errno));
			ms_free((void **)result);
			return FALSE;
		}
		(*result)[err]=0;
		err=pclose(f);
		if (command_ret!=NULL) *command_ret=err;
		return TRUE;
	}
#endif /*_WIN32_WCE*/
	return FALSE;
}

int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen)
{
	struct addrinfo hints,*res=NULL;
	int ret;
	const char *port;
	char host[NI_MAXHOST];
	char *p;
	host[NI_MAXHOST-1]='\0';
	strncpy(host,server,sizeof(host)-1);
	p=strchr(host,':');
	if (p) {
		*p='\0';
		port=p+1;
	}else port="3478";
	memset(&hints,0,sizeof(hints));
	hints.ai_family=PF_INET;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_protocol=IPPROTO_UDP;
	ret=getaddrinfo(host,port,&hints,&res);
	if (ret!=0){
		PrintConsole("getaddrinfo() failed for %s:%s : %s\n",host,port,gai_strerror(ret));
		return -1;
	}
	if (!res) return -1;
	memcpy(ss,res->ai_addr,res->ai_addrlen);
	*socklen=res->ai_addrlen;
	freeaddrinfo(res);
	return 0;
}

//#ifdef WIN32
//int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
//{
//    struct hostent *h = NULL;
//    struct addrinfo *ai;
//    struct sockaddr_in *sin;
//
//#if HAVE_WINSOCK2_H
//    int (WSAAPI *win_getaddrinfo)(const char *node, const char *service,
//                                  const struct addrinfo *hints,
//                                  struct addrinfo **res);
//    HMODULE ws2mod = GetModuleHandle("ws2_32.dll");
//    win_getaddrinfo = GetProcAddress(ws2mod, "getaddrinfo");
//    if (win_getaddrinfo)
//        return win_getaddrinfo(node, service, hints, res);
//#endif
//
//    *res = NULL;
//    sin = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));   //ser_malloc0
//	memset((struct sockaddr_in *)sin,0,sizeof(struct sockaddr_in));
//
//    if (!sin)
//        return EAI_FAIL;
//    sin->sin_family = AF_INET;
//
//    if (node) {
//        if (!ff_inet_aton(node, &sin->sin_addr)) {
//            if (hints && (hints->ai_flags & AI_NUMERICHOST)) {
//                ser_free(sin);
//                return EAI_FAIL;
//            }
//            h = gethostbyname(node);
//            if (!h) {
//                ser_free(sin);
//                return EAI_FAIL;
//            }
//            memcpy(&sin->sin_addr, h->h_addr_list[0], sizeof(struct in_addr));
//        }
//    } else {
//        if (hints && (hints->ai_flags & AI_PASSIVE)) {
//            sin->sin_addr.s_addr = INADDR_ANY;
//        } else
//            sin->sin_addr.s_addr = INADDR_LOOPBACK;
//    }
//
//    /* Note: getaddrinfo allows service to be a string, which
//     * should be looked up using getservbyname. */
//    if (service)
//        sin->sin_port = htons(atoi(service));
//
//    ai = (struct addrinfo*)malloc(sizeof(struct addrinfo));  //ser_malloc0
//	memset((void *)ai,0,sizeof(struct addrinfo));
//
//    if (!ai) {
//        ser_free(sin);
//        return EAI_FAIL;
//    }
//
//    *res = ai;
//    ai->ai_family = AF_INET;
//    ai->ai_socktype = hints ? hints->ai_socktype : 0;
//    switch (ai->ai_socktype) {
//    case SOCK_STREAM: ai->ai_protocol = IPPROTO_TCP; break;
//    case SOCK_DGRAM:  ai->ai_protocol = IPPROTO_UDP; break;
//    default:          ai->ai_protocol = 0;           break;
//    }
//
//    ai->ai_addr = (struct sockaddr *)sin;
//    ai->ai_addrlen = sizeof(struct sockaddr_in);
//    if (hints && (hints->ai_flags & AI_CANONNAME)){
//        ai->ai_canonname = h ? ser_strdup(h->h_name) : NULL;
//	}
//    ai->ai_next = NULL;
//    return 0;
//}
//#endif
//
//#if !defined(HAVE_INET_ATON)
//
//int ff_inet_aton (const char * str, struct in_addr * add)
//{
//    unsigned int add1 = 0, add2 = 0, add3 = 0, add4 = 0;
//
//    if (sscanf(str, "%d.%d.%d.%d", &add1, &add2, &add3, &add4) != 4)
//        return 0;
//
//    if (!add1 || (add1|add2|add3|add4) > 255) return 0;
//
//    add->s_addr = htonl((add1 << 24) + (add2 << 16) + (add3 << 8) + add4);
//
//    return 1;
//}
//#else
//int ff_inet_aton (const char * str, struct in_addr * add)
//{
//    return inet_aton(str, add);
//}
//#endif /* !HAVE_INET_ATON */
//
//#ifdef WIN32
//void freeaddrinfo(struct addrinfo *res)
//{
//#if HAVE_WINSOCK2_H
//    void (WSAAPI *win_freeaddrinfo)(struct addrinfo *res);
//    HMODULE ws2mod = GetModuleHandle("ws2_32.dll");
//    win_freeaddrinfo = (void (WSAAPI *)(struct addrinfo *res))
//                       GetProcAddress(ws2mod, "freeaddrinfo");
//    if (win_freeaddrinfo) {
//        win_freeaddrinfo(res);
//        return;
//    }
//#endif
//
//    ser_free(res->ai_canonname);
//    ser_free(res->ai_addr);
//    ser_free(res);
//}
//
//
//int getnameinfo(const struct sockaddr *sa, int salen,char *host, int hostlen,
//                   char *serv, int servlen, int flags)
//{
//    const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
//
//#if HAVE_WINSOCK2_H
//    int (WSAAPI *win_getnameinfo)(const struct sockaddr *sa, socklen_t salen,
//                                  char *host, DWORD hostlen,
//                                  char *serv, DWORD servlen, int flags);
//    HMODULE ws2mod = GetModuleHandle("ws2_32.dll");
//    win_getnameinfo = GetProcAddress(ws2mod, "getnameinfo");
//    if (win_getnameinfo)
//        return win_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
//#endif
//
//    if (sa->sa_family != AF_INET)
//        return EAI_FAMILY;
//    if (!host && !serv)
//        return EAI_NONAME;
//
//    if (host && hostlen > 0) {
//        struct hostent *ent = NULL;
//        unsigned int a;
//        if (!(flags & NI_NUMERICHOST))
//            ent = gethostbyaddr((const char *)&sin->sin_addr,
//                                sizeof(sin->sin_addr), AF_INET);
//
//        if (ent) {
//            snprintf(host, hostlen, "%s", ent->h_name);
//        } else if (flags & NI_NAMERQD) {
//            return EAI_NONAME;
//        } else {
//            a = ntohl(sin->sin_addr.s_addr);
//            snprintf(host, hostlen, "%d.%d.%d.%d",
//                     ((a >> 24) & 0xff), ((a >> 16) & 0xff),
//                     ((a >>  8) & 0xff), ( a        & 0xff));
//        }
//    }
//
//    if (serv && servlen > 0) {
//        struct servent *ent = NULL;
//        if (!(flags & NI_NUMERICSERV))
//            ent = getservbyport(sin->sin_port, flags & NI_DGRAM ? "udp" : "tcp");
//
//        if (ent) {
//            snprintf(serv, servlen, "%s", ent->s_name);
//        } else
//            snprintf(serv, servlen, "%d", ntohs(sin->sin_port));
//    }
//
//    return 0;
//}
//
//const char *gai_strerror(int ecode)
//{
//    switch(ecode) {
//    case EAI_FAIL   : return "A non-recoverable error occurred";
//    case EAI_FAMILY : return "The address family was not recognized or the address length was invalid for the specified family";
//    case EAI_NONAME : return "The name does not resolve for the supplied parameters";
//    }
//
//    return "Unknown error";
//}
//#endif

/*
 * this method is an utility method that calls fnctl() on UNIX or
 * ioctlsocket on Win32.
 * int retrun the result of the system method
 */
int ccp_set_non_blocking_socket (ortp_socket_t sock)
{
#if	!defined(_WIN32) && !defined(_WIN32_WCE)
	return fcntl (sock, F_SETFL, O_NONBLOCK);
#else
	unsigned long nonBlock = 1;
	return ioctlsocket(sock, FIONBIO , &nonBlock);
#endif
}

ortp_socket_t create_socket(int local_port)
{
	struct sockaddr_in laddr;
	ortp_socket_t sock;
	int optval;
	sock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock<0) {
		PrintConsole("Fail to create socket\n");
		return -1;
	}
	memset (&laddr,0,sizeof(laddr));
	laddr.sin_family=AF_INET;
	laddr.sin_addr.s_addr=INADDR_ANY;
	laddr.sin_port=htons(local_port);
	if (bind(sock,(struct sockaddr*)&laddr,sizeof(laddr))<0){
		PrintConsole("Bind socket to 0.0.0.0:%i failed: %s\n",local_port,getSocketError());
		ccp_close_socket(sock);
		return -1;
	}
	optval=1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
				(SOCKET_OPTION_VALUE)&optval, sizeof (optval))<0){
		PrintConsole("Fail to set SO_REUSEADDR\n");
	}
	ccp_set_non_blocking_socket(sock);
	return sock;
}

/*
 * this method is an utility method that calls close() on UNIX or
 * closesocket on Win32.
 * int retrun the result of the system method
 */
int ccp_close_socket(ortp_socket_t sock)
{
#if	!defined(_WIN32) && !defined(_WIN32_WCE)
	return close (sock);
#else
	return closesocket(sock);
#endif
}

/////////////////////////////////////////////////////

static int get_local_ip_for_with_connect(int type, const char *dest, char *result)
{
	int err,tmp;
	struct addrinfo hints;
	struct addrinfo *res=NULL;
	struct sockaddr_storage addr;
	struct sockaddr *p_addr=(struct sockaddr*)&addr;
	ortp_socket_t sock;
	socklen_t s;

	memset(&hints,0,sizeof(hints));
	hints.ai_family=(type==AF_INET6) ? PF_INET6 : PF_INET;
	hints.ai_socktype=SOCK_DGRAM;
	/*hints.ai_flags=AI_NUMERICHOST|AI_CANONNAME;*/
	err=getaddrinfo(dest,"5060",&hints,&res);
	if (err!=0){
		PrintConsole("getaddrinfo() error: %s\n",gai_strerror(err));
		return -1;
	}
	if (res==NULL){
		PrintConsole("bug: getaddrinfo returned nothing.\n");
		return -1;
	}
	sock=socket(res->ai_family,SOCK_DGRAM,0);
	tmp=1;
	err=setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(SOCKET_OPTION_VALUE)&tmp,sizeof(int));
	if (err<0){
		PrintConsole("Error in setsockopt: %s\n",strerror(errno));
	}
	err=connect(sock,res->ai_addr,res->ai_addrlen);
	if (err<0) {
		PrintConsole("Error in connect: %s\n",strerror(errno));
 		freeaddrinfo(res);
 		ccp_close_socket(sock);
		return -1;
	}
	freeaddrinfo(res);
	res=NULL;
	s=sizeof(addr);
	err=getsockname(sock,(struct sockaddr*)&addr,&s);
	if (err!=0) {
		PrintConsole("Error in getsockname: %s\n",strerror(errno));
		ccp_close_socket(sock);
		return -1;
	}
	if (p_addr->sa_family==AF_INET){
		struct sockaddr_in *p_sin=(struct sockaddr_in*)p_addr;
		if (p_sin->sin_addr.s_addr==0){
			ccp_close_socket(sock);
			return -1;
		}
	}
	err=getnameinfo((struct sockaddr *)&addr,s,result,SERPHONE_IPADDR_SIZE,NULL,0,NI_NUMERICHOST);
	if (err!=0){
		PrintConsole("getnameinfo error: %s\n",strerror(errno));
	}
	ccp_close_socket(sock);
	PrintConsole("Local interface to reach %s is %s.\n",dest,result);
	return 0;
}

int serphone_core_get_local_ip_for(int type, const char *dest, char *result)
{
	strcpy(result,type==AF_INET ? "127.0.0.1" : "::1");
#ifdef HAVE_GETIFADDRS
	if (dest==NULL) {
		/*we use getifaddrs for lookup of default interface */
		int found_ifs;

		found_ifs=get_local_ip_with_getifaddrs(type,result,LINPHONE_IPADDR_SIZE);
		if (found_ifs==1){
			return 0;
		}else if (found_ifs<=0){
			/*absolutely no network on this machine */
			return -1;
		}
	}
#endif
	/*else use connect to find the best local ip address */
	if (type==AF_INET)
		dest="87.98.157.38"; /*a public IP address*/
	else dest="2a00:1450:8002::68";
	return get_local_ip_for_with_connect(type,dest,result);
}

PayloadType * find_payload(RtpProfile *prof, const char *mime_type, int clock_rate, const char *recv_fmtp)
{
	PayloadType *candidate=NULL;
	int i;
	PayloadType *it;
	for(i=0;i<127;++i){
		it=rtp_profile_get_payload(prof,i);
		if (it!=NULL && strcasecmp(mime_type,it->mime_type)==0
			&& (clock_rate==it->clock_rate || clock_rate<=0) ){
			if ( (recv_fmtp && it->recv_fmtp && strstr(recv_fmtp,it->recv_fmtp)!=NULL) ||
				(recv_fmtp==NULL && it->recv_fmtp==NULL) ){
				/*exact match*/
				if (recv_fmtp) payload_type_set_recv_fmtp(it,recv_fmtp);
				return it;
			}else {
				if (candidate){
					if (it->recv_fmtp==NULL) candidate=it;
				}else candidate=it;
			}
		}
	}
	if (candidate && recv_fmtp){
		payload_type_set_recv_fmtp(candidate,recv_fmtp);
	}
	return candidate;
}

#if defined (_WIN32_WCE) || defined(_MSC_VER)
int ccp_ortp_file_exist(const char *pathname) {
	FILE* fd;
	if (pathname==NULL) return -1;
	fd=fopen(pathname,"r");
	if (fd==NULL) {
		return -1;
	} else {
		fclose(fd);
		return 0;
	}
}
#else
int ccp_ortp_file_exist(const char *pathname) {
	return access(pathname,F_OK);
}
#endif /*_WIN32_WCE*/
