

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include "progress.h"


progress_t * progress_init(progress_t **pp, int flags, char * file) {

	progress_t *p;	

	p = malloc(sizeof(progress_t));

	if ( p == NULL ) {
		return NULL;
	}

	p->file = file;
	p->flags = flags;
	p->textlen = 0;

	*pp = p;

	return p;

}

int progress_printf(progress_t *p, const char *format, ...) {

	va_list argptr;
	int i, textlen;
	char text[PROGRESS_MAX];

	if (p == NULL) {
		return 0;
	}

	va_start(argptr, format);
	vsnprintf(text, PROGRESS_MAX, format, argptr);
	va_end(argptr);
		
	if (p->file == NULL) {
		fprintf(stderr, "%s", text);
		textlen = strlen(text);
		if (textlen < p->textlen) {
			for (i = textlen; i < p->textlen; i++) {
				fprintf(stderr, " ");
			}
		}
		p->textlen = textlen;
		fprintf(stderr, "\r");
		fflush(stderr);

		return 1;
	}
}

int progress_steps(progress_t *p, int steps) {

	if (p == NULL) {
		return 0;
	}

	p->steps = steps;
	p->num = 0;

	return 1;
}

int progress_inc(progress_t *p, int num) {

	if (p == NULL) {
		return 0;
	}

	p->num += num;

	progress_printf(p, "Processing: %d/%d (%.1f\%)", p->num, p->steps, (double)p->num * 100 / p->steps);
	
	return 1;
}

void progress_free(progress_t *p) {
	free(p);
}

