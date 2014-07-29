//#define STEP 10

#include <stdio.h>

int case_func(int i) {

	int x = 0; 

	switch (i) {
		case 1 * STEP: x += 10; break;
		case 2 * STEP: x += 20; break;
		case 3 * STEP: x += 30; break;
		case 4 * STEP: x += 40; break;
		case 5 * STEP: x += 50; break;
		case 6 * STEP: x += 60; break;
		case 7 * STEP: x += 70; break;
		case 8 * STEP: x += 80; break;
		case 9 * STEP: x += 90; break;
		case 10 * STEP: x += 80; break;
		case 11 * STEP: x += 81; break;
		case 12 * STEP: x += 82; break;
		case 13 * STEP: x += 83; break;
		case 14 * STEP: x += 84; break;
		case 15 * STEP: x += 85; break;
		case 16 * STEP: x += 86; break;
		case 17 * STEP: x += 87; break;
		case 18 * STEP: x += 88; break;
		case 19 * STEP: x += 89; break;
		case 20 * STEP: x += 20; break;
		case 21 * STEP: x += 22; break;
		case 22 * STEP: x += 22; break;
		case 23 * STEP: x += 23; break;
		case 24 * STEP: x += 24; break;
		case 25 * STEP: x += 25; break;
		case 26 * STEP: x += 26; break;
		case 27 * STEP: x += 27; break;
		case 28 * STEP: x += 22; break;
		case 29 * STEP: x += 29; break;
		case 30 * STEP: x += 70; break;
		case 31 * STEP: x += 22; break;
		case 32 * STEP: x += 22; break;
		case 33 * STEP: x += 73; break;
		case 34 * STEP: x += 74; break;
		case 35 * STEP: x += 75; break;
		case 36 * STEP: x += 76; break;
		case 37 * STEP: x += 77; break;
		case 38 * STEP: x += 22; break;
		case 39 * STEP: x += 79; break;
		case 40 * STEP: x += 10; break;
		case 41 * STEP: x += 22; break;
		case 42 * STEP: x += 22; break;
		case 43 * STEP: x += 13; break;
		case 44 * STEP: x += 14; break;
		case 45 * STEP: x += 15; break;
		case 46 * STEP: x += 16; break;
		case 47 * STEP: x += 77; break;
		case 48 * STEP: x += 22; break;
		case 49 * STEP: x += 19; break;
		case 50 * STEP: x += 10; break;
		case 51 * STEP: x += 22; break;
		case 52 * STEP: x += 22; break;
		case 53 * STEP: x += 13; break;
		case 54 * STEP: x += 14; break;
		case 55 * STEP: x += 15; break;
		case 56 * STEP: x += 16; break;
		case 57 * STEP: x += 77; break;
		case 58 * STEP: x += 22; break;
		case 59 * STEP: x += 19; break;

	}
	
	return x;
}

int main(void) {

	int x = 0;
	int y = 0;

	for (x = 0 ; x < 1000000*500; x++) {
		y += case_func(x);
	}

	printf("%d\n", y);

	return 0;

}
