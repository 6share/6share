/* $Author: peltotal $ $Date: 2004/08/16 06:06:06 $ $Revision: 1.14 $ */
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

#ifndef _ALC_CHANNEL_H_
#define _ALC_CHANNEL_H_

/**** Typedefs ****/

typedef struct alc_channel {
	
	int ch_id;							/* channel identifier */
	struct alc_session *s;				/* pointer to parent session */

	char *port;
	char *addr;
	char *intface;
	char *intface_name;
	
	int tx_rate;						/* transmission rate in kbit/s on this channel */

	int nb_tx_units;					/* number of sent units per one loop */
	bool start_sending;
	bool ready;
	int wait_after_sp;					/* wait this number of loops before start sending */

	bool previous_lost;

#ifdef WIN32

	SOCKET	rx_sock;					/* rx socket */
	/*SOCKET	mc_sock;*/				/* for join/leave */
	SOCKET	tx_sock;					/* tx socket */	
#else
	int	rx_sock;						/* rx socket */
	int	tx_sock;						/* tx socket */	
#endif

#ifdef SSM
	struct ip_mreq_source source_imr;	/* for SSM join/leave */

#ifdef LINUX
	struct group_source_req greqs;		/* for MLDv2 SSM join/leave */
#endif

#endif

	struct ip_mreq imr;					/* for join/leave */
	struct ipv6_mreq imr6;

	struct sockaddr_in remote;			/* remote mcast addr */
	struct sockaddr_in6 remote6;		/* used when joining to mcast group */
	struct addrinfo *addrinfo;			
	struct tx_queue_struct *queue_ptr;

} alc_channel_t;

/**** Functions  ****/

int open_alc_channel(alc_channel_t *ch, alc_session_t *s, char *port,
					 char *addr, char *intface, char *intface_name, int tx_rate); 
int close_alc_channel(alc_channel_t *ch, alc_session_t *s);

#endif /* _ALC_CHANNEL_H_ */
