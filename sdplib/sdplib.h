/* $Author: peltotal $ $Date: 2004/12/27 13:47:08 $ $Revision: 1.7 $ */
/*
 *	 MAD-SDP, implementation of SDP protocol
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

#ifndef _SDPLIB_H_
#define _SDPLIB_H_

#include "sdp.h"
#include "port.h"

/**** Typedefs ****/

typedef struct sf {
	char *filter_mode;
	char *net_type;
	char *addr_type;
	char *dest_addr;
	char *src_addr;
} sf_t;

typedef struct fec_dec {
	unsigned int index;
	short fec_enc_id;
	int fec_inst_id;
	struct fec_dec* next;
} fec_dec_t;

/**** Functions ****/

char* sdp_attr_get(sdp_t *sdp, char *attr_name);

sf_t* sf_char2struct(char *src_filt);
void sf_free(sf_t *sf) ;

fec_dec_t* sdp_fec_dec_get(sdp_t *sdp);
void fec_dec_free(fec_dec_t *fec_dec);
fec_dec_t* fec_dec_char2struct(char *fec_dec);

#endif
