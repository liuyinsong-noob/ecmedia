/* enum lookup code */

#define   _CRTDBG_MAP_ALLOC 
#ifndef _WIN32_WCE
#include <errno.h>
#endif

#include <string.h>

#include "enum.h"
#include "ECMedia.h"
#define DNS_ANSWER_MAX_SIZE 2048


#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h> 
#include   <crtdbg.h>
#endif

static char *create_enum_domain(const char *number){
	int len=strlen(number);
	char *domain=(char *)malloc((len*2)+10);
	int i,j;
	
	for (i=0,j=len-1;j>=0;j--){
		domain[i]=number[j];
		i++;
		domain[i]='.';
		i++;
	}
	strcpy(&domain[i],"e164.arpa");
	return domain;
}


static bool_t is_a_number(const char *str){
	char *p=(char *)str;
	bool_t res=FALSE;
	bool_t space_found=FALSE;
	for(;;p++){
		switch(p[0]){
			case '9':
			case '8':
			case '7':
			case '6':
			case '5':
			case '4':
			case '3':
			case '2':
			case '1':
			case '0':
				res=TRUE;
				if (space_found) return FALSE; /* avoid splited numbers */
				break;
			case '\0':
				return res;
				break;
			case ' ':
				space_found=TRUE;
				break;
			default:
				return FALSE;
		}
	}
	return FALSE;
}
//4970072278724
bool_t is_enum(const char *sipaddress, char **enum_domain){
	char *p;
	p=(char *)strstr(sipaddress,"sip:");
	if (p==NULL) return FALSE; /* enum should look like sip:4369959250*/
	else p+=4;
	if (is_a_number(p)){
		if (enum_domain!=NULL){
			*enum_domain=create_enum_domain(p);
		}
		return TRUE;
	}
	return FALSE;
}



int enum_lookup(const char *enum_domain, enum_lookup_res_t **res){
	int err;
	//char dns_answer[DNS_ANSWER_MAX_SIZE];
	char *begin,*end;
	char *host_result, *command;
	int i;
	bool_t forkok;
	/*
	ns_msg handle;
	int count;
	
	memset(&handle,0,sizeof(handle));
	*res=NULL;
	ms_message("Resolving %s...",enum_domain);
	
	err=res_search(enum_domain,ns_c_in,ns_t_naptr,dns_answer,DNS_ANSWER_MAX_SIZE);
	if (err<0){
		ms_warning("Error resolving enum:",herror(h_errno));
		return -1;
	}
	ns_initparse(dns_answer,DNS_ANSWER_MAX_SIZE,&handle);
	count=ns_msg_count(handle,ns_s_an);
	
	for(i=0;i<count;i++){
		ns_rr rr;
		memset(&rr,0,sizeof(rr));
		ns_parserr(&handle,ns_s_an,i,&rr);
		ms_message("data=%s",ns_rr_rdata(rr));
	}
	*/
	command=ms_strdup_printf("host -t naptr %s",enum_domain);
	forkok=lp_spawn_command_line_sync(command,&host_result,&err);
	ms_free((void **)&command);
    command = NULL;
	if (forkok){
		if (err!=0){
			PrintConsole("Host exited with %i error status.\n",err);
			return -1;
		}
	}else{
		PrintConsole("Could not spawn the \'host\' command.\n");
		return -1;
	}		
	PrintConsole("Answer received from dns (err=%i): %s\n",err,host_result);
	
	begin=strstr(host_result,"sip:");
	if (begin==0) {
		PrintConsole("No sip address found in dns naptr answer.\n");
		return -1;
	}
	*res=(enum_lookup_res_t *)malloc(sizeof(enum_lookup_res_t));  //ms_malloc0
	memset((void *)(*res),0,sizeof(enum_lookup_res_t));


	err=0;
	for(i=0;i<MAX_ENUM_LOOKUP_RESULTS;i++){
		end=strstr(begin,"!");
		if (end==NULL) goto parse_error;
		end[0]='\0';
		(*res)->sip_address[i]=ms_strdup(begin);
		err++;
		begin=strstr(end+1,"sip:");
		if (begin==NULL) break;
	}
	ms_free((void **)&host_result);
    host_result = NULL;
	return err;

	parse_error:
		ms_free((void **)res);
        *res = NULL;
		ms_free((void **)&host_result);
        host_result = NULL;
		*res=NULL;
		PrintConsole("Parse error in enum_lookup().\n");
		return -1;
}

void enum_lookup_res_free(enum_lookup_res_t *res){
	int i;
	for (i=0;i<MAX_ENUM_LOOKUP_RESULTS;i++){
		if (res->sip_address[i]!=NULL)
        {
            ms_free((void **)&res->sip_address[i]);
            res->sip_address[i] = NULL;
        }
	}
	ms_free((void **)&res);
}
