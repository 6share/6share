/* $Author: peltotal $ $Date: 2004/06/04 10:38:42 $ $Revision: 1.14 $ */
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

#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

/**** Typedefs ****/ 

typedef struct trans_unit {

	struct trans_unit	*prev;		/* pointer to previous unit */
	struct trans_unit	*next;		/* pointer to next unit */
	unsigned int		esid;		/* encoding symbol ID */
	unsigned short		len;		/* length of this TU (in bytes) */
	char				*data;		/* pointer to data buffer */

} trans_unit_t;

typedef struct trans_block {

	struct trans_block	*prev;				/* pointer to previous block */
	struct trans_block	*next;				/* pointer to next block */
	unsigned int		sbn;				/* source block number */
	struct trans_unit	*unit_list;			/* pointer to first unit for this block */
	unsigned int		nb_of_rx_units;		/* received units for this block */
	unsigned int		n;					/* number of units for this block */

	unsigned int		k;
	unsigned int		max_k;
	unsigned int		max_n;

	bool				completed;				/* Is this block completed */

} trans_block_t;

typedef struct trans_obj {

	struct trans_obj	*prev;					/* pointer to previous obj */
	struct trans_obj	*next;					/* pointer to next obj */
	struct trans_block	*block_list;			/* pointer to first block for this object */
	unsigned int		nb_of_created_blocks;	/* created blocks for this object */
    unsigned int		nb_of_ready_blocks;		/* number of ready blocks for this object */
	unsigned char 		fec_enc_id;				/* identifier that maps to the FEC scheme */
	unsigned short 		fec_inst_id;			/* identifier that maps to the FEC algorithm */
	unsigned char		cont_enc_algo;			/* content encoding algorithm */

#ifdef WIN32
	ULONGLONG		len;			/* length of this TO (in bytes) */			
	ULONGLONG		rx_bytes;		/* received bytes for this TO */
	ULONGLONG		toi;			/* transport object identifier */
#else
	unsigned long long	len;			
	unsigned long long	rx_bytes;
	unsigned long long toi;
#endif

	unsigned int		eslen;			/* Encoding symbol length */
	unsigned int		max_sblen;		/* Maximum size source block length */

	struct blocking_struct  *bs;

	char			*tmp_filename;		/* Temporary filename for this object */
	int			fd;			/* File descriptor to be used for file saving */

} trans_obj_t;


/**** Functions ****/

trans_obj_t* create_object(void);
trans_block_t* create_block(void);
trans_unit_t* create_units(unsigned int number);

void insert_object(trans_obj_t *to, alc_session_t *s, int type);
void insert_block(trans_block_t *tb, trans_obj_t *to);
int insert_unit(trans_unit_t *tu, trans_block_t *tb, trans_obj_t *to);
void free_object(trans_obj_t *to, alc_session_t *s, int type);

#endif /* _TRANSPORT_H_ */



