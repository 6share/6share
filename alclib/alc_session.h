/* $Author: peltotal $ $Date: 2004/09/20 09:20:50 $ $Revision: 1.31 $ */
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

#ifndef _ALC_SESSION_H_
#define _ALC_SESSION_H_

#include "defines.h"

/**** Enumerations ****/

enum alc_session_states { SActive, SExiting, STxStopped, SClosed };

/**** Typedefs ****/

typedef struct wanted_obj {
	
	struct wanted_obj *prev;
	struct wanted_obj *next;

#ifdef WIN32
	ULONGLONG toi;
	ULONGLONG toi_len;
#else
	unsigned long long toi;
	unsigned long long toi_len;
#endif

	unsigned short eslen;				/* encoding symbol length */
	unsigned int max_sblen;				/* Maximum-size source block length */
	int fec_inst_id;					/* identifier that maps to the FEC algorithm */
	short fec_enc_id;
	unsigned short max_nb_of_enc_symb;

#ifdef USE_ZLIB
	unsigned char content_enc_algo;	/* content encoding algorithm */
#endif

} wanted_obj_t;
                                                                                                                                              
typedef struct rx_fdt_instance {
        
        struct rx_fdt_instance *prev;
        struct rx_fdt_instance *next;
        unsigned int fdt_instance_id;
} rx_fdt_instance_t;

typedef struct tx_queue_struct {
	unsigned char *data;
	unsigned int datalen;
	unsigned int nb_tx_ch;
	struct tx_queue_struct *next;
} tx_queue_t;

typedef struct alc_session {
	
	int s_id;				/* ALC session identifier */
	int mode;				/* mode for ALC session (sender/receiver) */

#ifdef WIN32
	ULONGLONG tsi;					/* Transport Session Identifier */
	ULONGLONG starttime;			/* Session start time */
	ULONGLONG stoptime;				/* Session stop time */
#else
	unsigned long long tsi;			/* Transport Session Identifier */
	unsigned long long starttime;	/* Session start time */
	unsigned long long stoptime;	/* Session stop time */
#endif

	struct alc_channel *ch_list[MAX_CHANNELS_IN_SESSION];	/* channels in session */
	int nb_channel;											/* number of channels in session */
	int max_channel;										/* number of max channels in session */

	int nb_sending_channel;
	int nb_ready_channel;

	enum alc_session_states state;	/* state for muppet session */

	int addr_family;
	int addr_type;					/* Multicast (0) or Unicast (1) */

	int fdt_instance_id;			/* current fdt instance number */

#ifdef USE_RLC
	struct mad_rlc *rlc;
#endif

	/**** SENDER ****/

	int def_ttl;					/* Default value for time to live */
	int def_tx_rate;				/* Default value for transmission rate in kbit/s */
	unsigned short def_eslen;		/* Default value for encoding symbol length */
	unsigned int def_mxnbes;		/* Default value for maximum number of es that can be generated from one sb */
	unsigned int def_max_sblen;		/* Default value for maximum-size source block length */
	bool simul_losses;				/* Simulate packet losses */
	double loss_ratio1;
	double loss_ratio2;
	int def_fec_ratio;				/* FEC ratio percent */

	unsigned char def_fec_enc_id;		/* identifier that maps to the FEC scheme */
	unsigned short def_fec_inst_id;		/* identifier that maps to the FEC algorithm */
	int cc_id;							/* identifier that maps to the used congestion control */
	int use_fec_oti_ext_hdr;

	int big_file_mode;
	int verbosity;

#ifdef WIN32
	ULONGLONG sent_bytes;
#else
	unsigned long long sent_bytes;
#endif /* WIN32 */

	int a_flag;

	tx_queue_t *tx_queue_begin;
	tx_queue_t *tx_queue_end;
	unsigned int tx_queue_size;

	bool first_unit_in_loop;

	int encode_content;

	bool half_word;

	/**** RECEIVER ****/

#ifdef SSM
	bool ssm;
#endif

	char base_dir[MAX_LENGTH];		/* Base directory for downloaded files */
	char *src_addr;
	/* unsigned long src_addr;	source address for filtering traffic */
	
	unsigned int rx_objs;			/* number of objects received in this session */
	struct trans_obj *obj_list;		/* pointer to first object */
	struct trans_obj *fdt_list;             /* pointer to first fdt instance */
	wanted_obj_t *wanted_obj_list;	/* pointer to first wanted object */ 
	rx_fdt_instance_t *rx_fdt_instance_list;

#ifdef WIN32
	unsigned long rx_thread_id;
	unsigned long tx_thread_id;
#else
	pthread_t rx_thread_id;
	pthread_t tx_thread_id;
#endif /* WIN32 */

	unsigned int lost_packets;
	int accept_expired_fdt_inst;
} alc_session_t;

/**** Functions ****/

int open_alc_session(int mode,
#ifdef WIN32
					 ULONGLONG tsi,
					 ULONGLONG	starttime,
					 ULONGLONG	stoptime,
#else
					 unsigned long long tsi,
					 unsigned long long startime,
					 unsigned long long stoptime,
#endif
					 char *src_addr, int addr_family,
					 int addr_type, int ttl, unsigned char fec_enc_id, unsigned short fec_inst_id,
					 unsigned int max_sblen, unsigned short eslen, int cc_id, int use_fec_oti_ext_hdr,
					 int tx_rate, int big_file_mode, int rx_nb_channel, bool simul_losses, int fec_ratio,
					 int verbosity, char *base_dir, bool half_word, int accept_expired_fdt_inst,
					 double loss_ratio1, double loss_ratio2
#ifdef SSM
					 , bool use_ssm
#endif

#ifdef USE_ZLIB
					, int encode_content);
#else 
					);
#endif

void close_alc_session(int s_id);

alc_session_t* get_alc_session(int s_id);

int add_alc_channel(int s_id, char *port, char *addr, char *intface, char *intface_name);
 
int remove_alc_channel(int s_id, int ch_id);

int get_session_state(int s_id);
void set_session_state(int s_id, enum alc_session_states state);

int get_session_a_flag(int s_id);
void set_session_a_flag(int s_id);

unsigned int get_fdt_instance_id(int s_id);
void set_fdt_instance_id(int s_id, unsigned int instance_id);

void set_session_sent_bytes(int s_id, unsigned int sent_bytes);
void add_session_sent_bytes(int s_id, unsigned int sent_bytes);

#ifdef WIN32
ULONGLONG get_session_sent_bytes(int s_id);
#else        
unsigned long long get_session_sent_bytes(int s_id);
#endif

int set_wanted_object(int s_id,
#ifdef WIN32
	ULONGLONG toi,
    ULONGLONG toi_len,
#else
	unsigned long long toi,
    unsigned long long toi_len,
#endif
		unsigned short eslen, unsigned int max_sblen, int fec_inst_id,
		short fec_enc_id, unsigned short max_nb_of_enc_symb

#ifdef USE_ZLIB
		,unsigned char content_enc_algo);
#else
		);
#endif

wanted_obj_t* get_wanted_object(alc_session_t *s,
#ifdef WIN32
        ULONGLONG toi);
#else
        unsigned long long toi);
#endif

void remove_wanted_object(int s_id,
#ifdef WIN32
						  ULONGLONG toi);
#else
						  unsigned long long toi);
#endif

int set_received_instance(alc_session_t *s, unsigned int fdt_instance_id);
bool is_received_instance(alc_session_t *s, unsigned int fdt_instance_id);

char* get_session_basedir(int s_id);

#endif /* _ALC_SESSION_H_ */









