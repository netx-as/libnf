

#define MAX_STR 100			/* max length of format string */
#define MAX_STR_LONG 1000   /* max length of format string */


/* output formats */
typedef enum output_fmt_s {
	OFMT_BIN_NFDUMP,
	OFMT_TXT_LINE,
	OFMT_TXT_RAW
} output_fmt_t;


/* prototype of formating function */
typedef void (*format_func_t)(char *buff, char *data);


/* list of the fields to be displayed */
typedef struct field_ent_s {
	int field;
	int type;
	int numbits;
	int numbits6;
	char format[MAX_STR];
	char hdr_format[MAX_STR];
	format_func_t format_func;
} field_ent_t;


typedef struct output_s {
	output_fmt_t output_fmt;
	char *filename;
	field_ent_t fields[LNF_FLD_TERM_];
	int numfields;
} output_t;

/* initialise output */
void output_init(output_t *output);
void output_set_fmt(output_t *output, output_fmt_t output_fmt, char *filename);

int output_field_add(output_t *output, int field);

void print_header(output_t *output);
void print_row(output_t *output, lnf_rec_t *rec);

/* parse aggreg string and add into output_t and lnf_mem_t */
int parse_aggreg(output_t *output, lnf_mem_t *memp, char *str);

