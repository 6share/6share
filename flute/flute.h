/* $Author: peltotal $ $Date: 2004/09/20 09:20:50 $ $Revision: 1.32 $ */
/*
 *   MAD-FLUTE, implementation of FLUTE protocol
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

#ifndef _FLUTE_H_
#define _FLUTE_H_

#include "../alclib/inc.h"
#include "../sdplib/sdplib.h"

#include "fdt.h"

/**** Typedefs ****/

typedef struct arguments{
  
	/**** Common ****/

#ifdef WIN32
	ULONGLONG tsi;					/* Transport Session Identifier */
	ULONGLONG starttime;			/* Session start time */		
	ULONGLONG stoptime;				/* Session stop time */
#else
	unsigned long long tsi;			/* Transport Session Identifier */
	unsigned long long starttime;	/* Session start time */		
	unsigned long long stoptime;	/* Session stop time */
#endif

	char *port;						/* Base channel port number  */ 
	char *addr;						/* Base channel Mcast address */
	char *intface;					/* Local interface to bind */
	char *intface_name;				/* Name/index of local interface for IPv6 multicast join */

	int addr_family;				/* Address family */
	int addr_type;					/* multicast or unicast */
	int mode;						/* Sender or receiver? */
	int cont;						/* Continuous transmission */  
	char file_path[MAX_LENGTH];		/* File to send/recv */
	int nb_channel;					/* Number of channels */
	int cc_id;						/* Congestion control */
	int big_file_mode;				/* Receive big files */
	int verbosity;					/* Verbosity level */
	int logfd;						/* Log file descriptor */

	/**** Receiver ****/		
  
	char *srcaddr;					/* Source address for filtering traffic */
	char base_dir[MAX_LENGTH];		/* Base directory for downloaded files */
	int rx_automatic;				/* Download files defined in fdt automatically */
	int openfile;					/* open received file automatically */

#ifdef USE_SDPLIB
	char sdp_file[MAX_LENGTH];		/* SDP file (SDP based recv) */
	sdp_t *sdp;						/* sdp */
	sf_t *src_filt;					/* sdp ext */
#endif

	int file_list_mode;
	int accept_expired_fdt_inst;
  
	/***** Sender ****/

#ifdef WIN32
	ULONGLONG toi;					/* Transport Object Identifier */
#else
	unsigned long long toi;			/* Transport Object Identifier */
#endif

	int txrate;						/* Transmission rate in kbit/s */
	int ttl;						/* Time to Live */
	int nb_tx;						/* Number of time to send the file/directory */
	bool simul_losses;				/* Simulate packet losses */
	double loss_ratio1;				/* Packet loss ratio 1 */
	double loss_ratio2;				/* Packet loss ratio 2 */
	int fec_ratio;					/* FEC ratio percent */
	unsigned short eslen;			/* Encoding symbol length */
	unsigned int max_sblen;			/* Maximum Source block length */
	unsigned char fec_enc_id;       /* FEC Encoding ID*/
	unsigned short fec_inst_id;     /* FEC Instance ID*/
	int files_in_fdt_inst;			/* Number of files defined in FDT Instance */
	char fdt_file[MAX_LENGTH];		/* FDT file (FDT based send) */
	int use_fec_oti_ext_hdr;		/* Include FEC OTI extension header to FLUTE header */

#ifdef USE_ZLIB
	int encode_content;				/* Encode content using zlib library 0 = no, 1 = FDT, 2 = FDT and files */
#endif

#ifdef SSM
	bool use_ssm;					/* Use Source Specific Multicast */
#endif
	bool half_word;
} arguments_t;

/**** Functions ****/

int flute_receiver_prepare(arguments_t *a, int *s_id, int *ch_id);
int flute_sender_prepare(arguments_t *a, int *s_id, int *ch_id);

int sender_in_file_mode(int *s_id, int *ch_id, arguments_t *a);
int sender_in_fdt_based_mode(int *s_id, int *ch_id, arguments_t *a);

int receiver_in_fdt_based_mode(int *s_id, arguments_t *a);
int receiver_in_automatic_mode(int *s_id, arguments_t *a, fdt_th_args_t *fdt_th_args);
int receiver_in_ui_mode(int *s_id, arguments_t *a, fdt_th_args_t *fdt_th_args);
int receiver_in_file_list_mode(int *s_id, arguments_t *a);
int receiver_in_wild_card_mode(int *s_id, arguments_t *a, char *filetoken);
int receiver_in_object_mode(int *s_id, arguments_t *a);

int parse_sdp_file(arguments_t *a, char addrs[MAX_CHANNELS_IN_SESSION][MAX_LENGTH],
				   char ports[MAX_CHANNELS_IN_SESSION][MAX_LENGTH]);

#endif
