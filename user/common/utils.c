/*
 * utils.c
 *
 *  Created on: 2016年2月25日
 *      Author: jk
 */

#include <string.h>

#include "utils.h"
#include "minilzo.h"


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);



/*
 * 去掉字符串开头的空格
 *
 */
const unsigned char* string_trimLeft(const unsigned char* string)
{
    const unsigned char* p = string;

    while(*p == ' ') p++;

    return p;
}


/*
 * 去掉字符串末尾的空格
 *
 */
void string_trimRight(unsigned char* string)
{
    unsigned char* p = string + strlen((const char *)string) - 1;

    while(*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')
    {
        *p = 0;
        p--;
    }

    return;
}

/*
 * locates the first occurrence of string s2 in the string
 * and return the next pointer by s2 in s1
 * if not found, the return null
 */
char *string_bypass(const char *s1, const char *s2)
{
    char *p = strstr(s1, s2);
    if(p)
    {
        p += strlen(s2);
    }

    return p;
}

//equivalent to eat_acsii_to_ucs2
void ascii2unicode(unsigned short* out, const unsigned char* in)
{
    int i = 0;
    unsigned char* outp = (unsigned char*)out;
    const unsigned char* inp = in;

    while( inp[i] )
    {
        outp[i * 2] = inp[i];
        outp[i * 2 + 1] = 0x00;
        i++;
    }

    out[i] = 0;
}

void unicode2ascii(unsigned char* out, const unsigned short* in)
{
    int i = 0;

    while( in[i] )
    {
        out[i] = in[i] & 0xFF;
        i++;
    }

    out[i] = 0;
}

/*
 * return
 *  0: success
 *  other: fail(refer to lzoconf.h)
 */
int miniLZO_compress(const char* src, int src_len, char* dst, int dst_len)
{
    return lzo1x_1_compress(src, src_len, dst, &dst_len,wrkmem);
}

/*
 * return
 *  0: success
 *  other: fail(refer to lzoconf.h)
 */
int miniLZO_decompress(const char* src, unsigned int src_len, char* dst, unsigned int* dst_len)
{
    return lzo1x_decompress_safe(src, src_len, dst, dst_len, NULL);
}

