
/* the hash table is organized in buckets */
/* every bucket can contain 2^16 items */
/* the maximum number of items for hash */
/* table is 2^16 * HASH_TABLE_MAX_BUCKETS */
#define HASH_TABLE_BUCKET_SIZE 65536
#define HASH_TABLE_MAX_BUCKETS 65536

/* number of collision that invoke increase of hash table */
#define HASH_TABLE_COLLISIONS 4

/* hash table iflags for every row */
typedef struct hash_table_row_flags_s {
	unsigned char occupied:1;
	unsigned char locked:1;
} hash_table_row_flags_t;


/* callback called when same key is found */
typedef void (*hash_table_callback_t)(char *key, char *hval, char *uval, void *p);

/* hash table handler */
typedef struct hash_table_s {
	int keylen;					/* size of aggregation key */
	int vallen;					/* size of vallues key */
	int rowlen;					/* total size of tow (row_flags_t + akey, skey, val) */
	hash_table_callback_t callback;
	int numbuckets;				/* number of allocated buckets */
	void * bucket[HASH_TABLE_MAX_BUCKETS];
} hash_table_t;



hash_table_t * hash_table_init(hash_table_t *t, int keylen, int vallen, hash_table_callback_t cb);
void * hash_table_insert(hash_table_t *t, char *key, char *val, void *p);

