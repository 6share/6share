/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.12 $ */
/*
 *   MAD-FLUTE, implementation of FLUTE protocol
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

#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#include "../alclib/inc.h"
#include "fdt.h"

/**** Functions ****/

int recvfile(int s_id, char *filepath,
#ifdef WIN32
			ULONGLONG toi,
#else
			unsigned long long toi,
#endif
			char *md5, int big_file_mode
#ifdef USE_ZLIB
			, char *encoding
#ifdef WIN32	
			, ULONGLONG file_len
#else
			, unsigned long long file_len
#endif

#endif
			 );

int fdtbasedrecv(fdt_t *fdt, int s_id, int openfile);
int fdtbasedrecv2(fdt_t *fdt, int s_id, int openfile, int accept_expired_files);
int fdtbasedrecv3(fdt_t *fdt, int s_id, int openfile, int accept_expired_files);

#endif /* _RECEIVER_H_ */
