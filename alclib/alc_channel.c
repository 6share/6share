/* $Author: peltotal $ $Date: 2004/08/18 06:27:46 $ $Revision: 1.14 $ */
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
 * This function creates and initializes a new channel.
 *
 * Params:	alc_channel_t *ch: Pointer to channel,
 *			alc_session_t *s: Pointer to session,
 *			char *port: Pointer to port string,
 *			char *addr: Pointer to address string,
 *			char *intface: Pointer to interface string,
 *			char *intface_name: Pointer to the name of interface,
 *			int txrate: Transmission rate
 *
 * Return:	int: Identifier for created channel in success, -1 otherwise
 *
 */

int open_alc_channel(alc_channel_t *ch, alc_session_t *s, char *port,
					 char *addr, char *intface, char *intface_name, int tx_rate) {
	
	int ret_val;

	if (!(ch = (alc_channel_t*)calloc(1, sizeof(alc_channel_t)))) {
		printf("Could not alloc memory for  alc channel!\n");
		return -1;
	}

	ch->ch_id = s->nb_channel; /* session level identifier for channel */
	ch->s = s;

	if(s->mode == SENDER) {
		if(ch->ch_id == 0) {

			if(ch->s->cc_id == Null) {
				ch->tx_rate = tx_rate;
				ch->nb_tx_units = 1;
				ch->ready = false;
				
				s->nb_sending_channel++;
			}

#ifdef USE_RLC
			else if(ch->s->cc_id == RLC) {
				ch->tx_rate = tx_rate;
				ch->nb_tx_units = 1;
				ch->start_sending = true;
				ch->ready = false;
				ch->wait_after_sp = 0;

				s->nb_sending_channel++;
			}
#endif
		}
		else {

			if(ch->s->cc_id == Null) {
				ch->tx_rate = tx_rate * (int)pow(2.0, (double)(ch->ch_id - 1));
				ch->nb_tx_units = (int)pow(2.0, (double)(ch->ch_id - 1));
				ch->ready = false;

				s->nb_sending_channel++;
			}
			
#ifdef USE_RLC
			else if(ch->s->cc_id == RLC) {
				ch->tx_rate = tx_rate * (int)pow(2.0, (double)(ch->ch_id - 1));
				ch->nb_tx_units = (int)pow(2.0, (double)(ch->ch_id - 1));
				ch->start_sending = false;
				ch->ready = false;
				ch->wait_after_sp = RLC_WAIT_AFTER_SP;
			}
#endif
		}

		ch->addr = addr;
		ch->port = port;
		ch->intface = intface;
		//ch->intface_name = intface_name;

		ch->previous_lost = false;
	}
	else if(s->mode == RECEIVER) {
		ch->addr = addr;
		ch->port = port;
		ch->intface = intface;
		ch->intface_name = intface_name;
	}
		
	ret_val = init_alc_socket(ch);
	
	if(ret_val < 0) {
		return ret_val;
	}
		
	s->ch_list[s->nb_channel] = ch;
	s->nb_channel++;

	return ch->ch_id;
}

/*
 * This function closes existing channel.
 *
 * Params:	alc_channel_t *ch: Pointer to channel to be closed,
 *			alc_session_t *s: Pointer to session
 *
 * Return:	int: 0 when closing ok, -1 otherwise
 *
 */

int close_alc_channel(alc_channel_t *ch, alc_session_t *s) {
	
	int ret_val;
	int ch_id = ch->ch_id;

	ret_val = close_alc_socket(ch);

	/* trying again if ret_val == -1 */
	if(ret_val == -1) {
		ret_val = close_alc_socket(ch);
	}	

	freeaddrinfo(ch->addrinfo);
	free(ch);
	s->ch_list[ch_id] = NULL;

	s->nb_channel--;

	return ret_val;
}

