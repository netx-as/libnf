

typedef int (*list_sort_callback_t)(char *key1, char *key2, void *p);

hash_table_row_hdr_t * list_sort(hash_table_row_hdr_t  *list, list_sort_callback_t callback, void *opts);


