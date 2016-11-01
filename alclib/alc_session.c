/* $Author: peltotal $ $Date: 2004/09/20 09:20:50 $ $Revision: 1.40 $ */
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

/**** Set global variables ****/
                                                                                                                                       
struct alc_session *alc_session_list[MAX_ALC_SESSIONS];
int nb_alc_session = 0;

/*
 * This function creates and initializes new session.
 *
 * Params:	int mode: SENDER or RECEIVER,
 *			ULONGLOLNG/unsigned long long tsi: Transport Session Identifier,
 *			ULONGLOLNG/unsigned long long starttime: Session start value,
 *			ULONGLOLNG/unsigned long long stoptime: Session stop value,
 *			char *src_addr: Source address with receiver (NULL with sender),
 *			int family: IP address family (IPv4 or IPv6),
 *			int ttl: Default Time To Live,
 *			unsigned char fec_enc_id: Default FEC Encoding ID,
 *			unsigned short fec_inst_id: Default FEC Instance ID for FEC Encoding IDs >= 128,
 *			unsigned int max_sblen: Default Maximum-Size Source Block Length,
 *			int cc_id: Used congestion control (0 = none, 1 = MAD_CC),
 *			int use_fec_oti_ext_hdr: Use FEC OTI extension header in file objects (1 = yes, 0 = no),
 *			int txrate: Default Transmission rate,
 *			int big_file_mode: Receive big files,
 *			int rx_nb_channel: Maximum number of used channel in receiver,
 *			bool simul_losses: Simulate packet losses in sender,
 *			int fec_ratio: FEC ratio percent,
 *			int verbosity: Verbosity level,
 *			char *base_dir: Base directory for session,
 *			bool half_word: Use half_word in sender,
 *			int accept_expired_fdt_inst: Accept expired FDT Instances in receiver,
 *          double loss_ratio1: Simulated packet loss ratio when previous packet was sent, 
 *          double loss_ratio2: Simulated packet loss ratio when previous packet was not sent,
 *			bool use_ssm: Use Source Specific Multicast (Receiver only),
 *			int encode_content: Encode content (Sender only, ZLIB format for FDT and GZIP format for file).
 *
 * Return:	int: Identifier for created session in success, -1 otherwise
 *
 */

int open_alc_session(int mode,
#ifdef WIN32
					 ULONGLONG tsi,
					 ULONGLONG starttime,
					 ULONGLONG stoptime,
#else
					 unsigned long long tsi,
					 unsigned long long starttime,
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
					 , int encode_content) {
#else 
					 ) {
#endif

#ifndef WIN32
	pthread_attr_t attr;
#endif
	alc_session_t *s;
	int i;

#ifdef USE_RLC
	int ret_val;
#endif

	char fullpath[MAX_PATH];

	if(!lib_init) {
		alc_init();
		/* alc session list initialization */
        memset(alc_session_list, 0, MAX_ALC_SESSIONS * sizeof(alc_session_t*));
	}
	
	if(nb_alc_session >= MAX_ALC_SESSIONS) {
		/* Could not create new alc session */
		printf("Could not create new alc session: too many sessions!\n");
		return -1;
	}

	if (!(s = (alc_session_t*)calloc(1, sizeof(alc_session_t)))) {
		printf("Could not alloc memory for alc session!\n");
		return -1;
	}

	s->mode = mode;
	s->tsi = tsi;
	s->state = SActive;
	s->addr_family = addr_family;
	s->addr_type = addr_type;
	s->cc_id = cc_id;

#ifdef USE_RLC
	if(s->cc_id == RLC) {
		ret_val = init_mad_rlc(s);
	
		if(ret_val < 0) {
			return ret_val;
		}
	}
#endif

	s->big_file_mode = big_file_mode;
	s->verbosity = verbosity;

	s->starttime = starttime;
	s->stoptime = stoptime;
	
	if(mode == SENDER) {

		memcpy(s->base_dir, base_dir, strlen(base_dir));
			
		s->nb_channel = 0;
		s->max_channel = MAX_CHANNELS_IN_SESSION;
		s->simul_losses = simul_losses;
		s->loss_ratio1 = loss_ratio1;
		s->loss_ratio2 = loss_ratio2;
		s->def_fec_ratio = fec_ratio; 
		s->sent_bytes = 0;
		s->a_flag = 0;
		s->fdt_instance_id = 0; /*uplimit 16777215;*/
		s->def_ttl = ttl;
		s->def_fec_enc_id = fec_enc_id;
		s->def_fec_inst_id = fec_inst_id;
		s->def_max_sblen = max_sblen;
		s->def_eslen = eslen;
		s->use_fec_oti_ext_hdr = use_fec_oti_ext_hdr;
		s->def_tx_rate = tx_rate;

		s->tx_queue_begin = NULL;
		s->tx_queue_end = NULL;
		s->tx_queue_size = 0;

		s->first_unit_in_loop = true;
		s->nb_ready_channel = 0;
		s->nb_sending_channel = 0;

		s->half_word = half_word;
#ifdef USE_ZLIB
		s->encode_content = encode_content;
#endif

	}

	if(mode == RECEIVER) {

#ifdef SSM
		s->ssm = use_ssm;
#endif

#ifdef WIN32
	
		memset(fullpath, 0, MAX_PATH);

		if(_fullpath(fullpath, base_dir, MAX_PATH) != NULL) {
			memcpy(s->base_dir, fullpath, strlen(fullpath));
		}
		else {
			memcpy(s->base_dir, base_dir, strlen(base_dir));
		}		
#else
		memset(fullpath, 0, MAX_PATH);
	
		if(getcwd(fullpath, MAX_PATH) != NULL) {
			memcpy(s->base_dir, fullpath, strlen(fullpath));
			strcat(s->base_dir, "/");
			strcat(s->base_dir, base_dir);
		}
		else {
			memcpy(s->base_dir, base_dir, strlen(base_dir));
		}
#endif

		s->nb_channel = 0;
		s->max_channel = rx_nb_channel;
		s->src_addr = src_addr;
		s->obj_list = NULL;
		s->fdt_list = NULL;
		s->wanted_obj_list = NULL;
		s->rx_fdt_instance_list = NULL;
		s->accept_expired_fdt_inst = accept_expired_fdt_inst;

		/* Create receiving thread */

#ifdef WIN32
		if (CreateThread(NULL, 0, (void*)rx_thread, (void*)s, 0, &s->rx_thread_id) == NULL) {
			perror("open_alc_session: CreateThread");
			return -1;
		}
#else
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 

		if (pthread_create(&s->rx_thread_id, &attr, rx_thread, (void*)s) != 0) {
			perror("open_alc_session: pthread_create");
			return -1;
		}
#endif

	}

	for(i = 0; i < MAX_ALC_SESSIONS; i++) {
		if(alc_session_list[i] == NULL) {
			s->s_id = i;
			alc_session_list[s->s_id] = s;
			break;
		}
	}

	nb_alc_session++;

	return s->s_id;
}

/*
 * This function closes session.
 *
 * Params:	int s_id: Session identifier
 *
 * Return:	void
 *
 */

void close_alc_session(int s_id) {

	int i, ret_val;
	wanted_obj_t *next_want;
	wanted_obj_t *want;
	rx_fdt_instance_t *next_instance;
	rx_fdt_instance_t *instance;

	tx_queue_t *next_pkt;
	tx_queue_t *pkt;
	
	trans_obj_t *to;
	
	alc_session_t *s = alc_session_list[s_id];
	
	s->state = SClosed;

#ifdef WIN32
	Sleep(1000);
#else
	sleep(1);
#endif

	for(i = 0; i < s->max_channel; i++) {
		
		if(s->ch_list[i] != NULL) {
			ret_val = close_alc_channel(s->ch_list[i], s);
		}
	}

	/* Closing, free all uncompleted objects, uncompleted fdt instances and wanted obj list */
			
	to = s->obj_list;

	while(to != NULL) {
		free_object(to, s, 1);
		to = s->obj_list;
	}

	to = s->fdt_list;
                                                                                                                                              
    while(to != NULL) {
        free_object(to, s, 0);
        to = s->fdt_list;
    }

	want = s->wanted_obj_list;

	while(want != NULL) {
		next_want = want->next;
		free(want);
		want = next_want;
	} 	

	instance = s->rx_fdt_instance_list;
                                                                                                                                          
    while(instance != NULL) {
        next_instance = instance->next;
        free(instance);
        instance = next_instance;
    }

#ifdef USE_RLC
	if(s->cc_id == RLC) {
		close_mad_rlc(s);
	}
#endif

	pkt = s->tx_queue_begin;

	while(pkt != NULL) {
		next_pkt = pkt->next;
		free(pkt->data);
		free(pkt);
		pkt = next_pkt;
	}

	free(s);
	alc_session_list[s_id] = NULL;
	nb_alc_session--;
}

/*
 * This function returns session from session_list.
 *
 * Params: int s_id: Session identifier
 *
 * Return: alc_session_t*: Pointer to session, NULL if session does not exist.
 *
 */

alc_session_t* get_alc_session(int s_id) {

	return alc_session_list[s_id];	
}

/*
 * This function returns state of session.
 *
 * Params:	int s_id: Session identifier
 *
 * Return:	int: State of the session, -1 if session does not exist anymore.
 *
 */

int get_session_state(int s_id) {
	
	if(alc_session_list[s_id] == NULL) {
		return -1;
	}

	return alc_session_list[s_id]->state;
}

/*
 * This function sets state of session.
 *
 * Params:	int s_id: Session identifier,
 *			enum alc_session_states state: New session state
 *
 * Return:	void
 *
 */

void set_session_state(int s_id, enum alc_session_states state) {
	
	alc_session_list[s_id]->state = state;
}

/*
 * This function returns state of A flag in session.
 *
 * Params:	int s_id: Session identifier
 *
 * Return:	int: State of A flag (0 = not set, 1 = set)
 *
 */

int get_session_a_flag(int s_id) {
	
	return alc_session_list[s_id]->a_flag;
}

/*
 * This function sets state of A flag in session.
 *
 * Params:	int s_id: Session identifier
 *
 * Return:	void
 *
 */

void set_session_a_flag(int s_id) {
	
	alc_session_list[s_id]->a_flag = 1;
}

/*
 * This function returns session's FDT Instance ID.
 *
 * Params:	int s_id: Session identifier
 *
 * Return:	unsigned int: Session's FDT Instance ID
 *
 */

unsigned int get_fdt_instance_id(int s_id) {

	return alc_session_list[s_id]->fdt_instance_id;
}

/* 
 * This function sets session's FDT Instance ID.
 *
 * Params:	int s_id: Session identifier,
 *			unsigned int: FDT Instance ID to be set
 *
 * Return:	void
 *
 */

void set_fdt_instance_id(int s_id, unsigned int instance_id) {

	alc_session_list[s_id]->fdt_instance_id =  (instance_id & 0x00FFFFFF);
}

/*
 * This function returns number of bytes sent in session since latest zeroing.
 *
 * Params:	int s_id: Session identifier
 *
 * Return:	ULONGLONG/unsigned long long: Number of bytes sent in session since latest zeroing
 *
 */
   
#ifdef WIN32
ULONGLONG get_session_sent_bytes(int s_id) {
#else
unsigned long long get_session_sent_bytes(int s_id) {
#endif
                                                                                                                     
        return alc_session_list[s_id]->sent_bytes;
}
                                                                                                                                       
/*
 * This function sets number of bytes sent in session.
 *
 * Params:	int s_id: Session identifier,
 *			unsigned int sent_bytes: Bytes sent
 *
 * Return:	void
 *
 */
                                                                                                                                       
void set_session_sent_bytes(int s_id, unsigned int sent_bytes) {
                                                                                                                                       
        alc_session_list[s_id]->sent_bytes = sent_bytes;
}

/*
 * This function adds sent bytes in session.
 *
 * Params:	int s_id: Session identifier,
 *			unsigned int sent_bytes: Bytes sent
 *
 * Return:	void
 *
 */
                                                                                                                                       
void add_session_sent_bytes(int s_id, unsigned int sent_bytes) {
                                                                                                                                       
        alc_session_list[s_id]->sent_bytes += sent_bytes;
}


/*
 * This function adds channel to session.
 *
 * Params:	int s_id: Session identifier,
 *			char *port: Pointer to port string,
 *			char *addr: Pointer to address string,
 *			char *intface: Pointer to interface string,
 *			char *intface_name: Pointer to the interface name.
 *
 * Return:	int: Identifier for created channel in success, -1 otherwise
 *
 */

int add_alc_channel(int s_id, char *port, char *addr, char *intface, char *intface_name) {
	
	alc_channel_t *ch = NULL;

	alc_session_t *s = alc_session_list[s_id];
	
	if(s->nb_channel >= s->max_channel) {
		/* Could not add new alc channel to alc session */
		/* printf("Could not create new alc channel: Max number of channels already used!\n");*/
		return -1;
	}

	return open_alc_channel(ch, s, port, addr, intface, intface_name, s->def_tx_rate);
}

/*
 * This function removes channel from session.
 *
 * Params:	int s_id: Session Identifier,
 *			int ch_id: Channel Identifier
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int remove_alc_channel(int s_id, int ch_id) {
	alc_session_t *s = alc_session_list[s_id];
	alc_channel_t *ch = s->ch_list[ch_id];
	
	return close_alc_channel(ch, s);
}

/*
 * This function set object wanted from session.
 *
 * Params:	int s_id: Session Identifier,
 *			ULONGLONG/unsigned long long  toi: Transport Object Identifier,
 *			ULONGLONG/unsigned long long toi_len: Object length,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Full-Size Source Block Length,
 *			int fec_inst_id: FEC Instance ID,
 *			short fec_enc_id: FEC Encoding ID,
 *			unsigned short max_nb_of_enc_symb:,
 *			unsigned char content_enc_algo:.
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

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
		,unsigned char content_enc_algo) {
#else 
	) {
#endif
	
	alc_session_t *s = alc_session_list[s_id];
	wanted_obj_t *wanted_obj;
	wanted_obj_t *tmp;

	tmp = s->wanted_obj_list;

	if(tmp == NULL) {

		if (!(wanted_obj = (wanted_obj_t*)calloc(1, sizeof(wanted_obj_t)))) {
			printf("Could not alloc memory for wanted object!\n");
			return -1;
		}

		wanted_obj->toi = toi;
		wanted_obj->toi_len = toi_len;
		wanted_obj->eslen = eslen;
		wanted_obj->max_sblen = max_sblen;
		wanted_obj->fec_inst_id = fec_inst_id;
		wanted_obj->fec_enc_id = fec_enc_id;
		wanted_obj->max_nb_of_enc_symb = max_nb_of_enc_symb;

#ifdef USE_ZLIB
		wanted_obj->content_enc_algo = content_enc_algo;
#endif
		
		wanted_obj->prev = NULL;
		wanted_obj->next = NULL;

		s->wanted_obj_list = wanted_obj;
	}
	else {
		for(;; tmp = tmp->next) {
			if(tmp->toi == toi) {
				break;
			}
			else if(tmp->next == NULL) {

				if (!(wanted_obj = (wanted_obj_t*)calloc(1, sizeof(wanted_obj_t)))) {
					printf("Could not alloc memory for wanted object!\n");
					return -1;
				}

				wanted_obj->toi = toi;
				wanted_obj->toi_len = toi_len;
				wanted_obj->eslen = eslen;
				wanted_obj->max_sblen = max_sblen;
				wanted_obj->fec_inst_id = fec_inst_id;
				wanted_obj->fec_enc_id = fec_enc_id;
				wanted_obj->max_nb_of_enc_symb = max_nb_of_enc_symb;

#ifdef USE_ZLIB
				wanted_obj->content_enc_algo = content_enc_algo;
#endif

				tmp->next = wanted_obj;
				wanted_obj->prev = tmp;
				wanted_obj->next = NULL;
				break;
			}
		}
	}

	return 0;
}

/*
 * This function checks if object identified with TOI is wanted and returns pointer to wanted object structure.
 *
 * Params:	alc_session_t *s: Pointer to Session,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier to be checked
 *
 * Return:	wanted_obj_t*: Pointer to wanted object structure in success, NULL otherwise
 *
 */

wanted_obj_t* get_wanted_object(alc_session_t *s,
								
#ifdef WIN32
								ULONGLONG toi) {
#else
								unsigned long long toi) {
#endif

	wanted_obj_t *tmp = s->wanted_obj_list;
		
	while(tmp != NULL) {
		if(tmp->toi == toi) {
			return tmp;
		}
		tmp = tmp->next;
	}

	return NULL;
}

/*
 * This function removes wanted object identified with toi from the session.
 *
 * Params:      int s_id: Session Identifier,
 *              ULONGLONG/unsigned long long toi: Transport Object Identifier
 *
 * Return:      void
 *
 */

void remove_wanted_object(int s_id,
#ifdef WIN32
						  ULONGLONG toi) {
#else
						  unsigned long long toi) {
#endif

	alc_session_t *s; 	
	wanted_obj_t *next_want;
	wanted_obj_t *want;
	
	s = get_alc_session(s_id);
	next_want = s->wanted_obj_list;

	while(next_want != NULL) {
		
		want = next_want;
		  
	    	if(want->toi == toi) {
			
	      		if(want->next != NULL) {
					want->next->prev = want->prev;
	      		}
	      		if(want->prev != NULL) {
					want->prev->next = want->next;
	      		}
	      		if(want == s->wanted_obj_list) {
					s->wanted_obj_list = want->next;
	      		}
	      
	      		free(want);
	      		break;
	    	}
	    	next_want = want->next;
	}
}

/*
 * This function sets FDT Instance to received list
 *
 * Params:	alc_session_t *s: Pointer to Session,
 *			unsigned int fdt_instance_id: FDT Instance ID to be set,
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int set_received_instance(alc_session_t *s, unsigned int fdt_instance_id) {
 
        rx_fdt_instance_t *rx_fdt_instance;
        rx_fdt_instance_t *list;
                                                                                                                                              
        list = s->rx_fdt_instance_list;
                                                                                                                                              
        if(list == NULL) {
                                                                                                                                              
                if (!(rx_fdt_instance = (rx_fdt_instance_t*)calloc(1, sizeof(rx_fdt_instance_t)))) {
                        printf("Could not alloc memory for rx_fdt_instance!\n");
                        return -1;
                }

                rx_fdt_instance->fdt_instance_id = fdt_instance_id;
                rx_fdt_instance->prev = NULL;
                rx_fdt_instance->next = NULL;
        
		s->rx_fdt_instance_list = rx_fdt_instance;
        }
        else {
                for(;; list = list->next) {
                        if(list->fdt_instance_id == fdt_instance_id) {
                                break;
                        }
                        else if(list->next == NULL) {
              
							if (!(rx_fdt_instance = (rx_fdt_instance_t*)calloc(1, sizeof(rx_fdt_instance_t)))) {
                					printf("Could not alloc memory for rx_fdt_instance!\n");
                        			return -1;
                			}
		 
							rx_fdt_instance->fdt_instance_id = fdt_instance_id;
                                                                                                                                              
							list->next = rx_fdt_instance;
							rx_fdt_instance->prev = list;
							rx_fdt_instance->next = NULL;
							break;
                        }
                }
        }
                                                                                                                                              
        return 0;
}

/*
 * This function checks if FDT Instance is already received
 *
 * Params:	alc_session_t *s: Pointer to Session,
 *			unsigned int fdt_instance_id: FDT Instance ID to be checked
 *
 * Return:	bool: true if received, false otherwise
 *
 */
                                                                                                                                              
bool is_received_instance(alc_session_t *s, unsigned int fdt_instance_id) {
                                                                                                                                              
        bool retval = false;
        rx_fdt_instance_t *list = s->rx_fdt_instance_list;
                                                                                                                                              
        while(list != NULL) {
                if(list->fdt_instance_id == fdt_instance_id) {
                        retval = true;
                        break;
                }
                list = list->next;
        }
                                                                                                                                              
        return retval;
}


/*
 * This function returns session's base directory.
 *
 * Params:	int s_id: Session identifier,
 *
 * Return:	char*: Pointer to base directory.
 *
 */

char* get_session_basedir(int s_id) {

	alc_session_t *s; 	
	
	s = get_alc_session(s_id);

	return s->base_dir;
}


