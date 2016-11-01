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

#include "inc.h"

/*
 * This function adds FDT LCT extension header to FLUTE's header.
 *
 * Params:	def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *			int hdrlen: Current length of FLUTE header,
 *			unsigned int fdt_instance_id: FDT Instance ID.
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_fdt_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen, unsigned int fdt_instance_id) {

	unsigned int word;
	int len = 0;

	word = ((EXT_FDT << 24) | ((FLUTE_VERSION & 0xF) << 20) | (fdt_instance_id & 0x000FFFFF));
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
	len += 4;

	return len;

}

/*
 * This function adds CENC LCT extension header to FLUTE's header.
 *
 * Params:      def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *              int hdrlen: Current length of FLUTE header,
 *              unsigned char comp_enc_algo: Content encoding algorith used in the FDT Instance Payload.
 *
 * Return:      int: Number of bytes added to FLUTE's header
 *
 */

int add_cenc_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen, unsigned char cont_enc_algo) {

        unsigned int word;
        int len = 0;

        word = ((EXT_CENC << 24) | (cont_enc_algo << 16) | (0 & 0x0000FFFF));
        *(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
        len += 4;

        return len;
}

/*
 * This function adds FTI (FEC Encoding IDs 0, 128, 130) LCT extension header to FLUTE's header.
 *
 * Params:	def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *			int hdrlen: Current length of FLUTE header,
 *			ULONGLONG/unsigned long long transferlen: Length of transport object,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum Source Block Length.
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_fti_0_128_130_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen,
#ifdef WIN32					 
							ULONGLONG transferlen,
#else
							unsigned long long transferlen,
#endif
							unsigned short fec_inst_id, unsigned short eslen, unsigned int max_sblen) {

	unsigned int word;
	unsigned short tmp;
	int len = 0;
	
	tmp = ((unsigned int)(transferlen >> 32) & 0x0000FFFF);
	word = ((EXT_FTI << 24) | (4 << 16) | tmp);
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
	len += 4;

	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl((unsigned int)transferlen);
	len += 4;

	word = ((fec_inst_id << 16) | eslen);
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl(word);
	len += 4;

	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl(max_sblen);
	len += 4;
	
	return len;
}

/*
 * This function adds FTI (FEC Encoding ID 129) LCT extension header to FLUTE's header.
 *
 * Params:	def_lct_hdr_t *def_ldt_hdr: Pointer to the default LCT header,
 *			int hdrlen: Current length of FLUTE header,
 *			ULONGLONG/unsigned long long objlen: Length of transport object,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned short max_sblen: Maximum Source Block Length,
 *			unsigned short mxnbofes: Maximum Number of Encoding Symbols.
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_fti_129_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen,
#ifdef WIN32					 
					 ULONGLONG transferlen,
#else
					 unsigned long long transferlen,
#endif

					 unsigned short fec_inst_id, unsigned short eslen,
					 unsigned short max_sblen, unsigned short mxnbofes) {

	unsigned int word;
	unsigned short tmp;
	int len = 0;
	
	tmp = ((unsigned int)(transferlen >> 32) & 0x0000FFFF);
	word = ((EXT_FTI << 24) | (4 << 16) | tmp);
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
	len += 4;

	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl((unsigned int)transferlen);
	len += 4;

	word = ((fec_inst_id << 16) | eslen);
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl(word);
	len += 4;

	word = ((max_sblen << 16) | mxnbofes);
	*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen + len) = htonl(word);
	len += 4;
	
	return len;
}

/*
 * This function adds NOP LCT extension header to FLUTE's header.
 *
 * Params:	To be defined
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_nop_lct_he(void) {
	int len = 0;
	return len;
}

/*
 * This function adds AUTH LCT extension header to FLUTE's header.
 *
 * Params:	To be defined
 *
 * Return:	int: Number of bytes added to FLUTE's header
 *
 */

int add_auth_lct_he(void) {
	int len = 0;
	return len;
}

