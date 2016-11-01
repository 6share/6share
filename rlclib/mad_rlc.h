/* $Author: peltotal $ $Date: 2004/05/11 06:41:21 $ $Revision: 1.6 $ */
/*
 *   MAD-RLC, implementation of RLC Congestion Control protocol
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
/*
 *	 Portions of code derived from MCL library by Vincent Roca et al.
 *   (http://www.inrialpes.fr/planete/people/roca/mcl/)
 *
 *   Copyright (c) 1999-2004 INRIA - Universite Paris 6 - All rights reserved
 *   (main author: Julien Laboure - julien.laboure@inrialpes.fr
 *                 Vincent Roca - vincent.roca@inrialpes.fr)
 */

#ifndef _MAD_RLC_H_
#define _MAD_RLC_H_

/**** Typedefs ****/

typedef struct rlc_hdr {

#ifdef  _BIT_FIELDS_LTOH
	unsigned char	reserved:7;		/* Unused, must be 0x55 (1010101) */
	unsigned char	sp:1;			/* Is this packet a Synchronisation Point (SP) ? */
#else
	unsigned char	sp:1;			/* Is this packet a Synchronisation Point (SP) ? */
	unsigned char	reserved:7;		/* Unused, must be 0x55 (1010101) */
#endif

	unsigned char	layer;			/* packet's layer (indice) */
	unsigned short	seqid;			/* packet's Sequence number (per layer sequence) */

} rlc_hdr_t;

typedef struct late_list {
	struct late_list	*next;		/* next late */
	unsigned short		seq_num;	/* RLC Sequence Number */
	double				losttime;	/* Time when considering lost */
} late_list_t;

typedef struct lost_list {
	struct lost_list *next;		/* next missing */
	int pkt_remaining;			/* Number of packets to receive before we forget this one */
} lost_list_t;

typedef struct mad_rlc {
	int sp_cycle;		/* Interval between 2 SPs at layer 0 (in µsec) */
	int pkt_timeout; 	/* Default Time To Wait for a late packet before assuming it's lost */
	int deaf_period;	/* Time for Deaf period after a dropped layer (in µsec) */

	int late_accepted;	/* if the amount of late packets between 2 SPs at top layer is
							   <= rlc_late_accepted then a layer can be added */

	int loss_accepted;	/* if the amount of lost packets between 2 SPs at top layer is
							   <= rlc_loss_accepted then a layer could be added */

	int loss_limit;		/* rlc_loss_limit / rlc_loss_timeout is the max loss rate for packet. */
	int loss_timeout;	/* if this rate is reached then we should drop the highest layer. */
	
	/* For each layer, value of current sequence number */
	unsigned short tx_layers_seq[MAX_CHANNELS_IN_SESSION];

	/* For each layer, time for the next SP */
	double tx_next_sp[MAX_CHANNELS_IN_SESSION];

	/* For each layer, =1 if we are waiting for the first packet */
	char rx_first_pkt[MAX_CHANNELS_IN_SESSION];

	/* For each layer, =1 if we are waiting for the first SP after deaf */
	char rx_first_sp[MAX_CHANNELS_IN_SESSION];

	/* For each layer, seq number of the next packet to receive */
	unsigned short rx_wait_for[MAX_CHANNELS_IN_SESSION];

	/* For each layer, list of missing seq number */
	late_list_t	rx_missing[MAX_CHANNELS_IN_SESSION];

	/* amount late packets since the last SP */
	unsigned short	rx_nblate_since_sp;

	/* amount of recent late packets */
	unsigned short	rx_nblate;

	/* amount lost packets since the last SP */
	unsigned short	rx_nblost_since_sp;

	/* amount of recent lost packets */
	unsigned short	rx_nblost;

	/* Current list of lost packets */
	lost_list_t	rx_lost;

	/* when in deaf period, specify deaf period end time */	
	double rx_deaf_wait;

	bool drop_highest_layer;

} mad_rlc_t;

/**** Functions ****/

int init_mad_rlc(alc_session_t *s);
void close_mad_rlc(alc_session_t *s);
double mad_rlc_next_sp(alc_session_t *s, int layer);
void mad_rlc_reset_tx_sp(alc_session_t *s);
int mad_rlc_fill_header(alc_session_t *s, rlc_hdr_t *rlc_hdr, int layer);
int mad_rlc_analyze_cci(alc_session_t *s, rlc_hdr_t *rlc_hdr);

#endif
