#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>

#include "xlog.h"

// general purpose

static const char escaped[] = "\
0ooooooabtnvfroooooooooooooooooo\
                                \
                            \\   \
                                \
                                \
                                \
                                \
";
                                
void
putchar_escaped(char c){
	char style = escaped[(unsigned char)c];
	switch(style){
	case 'o':
		printf("\\%03hho", c);
		break;
	case ' ':
		putchar(c);
		break;
	default:
		putchar('\\');
		putchar(style);
	}
}

void
putstr_escaped(const char *s){
	for(;*s != '\0';s++)
		putchar_escaped(*s);
}

void
putKeySym(KeySym k){
	char *str = XKeysymToString(k);
	if(str == NULL) {
		printf("%ld", k);
	} else {
		fputs(str, stdout);
	}
}

void
putTime(struct timeval *tv) {
	char buf[64];
	struct tm *gm = gmtime(&tv->tv_sec);
	strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", gm);
	fputs(buf, stdout);
	printf(".%03ld", tv->tv_usec / 1000);
}

