/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/
/* bit operations  */

#define BA_TYPE char

typedef struct bit_array_s {
	int size;
	char *data;
} bit_array_t;


bit_array_t * bit_array_init(bit_array_t *a, int size);
int bit_array_clear(bit_array_t *a);
int bit_array_get(bit_array_t *a, int pos);
int bit_array_set(bit_array_t *a, int pos, int val);
int bit_array_cmp(bit_array_t *a, bit_array_t *b);
int bit_array_copy(bit_array_t *d, bit_array_t *s);
void bit_array_release(bit_array_t *a);


/* fast functions without boundary check */
static void inline __bit_array_set(bit_array_t *a, int pos, int val) {
    a->data[pos] = val;
}

static int inline __bit_array_get(bit_array_t *a, int pos) {
	return a->data[pos];
}

