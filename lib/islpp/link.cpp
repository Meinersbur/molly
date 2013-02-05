
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Required by ISL
// From http://cboard.cprogramming.com/c-programming/127793-snprintf-linux-ok-windows-problem.html
// by Bayint Naung 
int snprintf(char *str, size_t size, const char *fmt, ...) {
    int ret;
    va_list ap;

    va_start(ap,fmt); 
    ret = vsnprintf(str,size,fmt,ap);
    // Whatever happen in vsnprintf, what I'll do is just to null terminate it 
    // and everything else is mynickmynick's problem
    str[size-1] = '\0';       
    va_end(ap);    
    return ret;    
}

#ifdef __cplusplus
}
#endif