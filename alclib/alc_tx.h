/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.10 $ */
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

#ifndef _ALC_TX_H_
#define _ALC_TX_H_

/**** Functions ****/

int alc_send(int s_id, int ch_id, char *buf,
#ifdef WIN32
			 ULONGLONG buf_len,
			 ULONGLONG toi,
			 ULONGLONG toilen,
#else
			 unsigned long long buf_len,
			 unsigned long long toi,
			 unsigned long long toilen,
#endif
			 unsigned short eslen, unsigned int max_sblen, unsigned int sbn, 
			 unsigned char fec_enc_id, unsigned short fec_inst_id, int is_last_block);

int alc_send2(int s_id, char *buf,
#ifdef WIN32
			 ULONGLONG buf_len,
			 ULONGLONG toi,
			 ULONGLONG toilen,
#else
			 unsigned long long buf_len,
			 unsigned long long toi,
			 unsigned long long toilen,
#endif
			 unsigned short eslen, unsigned int max_sblen, unsigned int sbn,
			 unsigned char fec_enc_id, unsigned short fec_inst_id);

int send_session_close_packet(int s_id, int ch_id);

void* tx_thread(void *s);

#endif /* _ALC_TX_H_ */
