/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.9 $ */
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

/*
 * This function converts double value to correct integer value.
 *
 * Params:	double value: Value to be converted.
 *
 * Return:	int: Converted integer value.
 *
 */

/*int double_to_int(double value) {
  
  int retval;
  double ceil_value;
  double floor_value;
  double abs_diff1;
  double abs_diff2;
 
  ceil_value = ceil(value);
  floor_value = floor(value);
 
  abs_diff1 = fabs(value - ceil_value);
  abs_diff2 = fabs(value - floor_value);
 
  if(abs_diff1 < abs_diff2) {
    retval = (int)ceil_value;     
  }
  else {
    retval = (int)floor_value;
  }

  return retval;
}*/

/*
 * This function calculates blocking scheme parameters.
 *
 * Params:	unsigned int L: Transport Object Length,
 *			unsigned int B: Source Block Length,
 *			unsigned int E: Encoding Symbol Length.
 *
 * Return:	blocking_struct_t: Structure containing blocking scheme parameters,
 *			NULL in error situations.
 *
 */

/*blocking_struct_t* compute_blocking_structure(unsigned int L, unsigned int B, unsigned int E) {

	unsigned int T;
	double A;
	double A_fraction;
	blocking_struct_t *bs;
	double tmp;

    if (!(bs = (blocking_struct_t*)calloc(1, sizeof(blocking_struct_t)))) {
            printf("Could not alloc memory for blocking_struct!\n");
            return NULL;
    }*/

	/* (a) */

	/*T = (unsigned int)ceil((double)L / E);*/

	/* (b) */

	/*bs->N = (unsigned int)ceil((double)T / B);*/

	/* (c) */

	/*A = ((double)T / (double)bs->N);*/

	/* (d) */

	/*bs->A_large = (unsigned int)ceil(A);*/

	/* (e) */

	/*bs->A_small = (unsigned int)floor(A);*/
	
	/* (f) */

	/*A_fraction = A - bs->A_small;*/

	/* (g) */

	/*tmp = A_fraction * (double)bs->N;

	bs->I = double_to_int(tmp);

	return bs;
}*/

/*
 * This function calculates blocking scheme parameters.
 *
 * Params:	ULONGLONG/unsigned long long L: Transport Object Length,
 *			unsigned int B: Source Block Length,
 *			unsigned int E: Encoding Symbol Length.
 *
 * Return:	blocking_struct_t: Structure containing blocking scheme parameters,
 *			NULL in error situations.
 *
 */

blocking_struct_t* compute_blocking_structure(
#ifdef WIN32
		ULONGLONG L,
#else
		unsigned long long L,
#endif 
		unsigned int B, unsigned int E) {

	unsigned int T;
	blocking_struct_t *bs;
	div_t div_T;
	div_t div_N;
	div_t div_A;

    if (!(bs = (blocking_struct_t*)calloc(1, sizeof(blocking_struct_t)))) {
            printf("Could not alloc memory for blocking_struct!\n");
            return NULL;
    }

	/* (a) */

	div_T = div((int)L, E);
	
	if(div_T.rem == 0) {
		T = div_T.quot;
	}
	else {
		T = div_T.quot + 1;
	}

	/* (b) */

	div_N = div(T, B);

	if(div_N.rem == 0) {
		bs->N = div_N.quot;
	}
	else {
		bs->N = div_N.quot + 1;
	}

	/* (c) */

	div_A = div(T, bs->N);
	
	if(div_A.rem == 0) {
		bs->A_large = div_A.quot;
	}
	else {
		bs->A_large = div_A.quot + 1;
	}

	/* (d) */
	
	bs->A_small = div_A.quot;
	
	/* (e) */

	bs->I = div_A.rem;

	return bs;
} 








