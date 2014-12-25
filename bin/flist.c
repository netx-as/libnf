

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <flist.h>


/* initialise file list */
int flist_init(flist_t **l) {

	*l = NULL;

	return 1;
}


/* add a new entry to the linked list */
int flist_push(flist_t **l, char *name) {
	
	flist_t *node;

	node = malloc(sizeof(flist_t));

	if (node == NULL) {
		return 0;
	}

	node->name = malloc(strlen(name) + 1);

	if (node->name == NULL) {
		free(node);
		return 0;
	}

	strcpy(node->name, name);
	node->next = *l;

	*l = node;

	return 1;
}

int flist_pop(flist_t **l, char *buff) {

	flist_t *node;

	if (*l != NULL) {
		node = *l;
		strcpy(buff, node->name);
		*l = node->next;
		free(node);
		return 1;
	}

	return 0;
}

int flist_count(flist_t **l) {

	flist_t *node;
	int i = 0;

	node = *l;

	while (node != NULL) {
		node = node->next;
		i++;
	}

	return i;
}


int flist_lookup_dir(flist_t **l, char *path) {

	DIR *dirp;
	struct dirent *dp;
	struct stat fs_buff;
	char new_path[PATH_MAX] = {};
	int ret;


	/* detect file type */
	if ( stat(path, &fs_buff) != 0 ) {
		return 0;
	}	

	if (S_ISDIR(fs_buff.st_mode)) {
		/* directory */
	
		if ((dirp = opendir(path)) == NULL) {
			return 0;
		}

		while ((dp = readdir(dirp)) != NULL) {
			/* ignore file names starting with dot */
			if (dp->d_name[0] != '.') {
				strcpy(new_path, path);
				strcat(new_path, "/");
				strcat(new_path, dp->d_name);
//				printf("Lookup: %s\n", new_path);		
				ret = flist_lookup_dir(l, new_path);
				if (!ret) {
					return 0;
				}
			}
		}	
		closedir(dirp);

	} else if (S_ISREG(fs_buff.st_mode)) {
		/* regular file */
		
		return flist_push(l, path);
		
	}


	return 1;
}

/*
int main(void) {

	flist_t *l;	
	char buff[PATH_MAX]; 

	flist_init(&l);
	
	//flist_lookup_dir(&l, "/Users/tpoder/tmp");
	flist_lookup_dir(&l, "../../c/a");

	while (flist_pop(&l, buff)) {

		printf("Processing: %s\n", buff);

	}

}
*/
