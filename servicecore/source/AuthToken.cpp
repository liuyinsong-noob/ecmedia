#include "AuthToken.h"
#include "base64.h"
#include <stdlib.h> 
#include <stdio.h> 
#include "string.h"

char GetValue(const char* value,  char *tag)
{
	if(tag && value && (strlen(tag) < 1024))
	{
		char sKey[1024] = {0};
		sprintf(sKey, "\"%s\":\"", tag);
		
		int key_len = 0;
		key_len = strlen(sKey);
		
		const char * pStart = NULL;		
		pStart = strstr(value, sKey);
		if( !pStart )
			return 2;
		
		char  tag[32] = {0};
		strncpy(tag, pStart + key_len, 1);
		
		return tag[0];	
	}
	else
	{
		return 2;
	}
	
}
int  DecodeAuthToken(const char * token, const char* key, char * output)
{
	if(token && key && output)
	{	
		if(strlen(token) > MAX_AUTHTOKEN_LEN)
		{
			//bad token
			return -1;
		}
		
		char strBase64Buffer[MAX_AUTHTOKEN_LEN] = {0};
		int  nBase64DecodedLen = Base64decode(strBase64Buffer,(char*)token);	
		
		for(int i=0; i<KEY_COUNT; i++)
		{
			output[i] = GetValue(strBase64Buffer, (char *)KEY_TOKEN[i]);
		}

		return 0;
	}
	else
	{
		return -1;
	}
};

int  EncodeAuthToken(const char * token, char *output)
{
	if(token && output)
	{
		char  sRes[MAX_AUTHTOKEN_LEN] = {0};
		strcpy(sRes, "{");
		for(int i=0; i<KEY_COUNT; i++)
		{
			char sTmp[1024] = {0};
			if(i == KEY_COUNT - 1)
			{
				sprintf(sTmp, "\"%s\":\"%c\"", KEY_TOKEN[i], token[i]);
			}
			else
			{	
				sprintf(sTmp, "\"%s\":\"%c\", ", KEY_TOKEN[i], token[i]);
			}

			strcat(sRes, sTmp);
		}

		strcat(sRes, "}");
			
		Base64encode(output,sRes,strlen(sRes));
		return 0;
	}
	else
	{
		return -1;
	}
}

