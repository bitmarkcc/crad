#include <stdlib.h>
#include <string.h>

#include <util.h>

char* rad_strcpy (char* out, const char* inp, int from, int len) {
    const char* inp_shifted = inp+from;
    int len_inp_shifted = strlen(inp_shifted);
    if (len <= len_inp_shifted) {
	memcpy(out,inp,len);
	out[len] = 0;
    }
    else {
	memcpy(out,inp,len_inp_shifted);
	out[len_inp_shifted] = 0;
    }
    return out;
}

char* rad_strip(char* s) {
    int len_s = strlen(s);
    if (s[len_s-1]=='\n') {
	s[len_s-1] = 0;
    }
    return s;
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char hex_digit(char c) {
    return p_util_hexdigit[(unsigned char)c];
}

char* rad_hex_to_uchar(const char* hex) {
    int siz = strlen(hex)/2;
    if (siz%2==1) return 0;
    uint8_t* out = malloc(siz);
    for (int i=0; i<siz; i++) {
	uint8_t c = hex_digit(*hex++);
	uint8_t n = c << 4;
	c = hex_digit(*hex++);
	if (c<0) break;
	n |= c;
	out[i] = n;
    }
    return out;
}

bool rad_is_space(char c) {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}
