/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.7 $ */
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

#ifndef _LCT_HDR_H_
#define _LCT_HDR_H_

#include "defines.h"

/**** Typedefs ****/

/*
 * Default LCT header
 *
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   V   | C | r |S| O |H|T|R|A|B|   HDR_LEN     |       CP      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Congestion Control Information (CCI, length = 32 bits)     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

typedef struct def_lct_hdr {
#ifdef _BIT_FIELDS_LTOH
	unsigned short	flag_b:1;		/* close session flag */
	unsigned short	flag_a:1;		/* close object flag */
	unsigned short	flag_r:1;		/* expected residual time present flag */ 
	unsigned short	flag_t:1;		/* sender current time present flag */
	unsigned short	flag_h:1;		/* half word flag */
	unsigned short	flag_o:2;		/* transport object identifier flag */
	unsigned short	flag_s:1;		/* transport session identifier flag */
	unsigned short	reserved:2;		/* reserved; must be zero */
	unsigned short	flag_c:2;		/* congestion control flag */
	unsigned short	version:4;		/* LCT version number */
	unsigned char	hdr_len;		/* total length of LCT header */
	unsigned char	codepoint;		/* identifier used by payload decoder */
#else
	unsigned short	version:4;		/* LCT version number */
	unsigned short	flag_c:2;		/* congestion control flag */
	unsigned short	reserved:2;		/* reserved; must be zero */
	unsigned short	flag_s:1;		/* transport session identifier flag */
	unsigned short	flag_o:2;		/* transport object identifier flag */
	unsigned short	flag_h:1;		/* half word flag */
	unsigned short	flag_t:1;		/* sender current time present flag */
	unsigned short	flag_r:1;		/* expected residual time present flag */
	unsigned short	flag_a:1;		/* close object flag */
	unsigned short	flag_b:1;		/* close session flag */
	unsigned char	hdr_len;		/* total length of LCT header */
	unsigned char	codepoint;		/* identifier used by payload decoder */
#endif
	unsigned int	cci;			/* congestion control header */
} def_lct_hdr_t;

/**** Functions ****/

/*
 * FDT Instance Header Extension
 *
 * 0                   1                   2                   3 
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   HET = 192   |   V   |           FDT Instance ID             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

int add_fdt_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen, unsigned int fdt_instance_id);

/*
 * Content Encoding of FDT Instance Payload
 *   
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   HET = 193   |     CENC      |          Reserved             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

int add_cenc_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen,
					unsigned char cont_enc_algo);

/*
 * FEC Object Transmission Information Extension header
 *
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   HET = 64    |     HEL       |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
 * |                       Transfer Length                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    FEC Instance ID            |FEC Encoding ID Specific Format|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

/* 
 * FEC Encoding ID Specific Format for IDs 0, 128 and 130
 *  
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *                                 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                                 |     Encoding Symbol Length    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  Maximum Source Block Length                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 */

/* 
 * FEC Encoding ID Specific Format for ID 129
 *  
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *                                 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                                 |     Encoding Symbol Length    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Maximum Source Block Length  | Max. Num. of Encoding Symbols |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 */

int add_fti_0_128_130_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen,
#ifdef WIN32					 
					 ULONGLONG transferlen,
#else
					 unsigned long long transferlen,
#endif
					 unsigned short fec_inst_id, unsigned short eslen, unsigned int max_sblen);

int add_fti_129_lct_he(def_lct_hdr_t *def_lct_hdr, int hdrlen,
#ifdef WIN32					 
					 ULONGLONG transferlen,
#else
					 unsigned long long transferlen,
#endif

					 unsigned short fec_inst_id, unsigned short eslen,
					 unsigned short max_sblen, unsigned short mxnbofes);

/*
 * No-Operation extension header
 *
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      HET      |      HEL      |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
 * .                                                               . 
 * .           Header Extension Content (HEC)                      . 
 * .                                                               .
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

int add_nop_lct_he(void);

/*
 * Packet authentication extension header
 *
 * 0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      HET      |      HEL      |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
 * .                                                               . 
 * .           Header Extension Content (HEC)                      . 
 * .                                                               .
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

int add_auth_lct_he(void);

#endif /* _LCT_HDR_H_ */

