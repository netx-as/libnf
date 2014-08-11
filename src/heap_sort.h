

typedef int (*heap_sort_callback_t)(char *key1, char *key2, void *p);

void heap_sort(char  **a, int count, heap_sort_callback_t callback, void *p);


