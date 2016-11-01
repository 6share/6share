/* $Author: peltotal $ $Date: 2004/06/04 10:38:42 $ $Revision: 1.8 $ */
/*
 *   MAD-Null-FEC, implementation of Compact No-Code FEC codec
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

#include "../alclib/inc.h"

/*
 * This function encodes source block data to transport block using Null-FEC.
 *
 * Params:	char *data: Pointer to data string to be segmented,
 *			ULONGLONG/unsigned long long len: Length of data string,
 *			unsigned int sbn: Source Block Number,
 *			unsigned short eslen: Encoding Symbol Length.
 *
 * Return:	trans_block_t *t: Pointer to transport block,
 *			NULL: In error cases.
 *
 */

trans_block_t* null_fec_encode_src_block(char *data, 
#ifdef WIN32
										ULONGLONG len
#else
										unsigned long long len
#endif
										, unsigned int sbn, unsigned short eslen) {
	
	trans_block_t *tr_block;	/* transport block struct */
	trans_unit_t *tr_unit;		/* transport unit struct */
	unsigned int nb_of_units;			/* number of units */

	unsigned int i;				/* loop variables */

#ifdef WIN32
	ULONGLONG data_left = len;
#else
	unsigned long long data_left = len;
#endif

	char *ptr;					/* pointer to left data */

	nb_of_units = (unsigned int)ceil((double)(unsigned int)len / (double)eslen);

	tr_block = create_block();

	if(tr_block == NULL) {
		return tr_block;
	}

	tr_unit = create_units(nb_of_units);

	if(tr_unit == NULL) {
		free(tr_block);
		return NULL;
	}

	ptr = data;

	tr_block->unit_list = tr_unit;
	tr_block->sbn = sbn;
	tr_block->n = nb_of_units;
	tr_block->k = nb_of_units;
		
	for(i = 0; i < nb_of_units; i++) {
		tr_unit->esid = i;
		tr_unit->len = data_left < eslen ? (unsigned short)data_left : eslen; /*min(eslen, data_left);*/

		/* Alloc memory for TU data */
		if(!(tr_unit->data = (char*)calloc(tr_unit->len, sizeof(char)))) {
			printf("Could not alloc memory for transport unit's data!\n");
			
			tr_unit = tr_block->unit_list;	

			while(tr_unit != NULL) {
				free(tr_unit->data);
				tr_unit++;
			}
	
			free(tr_block->unit_list);
			free(tr_block);
			return NULL;
		}

		memcpy(tr_unit->data, ptr, tr_unit->len);
		
		ptr += tr_unit->len;
		data_left -= tr_unit->len;
		tr_unit++;
	}

	return tr_block;
}

/*
 * This function decodes source block data to buffer using Null-FEC.
 *
 * Params:	trans_block_t *tr_block: Pointer to source block,
 *			ULONGLONG/unsigned long long *block_len: Pointer to length of block,
 *			unsigned short eslen: Encoding Symbol Length for this block.
 *
 * Return:	char*: Pointer to buffer which contains block's data,
 *			NULL: when memory could not be allocated.
 *
 */


char *null_fec_decode_src_block(trans_block_t *tr_block, 
#ifdef WIN32
								ULONGLONG *block_len
#else
								unsigned long long *block_len
#endif
								, unsigned short eslen) {

	char *buf = NULL; /* buffer where to construct the source block from data units */

    trans_unit_t *next_tu;
    trans_unit_t *tu;

#ifdef WIN32
	ULONGLONG len;
	ULONGLONG tmp;
#else
	unsigned long long len;
	unsigned long long tmp;
#endif

    len = eslen*tr_block->k;

    /* Allocate memory for buf */
    if(!(buf = (char*)calloc((unsigned int)(len + 1), sizeof(char)))) {
        printf("Could not alloc memory for buf!\n");
        return NULL;
    }

    tmp = 0;
	
	next_tu = tr_block->unit_list;
       		
	while(next_tu != NULL) {

        tu = next_tu;

		/*if(tu->data == NULL) {
			printf("SB: %i, esid: %i\n", tr_block->sbn, tu->esid);
			fflush(stdout);
		}*/

        memcpy((buf+(unsigned int)tmp), tu->data, tu->len);

        free(tu->data);
        tu->data = NULL;
        tmp += tu->len;

        next_tu = tu->next;
	}

	*block_len = len;

	return buf;
}

/*
 * This function decodes object to buffer using Null-FEC.
 *
 * Params:	trans_obj_t *tr_obj: Pointer to object,
 *			ULONGLONG/unsigned long long *data_len: Pointer to length of object,
 *			alc_session_t *s: Pointer to session.
 *
 * Return:	char*: Pointer to buffer which contains object's data,
 *			NULL: when memory could not be allocated.
 *
 */

char *null_fec_decode_object(trans_obj_t *to,
#ifdef WIN32
							 ULONGLONG *data_len
#else
							 unsigned long long *data_len
#endif
							 , alc_session_t *s) {
	
	char *object = NULL;
	char *block = NULL;

	trans_block_t *tb;

#ifdef WIN32
	ULONGLONG to_data_left;
	ULONGLONG len;
	ULONGLONG block_len;
	ULONGLONG position;
#else
	unsigned long long to_data_left;
	unsigned long long len;
	unsigned long long block_len;
	unsigned long long position;
#endif
	
	/* Allocate memory for buf */
	if(!(object = (char*)calloc((unsigned int)(to->len+1), sizeof(char)))) {
		printf("Could not alloc memory for buf!\n");
		return NULL;
	}
	
	to_data_left = to->len;

	tb = to->block_list;
	position = 0;
	
	while(tb != NULL) {
		block = null_fec_decode_src_block(tb, &block_len, (unsigned short)to->eslen);

		/* the last packet of the last source block might be padded with zeros */
		len = to_data_left < block_len ? to_data_left : block_len;

		memcpy(object+(unsigned int)position, block, (unsigned int)len);
		position += len;
		to_data_left -= len;

		free(block);
		tb = tb->next;
	}
	
	*data_len += to->len;
	return object;
}

