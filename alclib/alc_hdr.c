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

/*
 * This function adds FEC Payload ID (FEC Encoding ID 128) header to FLUTE's header.
 *
 * Params:	def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *			int hdrlen: Current length of FLUTE header,
 *			unsigned int sbn: Source Block Number,
 *			unsigned int es_id: Encoding Symbol Identifier.
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_alc_fpi_128(def_lct_hdr_t *def_lct_hdr, int hdrlen, unsigned int sbn, unsigned int es_id) {

	int len = 0;

	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(sbn);
	len += 4;
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl(es_id);	
	len += 4;
	
	return len;
}

/*
 * This function adds FEC Payload ID (FEC Encoding ID 129) header to FLUTE's header.
 *
 * Params:	def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *			int hdrlen: Current length of FLUTE header,
 *			unsigned int sbn: Source Block Number,
 *			unsigned short sbl: Source Block Length,
 *			unsigned short es_id: Encoding Symbol Identifier.
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_alc_fpi_129(def_lct_hdr_t *def_lct_hdr, int hdrlen, unsigned int sbn, 
					unsigned short sbl, unsigned short es_id) {

	int len = 0;
	unsigned int word = 0;

	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(sbn);

	len += 4;
	word = ((sbl << 16) | (es_id & 0xFFFF));
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl(word);	
	len += 4;

	return len;
}

/*
 * This function adds FEC Payload ID (FEC Encoding ID 0, 130) header to FLUTE's header.
 *
 * Params:	def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *			int hdrlen: Current length of FLUTE header,
 *			unsigned short sbn: Source Block Number,
 *			unsigned short es_id: Encoding Symbol Identifier.
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_alc_fpi_0_130(def_lct_hdr_t *def_lct_hdr, int hdrlen, unsigned short sbn, unsigned short es_id) {

	int len = 0;
	unsigned int word = 0;

	word = ((sbn << 16) | (es_id & 0xFFFF));

	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);	
	len += 4;

	return len;
}
