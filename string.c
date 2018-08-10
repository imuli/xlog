#include <stdio.h>
#include <string.h>
#include "xlog.h"

int
eqstr(const char *a, const char *b){
	if(a == b) return 1;
	if(a == NULL || b == NULL) return 0;
	return strcmp(a,b) == 0;
}

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

