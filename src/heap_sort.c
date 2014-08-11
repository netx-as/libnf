
/* code from http://rosettacode.org/wiki/Sorting_algorithms/Heapsort#C */

#include <stdio.h>
#include <stdlib.h>
#include <heap_sort.h>
 
//#define ValType char*
//#define IS_LESS(v1, v2)  (v1 < v2)
 
void sift_down( char **a, int start, int count, heap_sort_callback_t callback, void *p);
 
#define SWAP(r,s)  do { char *t=r; r=s; s=t; } while(0)
 
void heap_sort(char  **a, int count, heap_sort_callback_t callback, void *p) {
	int start, end;
 
	/* heapify */
	for (start = (count-2)/2; start >=0; start--) {
		sift_down( a, start, count, callback, p);
	}
 
	for (end=count-1; end > 0; end--) {
		SWAP(a[end],a[0]);
		sift_down(a, 0, end, callback, p);
	}
}
 
void sift_down(char  **a, int start, int end, heap_sort_callback_t callback, void *p) {
	int root = start;
 
	while ( root*2+1 < end ) {
		int child = 2*root + 1;
		if ((child + 1 < end) && callback(a[child], a[child+1], p)) {
			child += 1;
		}
		if (callback(a[root], a[child], p)) {
			SWAP( a[child], a[root] );
			root = child;
		} else {
            return;
		}
	}
}

