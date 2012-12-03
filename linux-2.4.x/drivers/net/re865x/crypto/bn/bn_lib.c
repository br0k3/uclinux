/* crypto/bn/bn_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#ifndef __KERNEL__
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../rtl_cryptGlue.h"
#else
#include "../../rtl865x/types.h"
#endif

#include "bn_lcl.h"

// Note: this may cause problem with standard C library. Just remove the following line.
#define INT_MAX 0x7fffffffL

const char *BN_version="Big Number";

/* For a 32 bit machine
 * 2 -   4 ==  128
 * 3 -   8 ==  256
 * 4 -  16 ==  512
 * 5 -  32 == 1024
 * 6 -  64 == 2048
 * 7 - 128 == 4096
 * 8 - 256 == 8192
 */
static int bn_limit_bits=0;
static int bn_limit_num=8;        /* (1<<bn_limit_bits) */
static int bn_limit_bits_low=0;
static int bn_limit_num_low=8;    /* (1<<bn_limit_bits_low) */
static int bn_limit_bits_high=0;
static int bn_limit_num_high=8;   /* (1<<bn_limit_bits_high) */
static int bn_limit_bits_mont=0;
static int bn_limit_num_mont=8;   /* (1<<bn_limit_bits_mont) */

void BN_set_params(int mult, int high, int low, int mont)
	{
	if (mult >= 0)
		{
		if (mult > (sizeof(int)*8)-1)
			mult=sizeof(int)*8-1;
		bn_limit_bits=mult;
		bn_limit_num=1<<mult;
		}
	if (high >= 0)
		{
		if (high > (sizeof(int)*8)-1)
			high=sizeof(int)*8-1;
		bn_limit_bits_high=high;
		bn_limit_num_high=1<<high;
		}
	if (low >= 0)
		{
		if (low > (sizeof(int)*8)-1)
			low=sizeof(int)*8-1;
		bn_limit_bits_low=low;
		bn_limit_num_low=1<<low;
		}
	if (mont >= 0)
		{
		if (mont > (sizeof(int)*8)-1)
			mont=sizeof(int)*8-1;
		bn_limit_bits_mont=mont;
		bn_limit_num_mont=1<<mont;
		}
	}

int BN_get_params(int which)
	{
	if      (which == 0) return(bn_limit_bits);
	else if (which == 1) return(bn_limit_bits_high);
	else if (which == 2) return(bn_limit_bits_low);
	else if (which == 3) return(bn_limit_bits_mont);
	else return(0);
	}

BIGNUM *BN_value_one(void)
	{
	static BN_ULONG data_one=1L;
	static BIGNUM const_one={&data_one,1,1,0};

	return(&const_one);
	}

char *BN_options(void)
	{
	static int init=0;
	static char data[16];

	if (!init)
		{
		init++;
		sprintf(data,"bn(%d,%d)",(int)sizeof(BN_ULONG)*8,
			(int)sizeof(BN_ULONG)*8);
		}
	return(data);
	}

int BN_num_bits_word(BN_ULONG l)
	{
	static const char bits[256]={
		0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		};

		{
		if (l & 0xffff0000L)
			{
			if (l & 0xff000000L)
				return(bits[(int)(l>>24L)]+24);
			else	return(bits[(int)(l>>16L)]+16);
			}
		else
			{
			if (l & 0xff00L)
				return(bits[(int)(l>>8)]+8);
			else	
				return(bits[(int)(l   )]  );
			}
		}
	}

int BN_num_bits(const BIGNUM *a)
	{
	BN_ULONG l;
	int i;

	bn_check_top(a);

	if (a->top == 0) return(0);
	l=a->d[a->top-1];
	assert(l != 0);
	i=(a->top-1)*BN_BITS2;
	return(i+BN_num_bits_word(l));
	}

void BN_clear_free(BIGNUM *a)
	{
	int i;

	if (a == NULL) return;
	if (a->d != NULL)
		{
		memset(a->d, 0, a->dmax*sizeof(a->d[0]));
		if (!(BN_get_flags(a,BN_FLG_STATIC_DATA)))
			rtlglue_free(a->d);
		}
	i=BN_get_flags(a,BN_FLG_MALLOCED);
	memset(a, 0, sizeof(BIGNUM));
	if (i)
		rtlglue_free(a);
	}

void BN_free(BIGNUM *a)
	{
	if (a == NULL) return;
	if ((a->d != NULL) && !(BN_get_flags(a,BN_FLG_STATIC_DATA)))
		rtlglue_free(a->d);
	a->flags|=BN_FLG_FREE; /* REMOVE? */
	if (a->flags & BN_FLG_MALLOCED)
		rtlglue_free(a);
	}

void BN_init(BIGNUM *a)
	{
	memset(a,0,sizeof(BIGNUM));
	}

BIGNUM *BN_new(void)
	{
	BIGNUM *ret;

	if ((ret=(BIGNUM *)rtlglue_malloc(sizeof(BIGNUM))) == NULL)
		return(NULL);
	ret->flags=BN_FLG_MALLOCED;
	ret->top=0;
	ret->neg=0;
	ret->dmax=0;
	ret->d=NULL;
	return(ret);
	}

/* This is an internal function that should not be used in applications.
 * It ensures that 'b' has enough room for a 'words' word number number.
 * It is mostly used by the various BIGNUM routines. If there is an error,
 * NULL is returned. If not, 'b' is returned. */

BIGNUM *bn_expand2(BIGNUM *b, int words)
	{
	BN_ULONG *A,*a;
	const BN_ULONG *B;
	int i;

	bn_check_top(b);

	if (words > b->dmax)
		{
		if (words > (INT_MAX/(4*BN_BITS2)))
			return NULL;
			
		bn_check_top(b);	
		if (BN_get_flags(b,BN_FLG_STATIC_DATA))
			return(NULL);
		a=A=(BN_ULONG *)rtlglue_malloc(sizeof(BN_ULONG)*(words+1));
		if (A == NULL)
			return(NULL);
		B=b->d;
		/* Check if the previous number needs to be copied */
		if (B != NULL)
			{
			for (i=b->top>>2; i>0; i--,A+=4,B+=4)
				{
				/*
				 * The fact that the loop is unrolled
				 * 4-wise is a tribute to Intel. It's
				 * the one that doesn't have enough
				 * registers to accomodate more data.
				 * I'd unroll it 8-wise otherwise:-)
				 *
				 *		<appro@fy.chalmers.se>
				 */
				BN_ULONG a0,a1,a2,a3;
				a0=B[0]; a1=B[1]; a2=B[2]; a3=B[3];
				A[0]=a0; A[1]=a1; A[2]=a2; A[3]=a3;
				}
			switch (b->top&3)
				{
				case 3:	A[2]=B[2];
				case 2:	A[1]=B[1];
				case 1:	A[0]=B[0];
				case 0:	; /* ultrix cc workaround, see above */
				}
			rtlglue_free(b->d);
			}

		b->d=a;
		b->dmax=words;

		/* Now need to zero any data between b->top and b->max */

		A= &(b->d[b->top]);
		for (i=(b->dmax - b->top)>>3; i>0; i--,A+=8)
			{
			A[0]=0; A[1]=0; A[2]=0; A[3]=0;
			A[4]=0; A[5]=0; A[6]=0; A[7]=0;
			}
		for (i=(b->dmax - b->top)&7; i>0; i--,A++)
			A[0]=0;
		
/*		memset(&(p[b->max]),0,((words+1)-b->max)*sizeof(BN_ULONG)); */
/*	{ int i; for (i=b->max; i<words+1; i++) p[i]=i;} */

		}

	return(b);
	}

BIGNUM *BN_dup(const BIGNUM *a)
	{
	BIGNUM *r;

	if (a == NULL) return NULL;

	bn_check_top(a);

	r=BN_new();
	if (r == NULL) return(NULL);
	return((BIGNUM *)BN_copy(r,a));
	}

BIGNUM *BN_copy(BIGNUM *a, const BIGNUM *b)
	{
	int i;
	BN_ULONG *A;
	const BN_ULONG *B;

	bn_check_top(b);

	if (a == b) return(a);
	if (bn_wexpand(a,b->top) == NULL) return(NULL);

	A=a->d;
	B=b->d;
	for (i=b->top>>2; i>0; i--,A+=4,B+=4)
		{
		BN_ULONG a0,a1,a2,a3;
		a0=B[0]; a1=B[1]; a2=B[2]; a3=B[3];
		A[0]=a0; A[1]=a1; A[2]=a2; A[3]=a3;
		}
	switch (b->top&3)
		{
		case 3: A[2]=B[2];
		case 2: A[1]=B[1];
		case 1: A[0]=B[0];
		case 0: ; /* ultrix cc workaround, see comments in bn_expand2 */
		}

/*	memset(&(a->d[b->top]),0,sizeof(a->d[0])*(a->max-b->top));*/
	a->top=b->top;
	if ((a->top == 0) && (a->d != NULL))
		a->d[0]=0;
	a->neg=b->neg;
	return(a);
	}

void BN_clear(BIGNUM *a)
	{
	if (a->d != NULL)
		memset(a->d,0,a->dmax*sizeof(a->d[0]));
	a->top=0;
	a->neg=0;
	}

BN_ULONG BN_get_word(BIGNUM *a)
	{
	int i,n;
	BN_ULONG ret=0;

	n=BN_num_bytes(a);
	if (n > sizeof(BN_ULONG))
		return(BN_MASK2);
	for (i=a->top-1; i>=0; i--)
		{
		ret=0;
		ret|=a->d[i];
		}
	return(ret);
	}

int BN_set_word(BIGNUM *a, BN_ULONG w)
	{
	int i,n;
	if (bn_expand(a,sizeof(BN_ULONG)*8) == NULL) return(0);

	n=sizeof(BN_ULONG)/BN_BYTES;
	a->neg=0;
	a->top=0;
	a->d[0]=(BN_ULONG)w&BN_MASK2;
	if (a->d[0] != 0) a->top=1;
	for (i=1; i<n; i++)
		{
		/* the following is done instead of
		 * w>>=BN_BITS2 so compilers don't complain
		 * on builds where sizeof(long) == BN_TYPES */
		w=0;
		a->d[i]=(BN_ULONG)w&BN_MASK2;
		if (a->d[i] != 0) a->top=i+1;
		}
	return(1);
	}

/* ignore negative */
BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret)
	{
	unsigned int i,m;
	unsigned int n;
	BN_ULONG l;

	if (ret == NULL) ret=BN_new();
	if (ret == NULL) return(NULL);
	l=0;
	n=len;
	if (n == 0)
		{
		ret->top=0;
		return(ret);
		}
	if (bn_expand(ret,(int)(n+2)*8) == NULL)
		return(NULL);
	i=((n-1)/BN_BYTES)+1;
	m=((n-1)%(BN_BYTES));
	ret->top=i;
	while (n-- > 0)
		{
		l=(l<<8L)| *(s++);
		if (m-- == 0)
			{
			ret->d[--i]=l;
			l=0;
			m=BN_BYTES-1;
			}
		}
	/* need to call this due to clear byte at top if avoiding
	 * having the top bit set (-ve number) */
	bn_fix_top(ret);
	return(ret);
	}

/* ignore negative */
int BN_bn2bin(const BIGNUM *a, unsigned char *to)
	{
	int n,i;
	BN_ULONG l;

	n=i=BN_num_bytes(a);
	while (i-- > 0)
		{
		l=a->d[i/BN_BYTES];
		*(to++)=(unsigned char)(l>>(8*(i%BN_BYTES)))&0xff;
		}
	return(n);
	}

int BN_ucmp(const BIGNUM *a, const BIGNUM *b)
	{
	int i;
	BN_ULONG t1,t2,*ap,*bp;

	bn_check_top(a);
	bn_check_top(b);

	i=a->top-b->top;
	if (i != 0) return(i);
	ap=a->d;
	bp=b->d;
	for (i=a->top-1; i>=0; i--)
		{
		t1= ap[i];
		t2= bp[i];
		if (t1 != t2)
			return(t1 > t2?1:-1);
		}
	return(0);
	}

int BN_cmp(const BIGNUM *a, const BIGNUM *b)
	{
	int i;
	int gt,lt;
	BN_ULONG t1,t2;

	if ((a == NULL) || (b == NULL))
		{
		if (a != NULL)
			return(-1);
		else if (b != NULL)
			return(1);
		else
			return(0);
		}

	bn_check_top(a);
	bn_check_top(b);

	if (a->neg != b->neg)
		{
		if (a->neg)
			return(-1);
		else	return(1);
		}
	if (a->neg == 0)
		{ gt=1; lt= -1; }
	else	{ gt= -1; lt=1; }

	if (a->top > b->top) return(gt);
	if (a->top < b->top) return(lt);
	for (i=a->top-1; i>=0; i--)
		{
		t1=a->d[i];
		t2=b->d[i];
		if (t1 > t2) return(gt);
		if (t1 < t2) return(lt);
		}
	return(0);
	}

int BN_set_bit(BIGNUM *a, int n)
	{
	int i,j,k;

	i=n/BN_BITS2;
	j=n%BN_BITS2;
	if (a->top <= i)
		{
		if (bn_wexpand(a,i+1) == NULL) return(0);
		for(k=a->top; k<i+1; k++)
			a->d[k]=0;
		a->top=i+1;
		}

	a->d[i]|=(((BN_ULONG)1)<<j);
	return(1);
	}

int BN_clear_bit(BIGNUM *a, int n)
	{
	int i,j;

	i=n/BN_BITS2;
	j=n%BN_BITS2;
	if (a->top <= i) return(0);

	a->d[i]&=(~(((BN_ULONG)1)<<j));
	bn_fix_top(a);
	return(1);
	}

int BN_is_bit_set(const BIGNUM *a, int n)
	{
	int i,j;

	if (n < 0) return(0);
	i=n/BN_BITS2;
	j=n%BN_BITS2;
	if (a->top <= i) return(0);
	return((a->d[i]&(((BN_ULONG)1)<<j))?1:0);
	}

int BN_mask_bits(BIGNUM *a, int n)
	{
	int b,w;

	w=n/BN_BITS2;
	b=n%BN_BITS2;
	if (w >= a->top) return(0);
	if (b == 0)
		a->top=w;
	else
		{
		a->top=w+1;
		a->d[w]&= ~(BN_MASK2<<b);
		}
	bn_fix_top(a);
	return(1);
	}

int bn_cmp_words(BN_ULONG *a, BN_ULONG *b, int n)
	{
	int i;
	BN_ULONG aa,bb;

	aa=a[n-1];
	bb=b[n-1];
	if (aa != bb) return((aa > bb)?1:-1);
	for (i=n-2; i>=0; i--)
		{
		aa=a[i];
		bb=b[i];
		if (aa != bb) return((aa > bb)?1:-1);
		}
	return(0);
	}
