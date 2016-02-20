

#define MAX_STR 100			/* max length of format string */
#define MAX_STR_LONG 1000   /* max length of format string */


/* output formats */
typedef enum output_fmt_s {
	OFMT_BIN_NFDUMP,
	OFMT_LINE,
	OFMT_RAW
} output_fmt_t;

struct output_s;

/* prototype of formating function */
typedef void (*format_func_t)(char *buff, char *data);
typedef int (*output_start_func_t)(struct output_s *output);
typedef int (*output_row_func_t)(struct output_s *output, lnf_rec_t *rec);
typedef int (*output_finish_func_t)(struct output_s *output);


/* list of the fields to be displayed */
typedef struct field_ent_s {
	int field;
	int type;
	int numbits;
	int numbits6;
	char format[MAX_STR];
	char hdr_format[MAX_STR];
	char name[MAX_STR];
	format_func_t format_func;
} field_ent_t;


typedef struct output_s {
	output_fmt_t output_fmt;
	char *filename;
	field_ent_t fields[LNF_FLD_TERM_];
	int numfields;
	int sortbits4;
	int sortbits6;

	lnf_rec_t *recp;
	lnf_mem_t *memp;
	int sortfield;
	long int outputflows;

	lnf_file_t *filep;

	output_start_func_t output_start_func;
	output_row_func_t output_row_func;
	output_finish_func_t output_finish_func;

} output_t;

/* initialise output */
void output_init(output_t *output);
void output_set_fmt(output_t *output, output_fmt_t output_fmt, char *filename);
void output_set_sort(output_t *output, int sortfield, int sortbits4, int sortbits6);

int output_field_add(output_t *output, int field);

int output_write(output_t *output, lnf_rec_t *rec);
int output_merge_threads(output_t *output);

int output_start(output_t *output);
int output_output_rows(output_t *output);
int output_row(output_t *output, lnf_rec_t *rec);
int output_finish(output_t *output);

/* parse aggreg string and add into output_t and lnf_mem_t */
int output_parse_aggreg(output_t *output, char *str);

