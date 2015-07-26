#include <stdio.h>

/*
 * Demonstration code for sorting a linked list.
 * 
 * The algorithm used is Mergesort, because that works really well
 * on linked lists, without requiring the O(N) extra space it needs
 * when you do it on arrays.
 * 
 * This code can handle singly and doubly linked lists, and
 * circular and linear lists too. For any serious application,
 * you'll probably want to remove the conditionals on `is_circular'
 * and `is_double' to adapt the code to your own purpose. 
 * 
 */

/*
 * This file is copyright 2001 Simon Tatham.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL SIMON TATHAM BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <hash_table.h>
#include <list_sort.h>

#define FALSE 0
#define TRUE 1

/*
typedef struct element element;
struct element {
    element *next, *prev;
    int i;
};

int cmp(element *a, element *b) {
    return a->i - b->i;
}
*/

/*
 * This is the actual sort function. Notice that it returns the new
 * head of the list. (It has to, because the head will not
 * generally be the same element after the sort.) So unlike sorting
 * an array, where you can do
 * 
 *     sort(myarray);
 * 
 * you now have to do
 * 
 *     list = listsort(mylist);
 */
hash_table_row_hdr_t * list_sort(hash_table_row_hdr_t  *list, list_sort_callback_t callback, void *opts) {
    hash_table_row_hdr_t *p, *q, *e, *tail, *oldhead;
    int insize, nmerges, psize, qsize, i;

    /*
     * Silly special case: if `list' was passed in as NULL, return
     * NULL immediately.
     */
    if (!list)
	return NULL;

    insize = 1;

    while (1) {
        p = list;
	oldhead = list;		       /* only used for circular linkage */
        list = NULL;
        tail = NULL;

        nmerges = 0;  /* count number of merges we do in this pass */

        while (p) {
            nmerges++;  /* there exists a merge to be done */
            /* step `insize' places along from p */
            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) {
                psize++;
		    q = (hash_table_row_hdr_t *)q->snext;
                if (!q) break;
            }

            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;

            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) {

                /* decide whether next element of merge comes from p or q */
                if (psize == 0) {
		    /* p is empty; e must come from q. */
		    e = q; q = (hash_table_row_hdr_t *)q->snext; qsize--;
		} else if (qsize == 0 || !q) {
		    /* q is empty; e must come from p. */
		    e = p; p = (hash_table_row_hdr_t *)p->snext; psize--;
		} else if (callback((char *)p, (char *)q, opts) >= 0) {
		    /* First element of p is lower (or same);
		     * e must come from p. */
		    e = p; p = (hash_table_row_hdr_t *)p->snext; psize--;
		} else {
		    /* First element of q is lower; e must come from q. */
		    e = q; q = (hash_table_row_hdr_t *)q->snext; qsize--;
		}

                /* add the next element to the merged list */
		if (tail) {
		    tail->snext = (char *)e;
		} else {
		    list = e;
		}
		tail = e;
            }

            /* now p has stepped `insize' places along, and q has too */
            p = q;
        }
	    tail->snext = NULL;

        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
            return list;

        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
}

