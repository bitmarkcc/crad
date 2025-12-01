#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>

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

void rad_rstrip_nl(char* str) {
    int len_str = strlen(str);
    if (str[len_str-1]=='\n') {
	str[len_str-1] = 0;
    }
}

char* rad_strip (char c, const char* str) {
    char* out = malloc(strlen(str)+1);
    bool start = true;
    int i = 0;
    while (*str) {
	if (start && *str==c) {
	    str++;
	    continue;
	}
	if (*str!=c) {
	    start = false;
	}
	if (!start) {
	    out[i] = *str;
	}
	i++;
	str++;
    }
    bool end = true;
    char* it = out+i-1;
    while (*it) {
	if (end && *it==c) {
	    it--;
	    continue;
	}
	if (*it!=c) {
	    end = false;
	    it[1] = 0;
	    break;
	}
    }
    return out;
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

bool rad_is_space (char c) {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

bool rad_get_input (char* str, size_t bufsiz) {
    if (!fgets(str,bufsiz,stdin)) return false;
    rad_rstrip_nl(str);
    return true;
}

char* rad_to_lower (const char* str) {
    size_t len = strlen(str);
    char* out = malloc(len+1);
    for (int i=0; i<len; i++) {
	out[i] = tolower(str[i]);
    }
    out[len] = 0;
    return out;
}

char* rad_remove_space_json (const char* str) {
    size_t len = strlen(str);
    char* out = malloc(len+1);
    int j=0;
    bool inValue = false;
    for (int i=0; i<len; i++) {
	if (!inValue && rad_is_space(str[i])) continue;
	out[j] = str[i];
	if (inValue && out[j] == '"') inValue = false;
	else if (out[j]=='"' && out[j-1]==':') inValue = true;
	j++;
    }
    out[j] = 0;
    return out;    
}

void rad_assert_equal (const uint8_t* a, const uint8_t* b, size_t n) {
    for (int i=0; i<n; i++) {
	assert(a[i] == b[i]);
    }
}

int rad_push_array (size_t* pn, void** arr, size_t m, void* pelement) {
    size_t n = *pn;
    if (n>0) {
	*arr = realloc(*arr,(n+1)*m);
	memcpy(*arr+n*m,pelement,m);
    }
    else {
	*arr = malloc(m);
	memcpy(*arr,pelement,m);
    }
    *pn = n+1;
}

char* time_offset (int offset) {
    if (offset<0) return "0000"; //shouldn't happen
    char* offset_str = malloc(5);
    memset(offset_str,'0',4);
    if (offset < 10)
	sprintf(offset_str+3,"%d",offset);
    else if (offset < 100)
	sprintf(offset_str+2,"%d",offset);
    else if (offset < 1000)
	sprintf(offset_str+1,"%d",offset);
    else
	sprintf(offset_str,"%d",offset);
    return offset_str;
}

int exec_command (const char* file, char* const argv []) {
    int pid = fork();
    switch (pid) {
    case -1:
	fprintf(stderr,"fork failed");
	return 1;
    case 0:
	//setpgrp(); //todo check if needed
	return execvp(file,argv);
    default:
	waitpid(pid,0,0);
	return 0;
    }
}

char* get_password () {
    static struct termios old_terminal;
    static struct termios new_terminal;

    //get settings of the actual terminal
    tcgetattr(STDIN_FILENO, &old_terminal);

    // do not echo the characters
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ECHO);

    // set this as the new terminal options
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);

    // get the password
    // the user can add chars and delete if he puts it wrong
    // the input process is done when he hits the enter
    // the \n is stored, we replace it with \0
    char* password = malloc(RAD_BUFSIZ);
    if (fgets(password, RAD_BUFSIZ, stdin) == NULL)
	password[0] = '\0';
    else
	password[strlen(password)-1] = '\0';

    // go back to the old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
    printf("\n");
    return password;
}
