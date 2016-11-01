/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.5 $ */
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

#ifndef _MAD_ZLIB_H_
#define _MAD_ZLIB_H_

#include "../alclib/inc.h"

#ifdef USE_ZLIB

int file_gzip_compress(char *file, char *mode);
int file_gzip_uncompress(char* file);

char* buffer_zlib_compress(char *buf, 
#ifdef WIN32
							ULONGLONG buflen, 
							ULONGLONG *comprlen
#else
							unsigned long long buflen, 
							unsigned long long *comprlen
#endif
						   );


char* buffer_zlib_uncompress(char *buf,
#ifdef WIN32
							ULONGLONG buflen, 
							ULONGLONG *uncomprlen
#else
							unsigned long long buflen, 
							unsigned long long *uncomprlen
#endif
						   );

char* buffer_gzip_uncompress(char *buf,
#ifdef WIN32
							ULONGLONG buflen, 
							ULONGLONG uncomprlen
#else
							unsigned long long buflen, 
							unsigned long long uncomprlen
#endif
						   );

#endif

#endif

