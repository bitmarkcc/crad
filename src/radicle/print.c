#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <print.h>

void eprintf (const char* format, ...) {
    va_list args;

    time_t now = time(0);
    struct tm* t = localtime(&now);
    char time_str [26];
    strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",t);
   
    fprintf(stderr,"%s ERROR ",time_str);

    va_start(args,format);
    vfprintf(stderr,format,args);
    va_end(args);
    
    fprintf(stderr,"\n");
}

void iprintf (const char* format, ...) {
    va_list args;

    time_t now = time(0);
    struct tm* t = localtime(&now);
    char time_str [26];
    strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",t);
   
    fprintf(stdout,"%s INFO ",time_str);

    va_start(args,format);
    vfprintf(stdout,format,args);
    va_end(args);
    
    fprintf(stdout,"\n");
}
