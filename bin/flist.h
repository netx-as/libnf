

/* linked list of file names */
typedef struct flist_s {
    char *name;
    struct flist_s *next;
} flist_t;


/* file list operations */
int flist_init(flist_t **l);
int flist_push(flist_t **l, char *name);
int flist_pop(flist_t **l, char *buff);
int flist_count(flist_t **l);
int flist_lookup_dir(flist_t **l, char *path);

