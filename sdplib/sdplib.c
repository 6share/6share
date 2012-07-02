/* $Author: peltotal $ $Date: 2004/12/27 13:47:08 $ $Revision: 1.8 $ */
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

#include <stdio.h>
#include <stdlib.h>

#include "sdplib.h"

/*
 * This function returns wanted attribute from sdp_t structure.
 *
 * Params:	sdp_t *sdp: Pointer to sdp_t structure,
 *			char *attr_name: Name of wanted attribute.
 *	
 * Return:	char*: Pointer to buffer containing value of wanted attribute,
 *			NULL: In error cases.
 */

char* sdp_attr_get(sdp_t *sdp, char *attr_name) {
	list_t *list;
	node_t *node;
	sdp_attribute_t *attr;
	int size;
	int i;

	list = sdp->a_attributes;
	size = list_size(list);

	if(size == 0) {
		return NULL;
	}

	node = list->node;
	attr = node->element;
    
	for(i = 0; i < size; i++) {
          	
		if(strcmp(attr_name, attr->a_att_field) == 0) {
			return attr->a_att_value;	
		}
		if(i < size - 1) {
			node = node->next;			
			attr = node->element;
		}
	}
	return NULL;
}

/*
 * This function returns FEC declaration structure list from sdp_t structure.
 *
 * Params:	sdp_t *sdp: Pointer to sdp_t structure.
 *	
 * Return:	fec_dec_t*: Pointer to FEC declaration structure list,
 *			NULL: In error cases.
 */

fec_dec_t* sdp_fec_dec_get(sdp_t *sdp) {
	list_t *list;
	node_t *node;
	sdp_attribute_t *attr;
	int size;
	int i;
	fec_dec_t *fec_dec = NULL;
	fec_dec_t *current = NULL;
	fec_dec_t *prev = NULL;

	list = sdp->a_attributes;
	size = list_size(list);

	if(size == 0) {
		return NULL;
	}

	node = list->node;
	attr = node->element;
    
	for(i = 0; i < size; i++) {
          	
		if(strcmp(attr->a_att_field, "FEC-declaration") == 0) {

			current = fec_dec_char2struct(attr->a_att_value);

			if(current == NULL) {
				fec_dec_free(fec_dec);
				return NULL;
			}

			if(prev == NULL) {
				fec_dec = current;
				prev = current;
			}
			else {
				prev->next = current;
				prev = current;
			}
		}

		if(i < size - 1) {
			node = node->next;			
			attr = node->element;
		}
	}

	return fec_dec;
}

/*
 * This function converts string FEC declaration to FEC declaration structure.
 *
 * Params:	char *fec_dec: Pointer to buffer containing FEC declaration string.
 *
 * Return:	fec_dec_t*: Pointer to FEC declaration structure,
 *			NULL: In error cases.
 */

fec_dec_t* fec_dec_char2struct(char *fec_dec) {
	fec_dec_t *fec_d;
	char *buf;
	
	if(!(fec_d = (fec_dec_t*)calloc(1, sizeof(fec_dec_t)))) {
		printf("Could not alloc memory for FEC-declaration structure!\n");
		return NULL;
	}

	buf = strtok(fec_dec, " ");

	fec_d->index = atoi(buf);

	buf = strtok(NULL, "=");
	buf = strtok(NULL, ";");

	fec_d->fec_enc_id = (short)atoi(buf);

	buf = strtok(NULL, "=");
	buf = strtok(NULL, " ");

	if(buf != NULL) {
		fec_d->fec_inst_id = atoi(buf);
	}
	else {
		fec_d->fec_inst_id = 0;
	}

	fec_d->next = NULL;
	
	return fec_d;
}

/*
 * This function frees FEC declaration structure.
 *
 * Params:	fec_dec_t *fec_dec: Pointer to FEC declaration structure.
 *
 * Return:	void
 *
 */

void fec_dec_free(fec_dec_t *fec_dec) {

	fec_dec_t *current;
	fec_dec_t *next;

	if(fec_dec == NULL) {
		return;
	}

	current = fec_dec;

	while(current->next != NULL) {
		next = current->next;
		free(current);
		current = next;
	}
}

/*
 * This function converts string source filter to source filter structure.
 *
 * Params:	char *src_filt: Pointer to buffer containing source filter string.
 *
 * Return:	sf_t*: Pointer to source filter structure,
 *			NULL: In error cases.
 */

sf_t* sf_char2struct(char *src_filt) {
	sf_t *sf;
	char *buf;
	
	if(!(sf = (sf_t*)calloc(1, sizeof(sf_t)))) {
		printf("Could not alloc memory for source filter structure!\n");
		return NULL;
	}

	buf = strtok(src_filt, " ");

	sf->filter_mode = (char*)calloc(1, strlen(buf)+1);
	strcpy(sf->filter_mode, buf);

	buf = strtok(NULL, " ");
	
	sf->net_type = (char*)calloc(1, strlen(buf)+1);
	strcpy(sf->net_type, buf);

	buf = strtok(NULL, " ");
	
	sf->addr_type = (char*)calloc(1, strlen(buf)+1);
	strcpy(sf->addr_type, buf);

	buf = strtok(NULL, " ");
	
	sf->dest_addr = (char*)calloc(1, strlen(buf)+1);
	strcpy(sf->dest_addr, buf);

	buf = strtok(NULL, " ");
	
	sf->src_addr = (char*)calloc(1, strlen(buf)+1);
	strcpy(sf->src_addr, buf);

	return sf;
}

/*
 * This function frees source filter structure.
 *
 * Params:	sf_t *sf: Pointer to source filter structure.
 *
 * Return:	void
 *
 */

void sf_free(sf_t *sf) {

	if(sf == NULL) {
		return;
	}
	free(sf->filter_mode);
	free(sf->net_type);
	free(sf->addr_type);
	free(sf->dest_addr);
	free(sf->src_addr);
}

