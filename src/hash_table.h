

/* hash table handler */
typedef struct hash_table_s {
	int size;
	char *data;
#define BA_DATA_LEN 8	/* size in bits of one item - sizeof(char) */
} hash_table_t;


hast_table_t * hash_table_init(hash_table_t *t, int keysize, int valsize);
/*int bit_array_clear(bit_array_t *a);
int bit_array_get(bit_array_t *a, int pos);
int bit_array_set(bit_array_t *a, int pos, int val);
int bit_array_cmp(bit_array_t *a, bit_array_t *b);
int bit_array_copy(bit_array_t *d, bit_array_t *s);
void bit_array_release(bit_array_t *a);
*/
