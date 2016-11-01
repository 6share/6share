/* $Author: peltotal $ $Date: 2004/05/05 05:43:56 $ $Revision: 1.12 $ */
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

#ifndef _ALC_RX_H_
#define _ALC_RX_H_

/**** Functions ****/

void* rx_thread(void *s);
char* alc_recv(int s_id, 
#ifdef WIN32			   
			   ULONGLONG toi,
			   ULONGLONG *data_len,
#else
			   unsigned long long toi,
			   unsigned long long *data_len,
#endif
			   int *retval);

char* alc_recv2(int s_id,
#ifdef WIN32			   
			   ULONGLONG *toi,
			   ULONGLONG *data_len,
#else
			   unsigned long long *toi,
			   unsigned long long *data_len,
#endif
			   int *retval);

char* alc_recv3(int s_id,
#ifdef WIN32			   
			   ULONGLONG *toi,
#else
			   unsigned long long *toi,
#endif
			   int *retval);

char* fdt_recv(int s_id,
			   
#ifdef WIN32
				ULONGLONG *data_len,
#else
				unsigned long long *data_len,
#endif
				int *retval

#ifdef USE_ZLIB
			   , unsigned char *cont_enc_algo
#endif
			   );

#endif
