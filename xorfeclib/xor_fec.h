/* $Author: peltotas $ $Date: 2004/06/02 12:34:19 $ $Revision: 1.1 $ */
/*
 *   MAD-XOR-FEC, implementation of Simple XOR FEC codec
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

#ifndef _XOR_FEC_H_
#define _XOR_FEC_H_

#include "../alclib/inc.h"

/**** Functions ****/

trans_block_t* xor_fec_encode_src_block(char *data, 
#ifdef WIN32
										ULONGLONG len
#else
										unsigned long long len
#endif
										, unsigned int sbn, unsigned short eslen);

char *xor_fec_decode_src_block(trans_block_t *tr_block, 
#ifdef WIN32
								ULONGLONG *block_len
#else
								unsigned long long *block_len
#endif
								, unsigned short eslen);

char *xor_fec_decode_object(trans_obj_t *to,
#ifdef WIN32
							 ULONGLONG *data_len
#else
							 unsigned long long *data_len
#endif
							 , alc_session_t *s);

#endif

