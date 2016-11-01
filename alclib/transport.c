/* $Author: peltotal $ $Date: 2004/06/04 10:38:42 $ $Revision: 1.12 $ */
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
 * This function creates new transport object structure.
 *
 * Params:	void
 *
 * Return:	trans_obj_t*: Pointer to created transport object in success,
 *			NULL otherwise.
 *
 */

trans_obj_t* create_object(void) {
	
	trans_obj_t *obj = NULL;

	if (!(obj = (trans_obj_t*)calloc(1, sizeof(trans_obj_t)))) {
		printf("Could not alloc memory for transport object!\n");
		return NULL;
	}

	return obj;
}

/*
 * This function creates new transport block structure.
 *
 * Params:	void
 *
 * Return:	trans_block_t*: Pointer to created transport block in success,
 *			NULL otherwise.
 *
 */

trans_block_t* create_block(void) {
 
	trans_block_t *block = NULL;

	if (!(block = (trans_block_t*)calloc(1, sizeof(trans_block_t)))) {
		printf("Could not alloc memory for transport block!\n");
		return NULL;
	}

	block->completed = false;
	
	return block;
}

/*
 * This function creates new transport unit structure/structures.
 *
 * Params:	unsigned int number: Number of units to be created.
 *
 * Return:	trans_unit_t*: Pointer to created transport unit in success,
 *			NULL otherwise.
 *
 */

trans_unit_t* create_units(unsigned int number) {
	
	trans_unit_t *unit = NULL;

	if (!(unit = (trans_unit_t*)calloc(number, sizeof(trans_unit_t)))) {
		printf("Could not alloc memory for %i transport units!\n", number);
	}

	return unit;
}

/*
 * This function inserts transport object to session.
 *
 * Params:	trans_obj_t *to: Pointer to transport object to be inserted,
 *			alc_session_t *s: Pointer to session,
 *			int type: Type of object to be inserted (0 = FDT Instance, 1 = normal object).
 *
 * Return:	void	
 *
 */

void insert_object(trans_obj_t *to, alc_session_t *s, int type) {
	
	trans_obj_t *tmp;
	
	if(type == 0) {
		tmp = s->fdt_list;
	}
	else {
		tmp = s->obj_list;
	}

	if(tmp == NULL) {

  		if(type == 0) {
        	        s->fdt_list = to;
        	}	
        	else {
                	s->obj_list = to;
       	 	}
	}
	else {
		for(;;) {
			if(to->toi < tmp->toi) {

				if(tmp->prev == NULL) {

					to->next = tmp;
					to->prev = tmp->prev;
				
					tmp->prev = to;

                if(type == 0) {
                        s->fdt_list = to;
                }
                else {
                        s->obj_list = to;
                }

				}
				else {

					to->next = tmp;
					to->prev = tmp->prev;
				
					tmp->prev->next = to;
					tmp->prev = to;
				}
				break;
			}

			if(tmp->next == NULL) {

				to->next = tmp->next;
				to->prev = tmp;
				
				tmp->next = to;
				break;
			}

			tmp = tmp->next;
		}
	}
}

/*
 * This function inserts transport block to transport object.
 *
 * Params:	trans_block_t *tb: Pointer to transport block to be inserted,
 *			trans_obj_t *to: Pointer to tranport object.
 *
 * Return:	void
 *
 */

void insert_block(trans_block_t *tb, trans_obj_t *to) {
	
	trans_block_t *tmp;

	tmp = to->block_list;

	to->nb_of_created_blocks++;

	if(tmp == NULL) {
		to->block_list = tb;
	}
	else {
		for(;;) {
			if(tb->sbn < tmp->sbn) {
				
				if(tmp->prev == NULL) {
					tb->next = tmp;
					tb->prev = tmp->prev;
				
					tmp->prev = tb;

					to->block_list = tb;
				}
				else {
					tb->next = tmp;
					tb->prev = tmp->prev;
				
					tmp->prev->next = tb;
					tmp->prev = tb;
					
				}
				break;
			}
			if(tmp->next == NULL) {
				tb->next = tmp->next;
				tb->prev = tmp;
				
				tmp->next = tb;
				break;
			}
			tmp = tmp->next;
		}
	}
}

/*
 * This function inserts transport unit to transport block.
 *
 * Params:	trans_unit_t *tu: Pointer to transport unit to be inserted,
 *			trans_block_t *tb: Pointer to transport block,
 *			trans_obj_t *tu: Pointer to transport object.
 *
 * Return:	int: 0 when transport unit is inserted, 1 when duplicated transport unit.
 *
 */

int insert_unit(trans_unit_t *tu, trans_block_t *tb, trans_obj_t *to) {

	trans_unit_t *tmp;
	/*char temp[1500];*/

	int retval = 0;

	tmp = tb->unit_list;

	if(tmp == NULL) {

		/*printf("sbn: %i, esid: %i\n", tb->sbn, tu->esid);

		memset(temp, 0, 1500);
		memcpy(temp, tu->data, tu->len);

		printf("%s\n", temp);*/

		to->rx_bytes += tu->len;
		
		/* for percentage counter */
		if(to->rx_bytes > to->len) {
			to->rx_bytes = to->len;		
		}

		tb->nb_of_rx_units++;

		tb->unit_list = tu;
	}
	else {
		for(;;) {
			if(tu->esid < tmp->esid) {
				/* Delayed unit */

				to->rx_bytes += tu->len;

				if(to->rx_bytes > to->len) {
					to->rx_bytes = to->len;		
				}

				tb->nb_of_rx_units++;
				
				if(tmp->prev == NULL) {
					tu->next = tmp;
					tu->prev = tmp->prev;
				
					tmp->prev = tu;

					tb->unit_list = tu;
				}
				else {
					tu->next = tmp;
					tu->prev = tmp->prev;
				
					tmp->prev->next = tu;
					tmp->prev = tu;
					
				}
				break;
			}
			else if(tu->esid == tmp->esid) {
				/* Duplicated unit */
				retval = 1;
				break;
			}
			if(tmp->next == NULL) {
				/* Last unit (normal order) */

				to->rx_bytes += tu->len;
			

				if(to->rx_bytes > to->len) {
					to->rx_bytes = to->len;		
				}

				tb->nb_of_rx_units++;

				tu->next = tmp->next;
				tu->prev = tmp;
				
				tmp->next = tu;
				break;
			}
			tmp = tmp->next;
		}
	}

	return retval;
}

/*
 * This function frees transport object structure.
 *
 * Params:	trans_obj_t *to: Pointer to transport object to be freed,
			alc_session_t *s: Pointer to session,
			int type: Type of object to be freed (0 = FDT Instance, 1 = normal object).
 *
 * Return:	void
 *
 */

void free_object(trans_obj_t *to, alc_session_t *s, int type) {
	
	trans_block_t *tb;
	trans_block_t *next_tb;
	trans_unit_t *tu;
	trans_unit_t *next_tu;

	next_tb = to->block_list;
	
	while(next_tb != NULL) {
		tb = next_tb;

		next_tu = tb->unit_list;

		while(next_tu != NULL) {
			tu = next_tu;
			
			if(tu->data != NULL) {	
				free(tu->data);
				tu->data = NULL;
			}
				
			next_tu = tu->next;
			free(tu);
		} 

		next_tb = tb->next;
		free(tb);
	}

	free(to->bs);

	if(to->next != NULL) {
		to->next->prev = to->prev;
	}
	if(to->prev != NULL) {
		to->prev->next = to->next;
	}
	if(((type == 0)&&(to == s->fdt_list))) {
		s->fdt_list = to->next;
	}
	else if(((type == 1)&&(to == s->obj_list))) {
		s->obj_list = to->next;
	}

	if(type == 1) {
		close(to->fd);
	}
        
	if(to->tmp_filename != NULL) {
	    free(to->tmp_filename);
        to->tmp_filename = NULL;
    }
	
	free(to);	
}

