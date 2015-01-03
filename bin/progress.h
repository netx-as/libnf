
#define PROGRESS_MAX 512

typedef struct progress_s {
	int flags;
	int steps;
	int num;
	int textlen;
	char *file;	
} progress_t;


progress_t * progress_init(progress_t **p, int flags, char * file);

int progress_printf(progress_t *p, const char *format, ...);

int progress_steps(progress_t *p, int steps);

int progress_inc(progress_t *p, int num);

void progress_free(progress_t *p);


