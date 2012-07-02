/* $Author: peltotal $ $Date: 2004/06/04 10:38:42 $ $Revision: 1.11 $ */
/*
 *   MAD-RS-FEC, implementation of Reed-Solomon FEC codec
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
#include "fec.h"
/*
 * This function encodes source block data to transport block using Reed-Solomon FEC.
 *
 * Params:	char *data: Pointer to data string to be segmented,
 *			ULONGLONG/unsigned long long len: Length of data string,
 *			unsigned int sbn: Source Block Number,
 *			unsigned short eslen: Encoding Symbol Length,
 *			int rs: FEC ratio percent,
 *			unsigned int max_sblen: Maximum Source Block Length.
 *
 * Return:	trans_block_t *t: Pointer to transport block,
 *			NULL: In error cases
 *
 */


trans_block_t* rs_fec_encode_src_block(char *data, 
#ifdef WIN32
								ULONGLONG len
#else
								unsigned long long len
#endif	   
								, unsigned int sbn,
								unsigned short eslen, int rs, unsigned int max_sblen) {
	
	trans_block_t *tr_block;		/* transport block struct */
	trans_unit_t *tr_unit;			/* transport unit struct */
	unsigned int i;					/* loop variables */
	char *ptr;						/* pointer to left data */
	void *code;
	char *src[GF_SIZE];
	unsigned int k;					/* number of data units */
	unsigned int n;					/* total number of units */
	unsigned int max_k;
	unsigned int max_n;

#ifdef WIN32
	ULONGLONG data_left = len;
#else
	unsigned long long data_left = len;
#endif

	div_t div_k;
	div_t div_max_n;
	div_t div_n;

	max_k = max_sblen;
	
	div_k = div((unsigned int)len, eslen);
	
	if(div_k.rem == 0) {
		k = (unsigned int)div_k.quot;
	}
	else {
		k = (unsigned int)div_k.quot + 1;
	}

	div_max_n = div((max_k * (100 + rs)), 100);
	max_n = (unsigned int)div_max_n.quot;

	div_n = div((k * max_n), (max_k));
	n = (unsigned int)div_n.quot;

	code = 	fec_new(k, n);

	tr_block = create_block();

	if(tr_block == NULL) {
		return tr_block;
	}

	tr_unit = create_units(n);

	if(tr_unit == NULL) {
		free(tr_block);
		return NULL;
	}

	ptr = data;

	tr_block->unit_list = tr_unit;
	tr_block->sbn = sbn;
	tr_block->n = n;

	tr_block->k = k;
	tr_block->max_k = max_k;
	tr_block->max_n = max_n;
	
	for(i = 0; i < k; i++) {
		
		tr_unit->esid = i;
		tr_unit->len = data_left < eslen ? (unsigned short)data_left : eslen; /*min(eslen, (unsigned int)data_left);*/

		/* Alloc memory for TU data */
		if(!(tr_unit->data = (char*)calloc(eslen, sizeof(char)))) {
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

		src[i] = tr_unit->data;
		
		ptr += tr_unit->len;
		data_left -= tr_unit->len;
		tr_unit++;
	}

	/* let's create FEC units */

	for (i = 0; i < (n - k); i++) {

		tr_unit->esid = k+i;
		tr_unit->len = eslen;

		/* Alloc memory for TU data */
		if(!(tr_unit->data = (char*)calloc(eslen, sizeof(char)))) {
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

		fec_encode(code, (void**)src, tr_unit->data, k+i, eslen);

		tr_unit++;
	}

	fec_free(code);

	return tr_block;
}

/*
 * This function decodes source block data to buffer using Reed-Solomon FEC.
 *
 * Params:      trans_block_t *tr_block: Pointer to source block,
 *              ULONGLONG/unsigned long long *block_len: Pointer to length of block,
 *				unsigned short eslen: Encoding Symbol Length for this block,
 *
 * Return:      char*: Pointer to buffer which contains block's data,
 *              NULL: when memory could not be allocated
 *
 */

char *rs_fec_decode_src_block(trans_block_t *tr_block, 
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
#else
	unsigned long long len;
#endif
	
	void	*code;
	char	*dst[GF_SIZE];
	int		index[GF_SIZE];
	unsigned int i = 0;
	unsigned int k;						/* number of data units */
	unsigned int n;						/* total number of units */
	div_t div_n;

	k = tr_block->k;

	div_n = div((k * tr_block->max_n), tr_block->max_k);
	n= (unsigned int)div_n.quot;

	len = eslen*tr_block->k;

	code = 	fec_new(k, n);

	/* Allocate memory for buf */
	if(!(buf = (char*)calloc((unsigned int)(len + 1), sizeof(char)))) {
		printf("Could not alloc memory for buf!\n");
		*block_len = -1;
		return NULL;
	}
	
	next_tu = tr_block->unit_list;
       		
	while(next_tu != NULL) {
                        
		tu = next_tu;
						
		dst[i] = tu->data;
		index[i] = tu->esid;
 
		next_tu = tu->next;
		i++;
	}

	/* Let's decode source block using Reed-Solomon FEC */

	fec_decode(code, (void**)dst, index, eslen);

	fec_free(code);

	/* Copy decoded encoding symbols to buffer */

	for(i = 0; i < k; i++) {
		memcpy(buf + i*eslen, dst[i], eslen);
	}

	next_tu = tr_block->unit_list;
       		
	while(next_tu != NULL) {           
		tu = next_tu;		
        free(tu->data);
		tu->data = NULL;              
		next_tu = tu->next;
	}

	*block_len = len;

	return buf;
}

/*
 * This function decodes object to buffer using Reed-Solomon FEC.
 *
 * Params:	trans_obj_t *tr_obj: Pointer to object,
 *			ULONGLONG/unsigned long long *datalen: Pointer to length of object,
 *			alc_session_t *s: Pointer to session.
 *
 * Return:	char*: Pointer to buffer which contains object's data,
 *			NULL: when memory could not be allocated.
 *
 */

char *rs_fec_decode_object(trans_obj_t *to,
#ifdef WIN32
							ULONGLONG *data_len
#else
							unsigned long long *data_len
#endif
							, alc_session_t *s) {

	char *object = NULL; /* buffer where to construct the object */
	char *block = NULL; /* buffer where to contruct the object */
	
	trans_block_t *tb;

#ifdef WIN32
	ULONGLONG len;
	ULONGLONG position;
	ULONGLONG to_data_left;
	ULONGLONG block_len;
#else
	unsigned long long len;
	unsigned long long position;
	unsigned long long to_data_left;
	unsigned long long block_len;
#endif

	/* Allocate memory for object */
	if(!(object = (char*)calloc((unsigned int)(to->len + 1), sizeof(char)))) {
		printf("Could not alloc memory for object!\n");
		*data_len = -1;
		return NULL;
	}

	position = 0;
	to_data_left = to->len;
	
	tb = to->block_list;
	
	while(tb != NULL) {

		block = rs_fec_decode_src_block(tb, &block_len, (unsigned short)to->eslen);

		/* the last packet of the last source block might be padded with zeros */
		len = to_data_left < block_len ? to_data_left : block_len;

		memcpy(object+(unsigned int)position, block, (unsigned int)len);
		position += len;
		to_data_left -= len;

		free(block);
		block = NULL;

		tb = tb->next;
	}

	*data_len = to->len;	
	return object;
}
