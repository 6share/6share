/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.8 $ */
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

#ifndef _MAD_H_
#define _MAD_H_

/**** Macros for min and max if not defined in Operation System ****/

#ifndef min
	#define min(a,b)	((a) <= (b) ? (a) : (b))
#endif

#ifndef max
	#define max(a,b)	((a) >= (b) ? (a) : (b))
#endif

/**** Global typedefs *****/

typedef enum {
	false,
	true
} bool;

/**** Global variables ****/

extern bool lib_init;

/**** Functions ****/

void alc_init(void);
double sec(void);
int ushort_ipv6addr(char *ipv6, unsigned short *ipv6addr, int *nb_ipv6_part);

#endif /* _MAD_H_ */



