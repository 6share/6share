/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.6 $ */
/*
 *   MAD-ALC, implementation of ALC/LCT protocols
 *   Copyright (c) 2003-2004 TUT - Tampere University of Technology
 *   main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "inc.h"

/**** Set global variables ****/

bool lib_init = false;

/**** Local global variables ****/

#ifdef WIN32
double time_factor;
LONGLONG start_time;
#endif /* WIN32 */

/**** Private functions ****/

void char_to_ushort(char c, unsigned short *target, int point);

#ifdef WIN32
void sec_init(void);
#endif /* WIN32 */

#ifdef WIN32

/*
 * This function initializes timer.
 *
 * Params:	void
 *
 * Return:	void
 *
 */

void sec_init(void) {	
	LONGLONG perf_cnt;
	QueryPerformanceFrequency((LARGE_INTEGER*)&perf_cnt);
	time_factor = 1.0/perf_cnt;
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
}

/*
 * This function return number of seconds since sec_init was called.
 *
 * Params:	void
 *
 * Return: double: 	Time since sec_init was called
 *
 */

double sec(void) {	
	LONGLONG cur_time;
	double time_span;         
	QueryPerformanceCounter((LARGE_INTEGER*)&cur_time);

	time_span = (cur_time - start_time) * time_factor;
	return time_span;
}	
#else

/*
 * This function return number of seconds since sec_init was called.
 *
 * Params:	void
 *
 * Return: double: 	Time since sec_init was called
 *
 */

double sec(void) {

	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}

#endif /* WIN32 */

/*
 * This function initialize library
 *
 * Params:	void
 *
 * Return:	void
 *
 */

void alc_init(void) {

#ifdef WIN32
	sec_init(); /* Initialize timer */
#endif /* WIN32 */
	lib_init = true;
}

/*
 * This function converts character to unsigned short and adds that value to target.
 *
 * Params:	char c: Character to be converted,
 *			unsigned short *target: Pointer where to convert,
 *			int point: Character place in four character string
 *
 * Return:	void
 *
 */

void char_to_ushort(char c, unsigned short *target, int point) {

	if(c == '0') {
		*target += 0x0 << (point * 4);
	}
	else if(c == '1') {
		*target += 0x1 << (point * 4);
	}
	else if(c == '2') {
		*target += 0x2 << (point * 4);
	}
	else if(c == '3') {
		*target += 0x3 << (point * 4);
	}
	else if(c == '4') {
		*target += 0x4 << (point * 4);
	}
	else if(c == '5') {
		*target += 0x5 << (point * 4);
	}
	else if(c == '6') {
		*target += 0x6 << (point * 4);
	}	
	else if(c == '7') {
		*target += 0x7 << (point * 4);
	}
	else if(c == '8') {
		*target += 0x8 << (point * 4);
	}
	else if(c == '9') {
		*target += 0x9 << (point * 4);
	}
	else if(((c == 'a') || (c == 'A'))) {
		*target += 0xa << (point * 4);
	}
	else if(((c == 'b') || (c == 'B'))) {
		*target += 0xb << (point * 4);
	}
	else if(((c == 'c') || (c == 'C'))) {
		*target += 0xc << (point * 4);
	}
	else if(((c == 'd') || (c == 'D'))) {
		*target += 0xd << (point * 4);
	}
	else if(((c == 'e') || (c == 'E'))) {
		*target += 0xe << (point * 4);
	}
	else if(((c == 'f') || (c == 'F'))) {
		*target += 0xf << (point * 4);
	}
}

/*
 * This function converts IPv6 hexa-address to IPv6 decimal-address.   
 *
 * Params:	char *ipv6: Pointer to IPv6 string to be converted,
 *			unsigned short *ipv6addr: Pointer where to convert,
 *			int *nb_ipv6_part: Number of parts in IPv6 string address.
 *
 * Return:	int: 0 in success, -1 in errors
 *
 */

int ushort_ipv6addr(char *ipv6, unsigned short *ipv6addr, int *nb_ipv6_part) { 

	char *ptr;
	int point; 
	int ch = ':';

	char *tmp = NULL;

	int i, j;

	char c;
	char first[5];
	char second[40];

	if(!(tmp = (char*)calloc((strlen(ipv6) + 1), sizeof(char)))) {
		printf("Could not alloc memory for tmp!\n");
		return -1;
	}

	memcpy(tmp, ipv6, strlen(ipv6));

	ptr = strchr(tmp, ch);

	for(*nb_ipv6_part = 0;;(*nb_ipv6_part)++) {

		if(*nb_ipv6_part > 7) {
			printf("Invalid IPv6 address!\n");
			fflush(stdout);
			free(tmp);
			return -1;
		}

		*(ipv6addr + *nb_ipv6_part) = 0;

		if(ptr == NULL) {

			if(strlen(second) > 4) {
				printf("Invalid IPv6 address!\n");
				fflush(stdout);
				free(tmp);
				return -1;
			}
			
			for(i = 0; i < (int)strlen(second); i++) {

				c = second[i];
				char_to_ushort(c, (ipv6addr + *nb_ipv6_part), ((int)strlen(second) - i - 1));
			}

			break;
		}

		memset(first, 0, 5);
		memset(second, 0, 40);

		point = (int)(ptr - tmp);

		memcpy(first, tmp, point);
		memcpy(second, (tmp + point + 1), (strlen(tmp) - (point + 1)));

		strcpy(tmp, second);

		if(strlen(first) > 4) {
			printf("Invalid IPv6 address!\n");
			fflush(stdout);
			free(tmp);
			return -1;
		}

		for(j = 0; j < (int)strlen(first); j++) {
			
			c = first[j];
			char_to_ushort(c, (ipv6addr + *nb_ipv6_part), ((int)strlen(first) - j - 1));
		}

		ptr = strchr(tmp, ch);
	}

	free(tmp);
	return 0;
}
