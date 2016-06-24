#include "Des.h"
#include <memory.h>

void Des::deskey(unsigned char *key, short edf)
{
	register int i, j, l, m, n;
	unsigned char pc1m[56], pcr[56];
	unsigned int kn[32];

	for ( j = 0; j < 56; j++ ) 
	{
		l = pc1[j];
		m = l & 07;
		pc1m[j] = (key[l >> 3] & bytebit[m]) ? 1 : 0;
	}
	for( i = 0; i < 16; i++ ) 
	{
		if( edf == DE1 ) 
			m = (15 - i) << 1;
		else             
			m = i << 1;
		n = m + 1;
		kn[m] = kn[n] = 0L;
		for( j = 0; j < 28; j++ ) 
		{
			l = j + totrot[i];
			if( l < 28 ) 
				pcr[j] = pc1m[l];
			else 
				pcr[j] = pc1m[l - 28];
		}
		for( j = 28; j < 56; j++ ) 
		{
			l = j + totrot[i];
			if( l < 56 ) 
				pcr[j] = pc1m[l];
			else         
				pcr[j] = pc1m[l - 28];
		}
		for( j = 0; j < 24; j++ ) 
		{
			if( pcr[pc2[j]] )    
				kn[m] |= bigbyte[j];
			if( pcr[pc2[j+24]] ) 
				kn[n] |= bigbyte[j];
		}
	}
	cookey(kn);
};

void Des::cookey(unsigned int *raw1)
{
	register unsigned int *cook, *raw0;
	unsigned int dough[32];
	register int i;

	cook = dough;
	for( i = 0; i < 16; i++, raw1++ ) 
	{
		raw0 = raw1++;
		*cook   = (*raw0 & 0x00fc0000L) << 6;
		*cook  |= (*raw0 & 0x00000fc0L) << 10;
		*cook  |= (*raw1 & 0x00fc0000L) >> 10;
		*cook++|= (*raw1 & 0x00000fc0L) >> 6;
		*cook   = (*raw0 & 0x0003f000L) << 12;
		*cook  |= (*raw0 & 0x0000003fL) << 16;
		*cook  |= (*raw1 & 0x0003f000L) >> 4;
		*cook++       |= (*raw1 & 0x0000003fL);
	}
	usekey(dough);	
};

void Des::cpkey(unsigned int *into)
{
	register unsigned int *from, *endp;

  	from = KnL, endp = &KnL[32];
  	while( from < endp ) 
		*into++ = *from++;
};

void Des::usekey(unsigned int *from)
{
	register unsigned int *to, *endp;

  	to = KnL, endp = &KnL[32];
  	while( to < endp ) 
		*to++ = *from++;
};

void Des::scrunch(unsigned char *outof, unsigned int *into)
{
	*into   = (*outof++ & 0xffL) << 24;
	*into  |= (*outof++ & 0xffL) << 16;
	*into  |= (*outof++ & 0xffL) << 8;
	*into++ |= (*outof++ & 0xffL);
	*into   = (*outof++ & 0xffL) << 24;
	*into  |= (*outof++ & 0xffL) << 16;
	*into  |= (*outof++ & 0xffL) << 8;
	*into  |= (*outof   & 0xffL);	
};

void Des::unscrun(unsigned int *outof, unsigned char *into)
{
	*into++ = (*outof >> 24) & 0xffL;
	*into++ = (*outof >> 16) & 0xffL;
	*into++ = (*outof >>  8) & 0xffL;
	*into++ =  *outof++      & 0xffL;
	*into++ = (*outof >> 24) & 0xffL;
	*into++ = (*outof >> 16) & 0xffL;
	*into++ = (*outof >>  8) & 0xffL;
	*into   =  *outof     & 0xffL;	
};

void Des::desfunc(unsigned int *block, unsigned int *keys)
{
	register unsigned int fval, work, right, leftt;
	register int round;

	leftt = block[0];
	right = block[1];
	work = ((leftt >> 4) ^ right) & 0x0f0f0f0fL;
	right ^= work;
	leftt ^= (work << 4);
	work = ((leftt >> 16) ^ right) & 0x0000ffffL;
	right ^= work;
	leftt ^= (work << 16);
	work = ((right >> 2) ^ leftt) & 0x33333333L;
	leftt ^= work;
	right ^= (work << 2);
	work = ((right >> 8) ^ leftt) & 0x00ff00ffL;
	leftt ^= work;
	right ^= (work << 8);
	right = ((right << 1) | ((right >> 31) & 1L)) & 0xffffffffL;
	work = (leftt ^ right) & 0xaaaaaaaaL;
	leftt ^= work;
	right ^= work;
	leftt = ((leftt << 1) | ((leftt >> 31) & 1L)) & 0xffffffffL;

	for( round = 0; round < 8; round++ ) 
	{
		work  = (right << 28) | (right >> 4);
		work ^= *keys++;
		fval  = SP7[ work             & 0x3fL];
		fval |= SP5[(work >>  8) & 0x3fL];
		fval |= SP3[(work >> 16) & 0x3fL];
		fval |= SP1[(work >> 24) & 0x3fL];
		work  = right ^ *keys++;
		fval |= SP8[ work             & 0x3fL];
		fval |= SP6[(work >>  8) & 0x3fL];
		fval |= SP4[(work >> 16) & 0x3fL];
		fval |= SP2[(work >> 24) & 0x3fL];
		leftt ^= fval;
		work  = (leftt << 28) | (leftt >> 4);
		work ^= *keys++;
		fval  = SP7[ work             & 0x3fL];
		fval |= SP5[(work >>  8) & 0x3fL];
		fval |= SP3[(work >> 16) & 0x3fL];
		fval |= SP1[(work >> 24) & 0x3fL];
		work  = leftt ^ *keys++;
		fval |= SP8[ work             & 0x3fL];
		fval |= SP6[(work >>  8) & 0x3fL];
		fval |= SP4[(work >> 16) & 0x3fL];
		fval |= SP2[(work >> 24) & 0x3fL];
		right ^= fval;
	}

	right = (right << 31) | (right >> 1);
	work = (leftt ^ right) & 0xaaaaaaaaL;
	leftt ^= work;
	right ^= work;
	leftt = (leftt << 31) | (leftt >> 1);
	work = ((leftt >> 8) ^ right) & 0x00ff00ffL;
	right ^= work;
	leftt ^= (work << 8);
	work = ((leftt >> 2) ^ right) & 0x33333333L;
	right ^= work;
	leftt ^= (work << 2);
	work = ((right >> 16) ^ leftt) & 0x0000ffffL;
	leftt ^= work;
	right ^= (work << 16);
	work = ((right >> 4) ^ leftt) & 0x0f0f0f0fL;
	leftt ^= work;
	right ^= (work << 4);
	*block++ = right;
	*block = leftt;	
};

/*********************************************************************** 
 * Validation sets:
 * Single-length key, single-length plaintext -
 * Key    : 0123 4567 89ab cdef
 * Plain  : 0123 4567 89ab cde7
 * Cipher : c957 4425 6a5e d31d
 *
 **********************************************************************/
void Des::des_key(des_ctx *dc, unsigned char *key)
{
	deskey(key,EN0);
	cpkey(dc->ek);
	deskey(key,DE1);
	cpkey(dc->dk);	
};

/* Encrypt several blocks in ECB mode.  Caller is responsible for
   short blocks. */
void Des::des_enc(des_ctx *dc, unsigned char *Data, int blocks)
{
	unsigned int work[2];
	int i;
	unsigned char *cp;

	cp = Data;
	for(i=0;i<blocks;i++)
	{
		scrunch(cp,work);
		desfunc(work,dc->ek);
		unscrun(work,cp);
		cp+=8;
	}	
};

void Des::des_dec(des_ctx *dc, unsigned char *Data, int blocks)
{
	unsigned int work[2];
	int i;
	unsigned char *cp;

	cp = Data;
	for(i=0;i<blocks;i++)
	{
		scrunch(cp,work);
		desfunc(work,dc->dk);
		unscrun(work,cp);
		cp+=8;
	}	
};

void Des::des_enc3(des_ctx *dc1, des_ctx *dc2, unsigned char *Data, int blocks)
{
	unsigned int work[2];
	int i;
	unsigned char *cp;

	cp = Data;
	for(i=0;i<blocks;i++)
	{
		scrunch(cp, work);
		desfunc(work, dc1->ek);
		desfunc(work, dc2->dk);
		desfunc(work, dc1->ek);
		unscrun(work, cp);
		cp += 8;
	}
};

void Des::des_dec3(des_ctx *dc1, des_ctx *dc2, unsigned char *Data, int blocks)
{
	unsigned int work[2];
	int i;
	unsigned char *cp;

	cp = Data;
	for(i=0;i<blocks;i++)
	{
		scrunch(cp, work);
		desfunc(work, dc1->dk);
		desfunc(work, dc2->ek);
		desfunc(work, dc1->dk);
		unscrun(work, cp);
		cp += 8;
	}
};

std::string Des_Encode(std::string key, std::string text)
{
	Des des;
	des_ctx ctx;
	des.des_key(&ctx, (unsigned char*)key.c_str());
	int blocknum = text.length()/8+1;
	int len = blocknum*8;
	
	unsigned char * data = new unsigned char[len];
	char* hexdata = new char[2*len+1];
	

	memset(data,8-text.length()%8,len);
	memcpy(data,text.c_str(),text.length());

	des.des_enc(&ctx,data,blocknum);
	
	for( int i =0 ; i<len ; i++)
		sprintf(hexdata+2*i,"%02x",((unsigned int)data[i])&0x00ff);
	hexdata[2*len]=0;
	
	std::string ret = hexdata;
	delete data;
	delete hexdata;
	return ret;
	
}
std::string Des_Decode(std::string key, std::string text)
{
	return "";

}