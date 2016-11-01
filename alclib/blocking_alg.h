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

#ifndef _BLOCKING_ALG_H_
#define _BLOCKING_ALG_H_

/**** Typedefs ****/

typedef struct blocking_struct {

        unsigned int N;
        unsigned int I;
        unsigned int A_large;
        unsigned int A_small;

} blocking_struct_t;

/**** Functions ****/

blocking_struct_t *compute_blocking_structure(
#ifdef WIN32
		ULONGLONG L,
#else
		unsigned long long L,
#endif
		unsigned int B, unsigned int E);

#endif

