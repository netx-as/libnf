

/* bit operations  */
typedef struct bit_array_s {
	int size;
	char *data;
#define BA_DATA_LEN 8	/* size in bits of one item - sizeof(char) */
} bit_array_t;


bit_array_t * bit_array_init(bit_array_t *a, int size);
int bit_array_clear(bit_array_t *a);
int bit_array_get(bit_array_t *a, int pos);
int bit_array_set(bit_array_t *a, int pos, int val);
int bit_array_cmp(bit_array_t *a, bit_array_t *b);
int bit_array_copy(bit_array_t *d, bit_array_t *s);
void bit_array_release(bit_array_t *a);

