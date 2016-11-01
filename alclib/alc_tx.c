/* $Author: peltotas $ $Date: 2004/12/22 12:17:36 $ $Revision: 1.48 $ */
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

/**** Private functions ****/

int calculate_packet_length(alc_session_t *s);

int send_unit(trans_unit_t *tr_unit, trans_block_t *tr_block, alc_channel_t *ch,
#ifdef WIN32
			   ULONGLONG toi,
			   ULONGLONG toilen,
#else
			   unsigned long long toi,
			   unsigned long long toilen,
#endif
               unsigned int max_sblen,
               unsigned short eslen,
			   unsigned char fec_enc_id,
			   unsigned short fec_inst_id,
			   int a_flag);

int send_unit2(trans_unit_t *tr_unit, trans_block_t *tr_block, alc_session_t *s,
#ifdef WIN32
			   ULONGLONG toi,
			   ULONGLONG toilen,
#else
			   unsigned long long toi,
			   unsigned long long toilen,
#endif
               unsigned int max_sblen,
               unsigned short eslen,
			   unsigned char fec_enc_id,
			   unsigned short fec_inst_id);

int randomloss(double lossprob);

int add_pkt_to_tx_queue(alc_session_t *s, unsigned char *sendbuf,  unsigned int sendlen);

/*
 * This function sends source block from object to channel.
 *
 * Params:	int s_id: Session identifier,
 *			int ch_id: Channel identifier,
 *			char *buf: Pointer to data to be sent,
 *			ULONGLONG/unsigned long long buf_len: Lenght of data to be sent,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			ULONGLONG/unsigned long long toilen: Length of transport object,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned int sbn: Souirce Block Number,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			int is_last_block: is this last block for object (1 = yes, 0 = no).
 *
 * Return:	int: 0 after succesfull sending, -1 otherwise
 *
 */

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
			 unsigned char fec_enc_id, unsigned short fec_inst_id, int is_last_block) {

	trans_block_t *tr_block;
	trans_unit_t *tr_unit;
	unsigned int i, j;
	
	alc_channel_t *ch;

#ifdef WIN32
	ULONGLONG sent = 0;
	ULONGLONG tb_data_left;
#else
	unsigned long sent = 0;
	unsigned long tb_data_left;
#endif

	int packet_length;
	double interval;
	double packetpersec;
	double currenttime;
	double lasttime;
	int addr_family;
	int use_fec_oti_ext_hdr;
	double loss_prob;
	int retval;
	bool a_flag_packet_sent = false;

	ch = get_alc_session(s_id)->ch_list[ch_id];

	addr_family = ch->s->addr_family;
	use_fec_oti_ext_hdr = ch->s->use_fec_oti_ext_hdr;

	if(toi == FDT_TOI) {

		/* eslen + DEF_LCT_HDR + TSI + TOI + EXT_FDT + EXT_CENC + EXT_FTI + FEC_PL_ID + UDP + IP */ 

		if(addr_family == PF_INET) {

			if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {
				packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 4 + 16 + 4 + 8 + 20); 
			}
			else if(((fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 4 + 16 + 8 + 8 + 20); 
			}
		}
		else if(addr_family == PF_INET6) {

			if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {
				packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 4 + 16 + 4 + 8 + 40); 
			}
			else if(((fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 4 + 16 + 8 + 8 + 40); 
			}
		}
	}
	else {
		if(addr_family == PF_INET) {

			if(use_fec_oti_ext_hdr == 1) {
				/* eslen + DEF_LCT_HDR + TSI + TOI + EXT_FTI + FEC_PL_ID + UDP + IP */ 
				
				if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 4 + 8 + 20); 
				}
				else if(((fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
					 (fec_enc_id == SB_SYS_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 8 + 8 + 20); 
				}
			}
			else if(use_fec_oti_ext_hdr == 0) {

				/* eslen + DEF_LCT_HDR + TSI + TOI + FEC_PL_ID + UDP + IP */ 
				
				if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 8 + 20); 
				}
				else if(((fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
					 (fec_enc_id == SB_SYS_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 8 + 8 + 20); 
				}
			}
		}
		else if(addr_family == PF_INET6) {

			if(use_fec_oti_ext_hdr == 1) {
				/* eslen + DEF_LCT_HDR + TSI + TOI + EXT_FTI + FEC_PL_ID + UDP + IP */ 
				
				if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 4 + 8 + 40); 
				}
				else if(((fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
					 (fec_enc_id == SB_SYS_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 8 + 8 + 40); 
				}
			}
			else if(use_fec_oti_ext_hdr == 0) {

				/* eslen + DEF_LCT_HDR + TSI + TOI + FEC_PL_ID + UDP + IP */ 
				
				if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 8 + 40); 
				}
				else if(((fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
					 (fec_enc_id == SB_SYS_FEC_ENC_ID))) {
					packet_length =  eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 8 + 8 + 40); 
				}
			}
		}
	}
	
 	packetpersec = ((double)(ch->tx_rate * 1000) / (double)(packet_length * 8));   
	interval = ((double)1 / packetpersec);

	if(fec_enc_id == COM_NO_C_FEC_ENC_ID) {

#ifdef USE_NULL_FEC
		tr_block = null_fec_encode_src_block(buf, buf_len, sbn, eslen);

/* Possible your own NULL-FEC library
#elif YOUR_OWN_NULL_FEC
		tr_block = your_own_null_encode_src_block(buf, buf_len, sbn, eslen);
			
*/
#endif
	}
#ifdef USE_SIMPLE_XOR
	else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
		tr_block = xor_fec_encode_src_block(buf, buf_len, sbn, eslen);
        }
#endif

#ifdef USE_REED_SOLOMON
	else if(((fec_enc_id == SB_SYS_FEC_ENC_ID) && (fec_inst_id == REED_SOL_FEC_INST_ID))) {
		tr_block = rs_fec_encode_src_block(buf, buf_len, sbn, eslen, ch->s->def_fec_ratio,
										   max_sblen);
	}
#endif
	else {
		printf("FEC Encoding %i/%i is not supported!\n", fec_enc_id, fec_inst_id);
		return -1; 
	}

	if(tr_block == NULL) {
		return -1;
	}

	lasttime = sec();

	tb_data_left = eslen * tr_block->n;
	tr_unit = tr_block->unit_list;
		
	for(i = 0; i < tr_block->n; i++) {

		while(1) {

			if(ch->s->state == SExiting) {

				while(1) {

					free(tr_unit->data);

					if(tr_unit->esid == (tr_block->n - 1)) {
						break;
					}
 
					tr_unit++;
				}
			
				free(tr_block->unit_list);
				free(tr_block);

				return -2;
			}

			currenttime = sec();

			if(currenttime >= (lasttime + interval)) {

				if((((get_alc_session(s_id)->a_flag) || (get_alc_session(s_id)->state == SExiting))
				   && ((is_last_block) && (((i + 1) == tr_block->n))))) {
					
					for(j = 0; j < 3; j++) { /* We send last encoding symbol 3 times with A-flag up */
							
						loss_prob = 0;

						if(ch->s->simul_losses) {
                                                	if(ch->previous_lost == true) {
                                                        	loss_prob = ch->s->loss_ratio2; /*P_LOSS_WHEN_LOSS;*/
                                                    	}
                                                     	else {
                                                        	loss_prob = ch->s->loss_ratio1; /*P_LOSS_WHEN_OK;*/
                                                	}
                                             	}

                                          	if(!randomloss(loss_prob)) {

                                                	retval = send_unit(tr_unit, tr_block, ch, toi, toilen, max_sblen, eslen,
                                                                           fec_enc_id, fec_inst_id, 1);

                                                      	if(retval < 0) {

                                                    		while(1) {

                                                               		free(tr_unit->data);

                                                                	if(tr_unit->esid == (tr_block->n - 1)) {
                                                                        	break;
                                                                  	}

                                                                       	tr_unit++;
                                                         	}

                                                         	free(tr_block->unit_list);
                                                             	free(tr_block);

                                                        	return -1;
                                                 	}
	
							if(!a_flag_packet_sent) {
							
								sent += tr_unit->len;
                                                        	/*add_session_sent_bytes(s_id, min(tr_unit->len, tb_data_left));*/
                                                        	/*tb_data_left -= min(tr_unit->len, tb_data_left);*/
                                                        	add_session_sent_bytes(s_id, tb_data_left < tr_unit->len ?
                                                                                                (unsigned int)tb_data_left : tr_unit->len);
                                                        	tb_data_left -= tb_data_left < tr_unit->len ? tb_data_left : tr_unit->len;

                                                        	if(ch->s->verbosity) {
#ifdef WIN32
                                                                printf("%.2f%% of object sent, TOI: %I64u (SBN: %i ESI: %i)\n",
                                                                        	(double)((double)(100 * ((double)(LONGLONG)get_session_sent_bytes( s_id )/
                                                                        	(double)(LONGLONG)toilen))), toi, tr_block->sbn, tr_unit->esid);
																
																/*printf("%i%% of object sent, TOI: %I64u (SBN: %i ESI: %i)\n",
                                                                        	(int)((double)(100 * ((double)(LONGLONG)get_session_sent_bytes( s_id )/
                                                                        	(double)(LONGLONG)toilen))), toi, tr_block->sbn, tr_unit->esid);*/
#else
                                                               	 	printf("%.2f%% of object sent, TOI: %llu (SBN: \t%i\t ESI: \t%i\t)\n",
                                                                        	(double)((double)(100 * ((double)(long long)get_session_sent_bytes( s_id )/
                                                                       	 	(double)(long long)toilen))), toi, tr_block->sbn, tr_unit->esid);

																			/*printf("%i%% of object sent, TOI: %llu (SBN: %i ESI: %i)\n",
                                                                        	(int)((double)(100 * ((double)(long long)get_session_sent_bytes( s_id )/
                                                                       	 	(double)(long long)toilen))), toi, tr_block->sbn, tr_unit->esid);*/
#endif
                                                                	fflush(stdout);
                                                        	}
													
								a_flag_packet_sent = true;
							}

                                                	ch->previous_lost = false;
                                              	}
                                               	else {
                                                    	ch->previous_lost = true;
                                        	}
					}
				}
				else {
					/* packets from esid 0 to sblen - 1 */
					loss_prob = 0;

					if(ch->s->simul_losses) {
						if(ch->previous_lost == true) {
							loss_prob = ch->s->loss_ratio2;/*P_LOSS_WHEN_LOSS;*/
						}
						else {
							loss_prob = ch->s->loss_ratio1;/*P_LOSS_WHEN_OK;*/
						}
					}

					if(!randomloss(loss_prob)) {

						retval = send_unit(tr_unit, tr_block, ch, toi, toilen, max_sblen, eslen,
											fec_enc_id, fec_inst_id, 0);

						if(retval < 0) {
							
							while(1) {

								free(tr_unit->data);

								if(tr_unit->esid == (tr_block->n - 1)) {
									break;
								}

								tr_unit++;
							}
						
							free(tr_block->unit_list);
							free(tr_block);

							return -1;
						}

						sent += tr_unit->len;
						/*add_session_sent_bytes(s_id, min(tr_unit->len, tb_data_left));
 						tb_data_left -= min(tr_unit->len, tb_data_left);*/
						add_session_sent_bytes(s_id, tb_data_left < tr_unit->len ?
													(unsigned int)tb_data_left : tr_unit->len);
 						tb_data_left -= tb_data_left < tr_unit->len ? tb_data_left : tr_unit->len;
						
						if(ch->s->verbosity) {
#ifdef WIN32
							printf("%.2f%% of object sent, TOI: %I64u (SBN: %i ESI: %i)\n",
									(double)((double)(100 * ((double)(LONGLONG)get_session_sent_bytes( s_id )/
									(double)(LONGLONG)toilen))), toi, tr_block->sbn, tr_unit->esid);
							
									/*printf("%i%% of object sent, TOI: %I64u (SBN: %i ESI: %i)\n",
									(int)((double)(100 * ((double)(LONGLONG)get_session_sent_bytes( s_id )/
									(double)(LONGLONG)toilen))), toi, tr_block->sbn, tr_unit->esid);*/
#else
							printf("%.2f%% of object sent, TOI: %llu (SBN: \t%i\t ESI: \t%i\t)\n",
                                     (double)((double)(100 * ((double)(long long)get_session_sent_bytes( s_id )/
                                     (double)(long long)toilen))), toi, tr_block->sbn, tr_unit->esid);
									/*printf("%i%% of object sent, TOI: %llu (SBN: %i ESI: %i)\n",
                                    (int)((double)(100 * ((double)(long long)get_session_sent_bytes( s_id )/
                                    (double)(long long)toilen))), toi, tr_block->sbn, tr_unit->esid);*/
#endif							
							fflush(stdout);
						}
						
						ch->previous_lost = false;
					}
					else {
						ch->previous_lost = true;
					}
				}
				
				lasttime += interval;/*= currenttime*/;
				break;
			}
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif /* WIN32 */
		}
		
		free(tr_unit->data);
		tr_unit++;
	}

	free(tr_block->unit_list);
	free(tr_block);
	
	return 0;	
}

/*
 * This function sends source block from object to channel.
 *
 * Params:	int s_id: Session identifier,
 *			char *buf: Pointer to data to be sent,
 *			ULONGLONG/unsigned long long buf_len: Lenght of data to be sent,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			ULONGLONG/unsigned long long toilen: Length of transport object,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned int sbn: Source Block Number,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID.
 *
 * Return:	int: 0 after succesfull sending, -1 or -2 in error/stopping situations.
 *
 */

int alc_send2(int s_id, char *buf,
#ifdef WIN32
			 ULONGLONG	buf_len,
			 ULONGLONG toi,
			 ULONGLONG toilen,
#else
			 unsigned long long buf_len,
			 unsigned long long toi,
			 unsigned long long toilen,
#endif
			 unsigned short eslen, unsigned int max_sblen, unsigned int sbn,
			 unsigned char fec_enc_id, unsigned short fec_inst_id) {

	trans_block_t *tr_block;
	trans_unit_t *tr_unit;
	unsigned int i;
	int retcode;	
	alc_session_t *s;
	
	s = get_alc_session(s_id);
	
	if(fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
		tr_block = null_fec_encode_src_block(buf, buf_len, sbn, eslen);
		
/* Possible your own NULL-FEC library
#elif YOUR_OWN_NULL_FEC
		tr_block = your_own_null_encode_src_block(buf, buf_len, sbn, eslen);
			
*/
#endif
	}

#ifdef USE_SIMPLE_XOR
        else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                tr_block = xor_fec_encode_src_block(buf, buf_len, sbn, eslen);
        }
#endif

#ifdef USE_REED_SOLOMON
	else if(((fec_enc_id == SB_SYS_FEC_ENC_ID) && (fec_inst_id == REED_SOL_FEC_INST_ID))) {
		tr_block = rs_fec_encode_src_block(buf, buf_len, sbn, eslen, s->def_fec_ratio,
										   max_sblen);
	}
#endif
	else {
		printf("FEC Encoding %i/%i is not supported!\n", fec_enc_id, fec_inst_id);
		return -1; 
	}

	if(tr_block == NULL) {
		return -1;
	}

	tr_unit = tr_block->unit_list;
		
	for(i = 0; i < tr_block->n; i++) {
		
		retcode = send_unit2(tr_unit, tr_block, s, toi, toilen,
							 max_sblen, eslen, fec_enc_id, fec_inst_id);
	
		if(retcode == -1) {
			i--;
			continue;
		}
		else if(retcode == -2) {

			while(1) {

				free(tr_unit->data);

				if(tr_unit->esid == (tr_block->n - 1)) {
					break;
				}
 
				tr_unit++;
			}
			
			free(tr_block->unit_list);
			free(tr_block);

			return retcode;		
		}

		free(tr_unit->data);
		tr_unit++;
	}

	free(tr_block->unit_list);
	free(tr_block);
	
	return 0;	
}

/*
 * This function sends one unit to channel.
 *
 * Params:	trans_unit_t *tr_unit: Pointer to transport unit to be sent,
 *			trans_block_t *tr_block: Pointer to transport block that this units belongs,
 *			alc_channel_t *ch: Pointer to channel,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			ULONGLONG/unsigned long long toilen: Length of transport object,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			int a_flag: Set A flag (1 = yes, 0 = no).
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int send_unit(trans_unit_t *tr_unit, trans_block_t *tr_block, alc_channel_t *ch,
#ifdef WIN32
			   ULONGLONG toi,
			   ULONGLONG toilen,
#else
			   unsigned long long toi,
			   unsigned long long toilen,
#endif
			   unsigned int max_sblen, unsigned short eslen,
			   unsigned char fec_enc_id, unsigned short fec_inst_id,

			   int a_flag) {

	/*int i;*/
	int	sendlen;								/* number of bytes to send */
	int	hdrlen;									/* length of total ALC header */

	unsigned char sendbuf[MAX_PACKET_LENGTH];	/* TU (hdrs+data) cannot be larger */

	def_lct_hdr_t *def_lct_hdr;

	unsigned short half_word1;
	unsigned short half_word2;
	unsigned int word;

	unsigned short max_16bits = 0xFFFF;
	unsigned int max_32bits = 0xFFFFFFFF;

#ifdef WIN32
	ULONGLONG max_48bits = (ULONGLONG)0xFFFFFFFFFFFF;
	ULONGLONG max_64bits = (ULONGLONG)0xFFFFFFFFFFFFFFFF;
#else
	unsigned long long max_48bits = (unsigned long long)0xFFFFFFFFFFFF;
	unsigned long long  max_64bits = (unsigned long long)0xFFFFFFFFFFFFFFFF;
#endif

	hdrlen = sizeof(def_lct_hdr_t);

	memset(sendbuf, 0, MAX_PACKET_LENGTH);
	def_lct_hdr = (def_lct_hdr_t*)sendbuf;

	def_lct_hdr->version = ALC_VERSION;
	def_lct_hdr->flag_c = 0;
	def_lct_hdr->reserved = 0;
	def_lct_hdr->flag_t = 0;
	def_lct_hdr->flag_r = 0;

	def_lct_hdr->flag_a = a_flag;
	def_lct_hdr->flag_b = 0; /* TODO */

	def_lct_hdr->codepoint = fec_enc_id;

	def_lct_hdr->cci = htonl(0);
	
	if(ch->s->half_word) {
		if(((ch->s->tsi <= max_16bits) && (toi <= max_16bits))) {
			def_lct_hdr->flag_s = 0;
			def_lct_hdr->flag_o = 0;
			def_lct_hdr->flag_h = 1;

			half_word1 = (unsigned short)ch->s->tsi;
			half_word2 = (unsigned short)toi;
			
			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		}
		else if(((((ch->s->tsi <= max_16bits) && (((toi > max_16bits) && (toi <= max_32bits))))) ||
			 (((toi <= max_16bits) && (((ch->s->tsi > max_16bits) && (ch->s->tsi <= max_32bits))))))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)ch->s->tsi);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;	
		}
		else if((((ch->s->tsi > max_16bits) && (ch->s->tsi <= max_32bits))) &&
			 (((toi > max_16bits) && (toi <= max_32bits)))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)ch->s->tsi);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((ch->s->tsi <= max_16bits) && (((toi > max_32bits) && (toi <= max_48bits))))) {		
			def_lct_hdr->flag_s = 0;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 1;

			half_word1 = (unsigned short)ch->s->tsi;
			half_word2 = (unsigned short)((toi >> 32) & 0x0000FFFF);
			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
			
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((toi < max_16bits) && (((ch->s->tsi > max_32bits) && (ch->s->tsi <= max_48bits))))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 0;
			def_lct_hdr->flag_h = 1;

			word = (unsigned int)((ch->s->tsi >> 16) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		
			half_word1 = (unsigned short)(ch->s->tsi & 0x0000FFFF);
			half_word2 = (unsigned short)toi;

			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		}
		else if((((ch->s->tsi > max_32bits) && (ch->s->tsi <= max_48bits))) &&
			 (((toi > max_32bits) && (toi <= max_48bits)))) {		
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 1;

			word = (unsigned int)((ch->s->tsi >> 16) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		
			half_word1 = (unsigned short)(ch->s->tsi & 0x0000FFFF);
			half_word2 = (unsigned short)((toi >> 32) & 0x0000FFFF);
			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
			
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if((((ch->s->tsi > max_16bits) && (ch->s->tsi <= max_32bits))) &&
			 (((toi > max_32bits) && (toi <= max_64bits)))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 2;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)ch->s->tsi);
			hdrlen += 4;

			word = (unsigned int)((toi >> 32) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((ch->s->tsi < max_16bits) && ((toi > max_48bits) && (toi <= max_64bits)))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 2;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)ch->s->tsi);
			hdrlen += 4;

			word = (unsigned int)((toi >> 32) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else {
			printf("TSI & TOI field combination not supported!\n");
			fflush(stdout);
			return -1;
		}
	}
	else {
		def_lct_hdr->flag_s = 1;
		def_lct_hdr->flag_h = 0;
	
		*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)ch->s->tsi);
		hdrlen += 4;

		if(toi <= max_32bits) {
			def_lct_hdr->flag_o = 1;
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((toi > max_32bits) && (toi <= max_64bits))) {
			def_lct_hdr->flag_o = 2;
		
			word = (unsigned int)((toi >> 32) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else {
			printf("TSI & TOI field combination not supported!\n");
			fflush(stdout);
			return -1;
		}
	}

	if(toi == FDT_TOI) {
		hdrlen += add_fdt_lct_he(def_lct_hdr, hdrlen, ch->s->fdt_instance_id);

#ifdef USE_ZLIB
		if(ch->s->encode_content) {
			 hdrlen += add_cenc_lct_he(def_lct_hdr, hdrlen, (unsigned char)ZLIB);
		}	
#endif
		
	}
	
	if(((toi == FDT_TOI) || (ch->s->use_fec_oti_ext_hdr == 1))) {

		if(fec_enc_id == COM_NO_C_FEC_ENC_ID) {

			hdrlen += add_fti_0_128_130_lct_he(def_lct_hdr, hdrlen, toilen, 0,
											   eslen, max_sblen);	
		}
		else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {

                        hdrlen += add_fti_0_128_130_lct_he(def_lct_hdr, hdrlen, toilen, fec_inst_id,
                                                                                           eslen, max_sblen);
                }
		else if(((fec_enc_id == COM_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID))) {

			hdrlen += add_fti_0_128_130_lct_he(def_lct_hdr, hdrlen, toilen, fec_inst_id,
											   eslen, max_sblen);	
		}
		else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {

			hdrlen += add_fti_129_lct_he(def_lct_hdr, hdrlen, toilen, fec_inst_id,
							eslen, (unsigned short)tr_block->max_k, (unsigned short)tr_block->max_n);
		}
	}

	/* TODO: add other LCT header extensions here */
	/*if(nop) {
		hdrlen += add_nop_lct_he();
	}
	if(auth) {
		hdrlen += add_auth_lct_he();
	}*/

	def_lct_hdr->hdr_len = hdrlen >> 2; /* Header length in 32-bit words */	
	*(unsigned short*)def_lct_hdr = htons(*(unsigned short*)def_lct_hdr);

	/* FEC Payload ID */

	if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {

		hdrlen += add_alc_fpi_0_130(def_lct_hdr, hdrlen, (unsigned short)tr_block->sbn,
						(unsigned short)tr_unit->esid);	
	}
	else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {

                hdrlen += add_alc_fpi_128(def_lct_hdr, hdrlen, tr_block->sbn, tr_unit->esid);
        }
	else if(fec_enc_id == SB_LB_E_FEC_ENC_ID) {

		hdrlen += add_alc_fpi_128(def_lct_hdr, hdrlen, tr_block->sbn, tr_unit->esid);
	}
	else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {

		hdrlen += add_alc_fpi_129(def_lct_hdr, hdrlen, tr_block->sbn, (unsigned short)tr_block->k,
								  (unsigned short)tr_unit->esid);
	}

	memcpy(sendbuf + hdrlen, tr_unit->data, tr_unit->len);

	sendlen = hdrlen + tr_unit->len;

	/*for(i = 0; i < sendlen; i++) {
		printf("0x%x.", sendbuf[i]);
	}
	printf("\n");*/

	sendto(ch->tx_sock, (char *)sendbuf, sendlen, 0,
		   ch->addrinfo->ai_addr, ch->addrinfo->ai_addrlen);

	return 0;
}

/*
 * This function sends one unit to tx_queue.
 *
 * Params:	trans_unit_t *tr_unit: Pointer to transport unit to be sent,
 *			trans_block_t *tr_block: Pointer to transport block that this units belongs,
 *			alc_session_t *s: Pointer to session,
 *			unsigned int toi: Transport Object Identifier,
 *			ULONGLONG/unsigned long long toilen: Length of transport object,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID.
 *
 * Return:	int: 0 in success, -1 or -2  in error/stopping cases.
 *
 */

int send_unit2(trans_unit_t *tr_unit, trans_block_t *tr_block, alc_session_t *s,
#ifdef WIN32
			   ULONGLONG toi,
			   ULONGLONG toilen,
#else
			   unsigned long long toi,
			   unsigned long long toilen,
#endif
		unsigned int max_sblen, unsigned short eslen,
		unsigned char fec_enc_id, unsigned short fec_inst_id) {

	int	hdrlen;		/* length of total ALC header */

	unsigned char pkt[MAX_PACKET_LENGTH];
	int	sendlen = 0;
	unsigned char *sendbuf;

	def_lct_hdr_t *def_lct_hdr;

	unsigned short half_word1;
	unsigned short half_word2;
	unsigned int word;
        
	unsigned short max_16bits = 0xFFFF;
	unsigned int max_32bits = 0xFFFFFFFF;

#ifdef WIN32
	ULONGLONG max_48bits = (ULONGLONG)0xFFFFFFFFFFFF;
	ULONGLONG max_64bits = (ULONGLONG)0xFFFFFFFFFFFFFFFF;
#else
	unsigned long long max_48bits = (unsigned long long)0xFFFFFFFFFFFF;
	unsigned long long  max_64bits = (unsigned long long)0xFFFFFFFFFFFFFFFF;
#endif
	hdrlen = sizeof(def_lct_hdr_t);
		
	memset(pkt, 0, MAX_PACKET_LENGTH);
	def_lct_hdr = (def_lct_hdr_t*)pkt;

	def_lct_hdr->version = ALC_VERSION;
	def_lct_hdr->flag_c = 0;
	def_lct_hdr->reserved = 0;
	def_lct_hdr->flag_t = 0;
	def_lct_hdr->flag_r = 0;

	def_lct_hdr->flag_a = 0;

	def_lct_hdr->flag_b = 0; /* TODO */

	def_lct_hdr->codepoint = fec_enc_id;

	def_lct_hdr->cci = htonl(0);

	if(s->half_word) {
		if(((s->tsi <= max_16bits) && (toi <= max_16bits))) {
			def_lct_hdr->flag_s = 0;
			def_lct_hdr->flag_o = 0;
			def_lct_hdr->flag_h = 1;

			half_word1 = (unsigned short)s->tsi;
			half_word2 = (unsigned short)toi;
			
			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		}
		else if(((((s->tsi <= max_16bits) && (((toi > max_16bits) && (toi <= max_32bits))))) ||
			 (((toi <= max_16bits) && (((s->tsi > max_16bits) && (s->tsi <= max_32bits))))))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)s->tsi);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;	
		}
		else if((((s->tsi > max_16bits) && (s->tsi <= max_32bits))) &&
			 (((toi > max_16bits) && (toi <= max_32bits)))) {			
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)s->tsi);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((s->tsi <= max_16bits) && (((toi > max_32bits) && (toi <= max_48bits))))) {		
			def_lct_hdr->flag_s = 0;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 1;

			half_word1 = (unsigned short)s->tsi;
			half_word2 = (unsigned short)((toi >> 32) & 0x0000FFFF);
			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
			
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((toi < max_16bits) && (((s->tsi > max_32bits) && (s->tsi <= max_48bits))))) {

			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 0;
			def_lct_hdr->flag_h = 1;

			word = (unsigned int)((s->tsi >> 16) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		
			half_word1 = (unsigned short)(s->tsi & 0x0000FFFF);
			half_word2 = (unsigned short)toi;

			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		}
		else if((((s->tsi > max_32bits) && (s->tsi <= max_48bits))) &&
			 (((toi > max_32bits) && (toi <= max_48bits)))) {		
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 1;
			def_lct_hdr->flag_h = 1;

			word = (unsigned int)((s->tsi >> 16) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		
			half_word1 = (unsigned short)(s->tsi & 0x0000FFFF);
			half_word2 = (unsigned short)((toi >> 32) & 0x0000FFFF);
			word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
			
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if((((s->tsi > max_16bits) && (s->tsi <= max_32bits))) &&
			 (((toi > max_32bits) && (toi <= max_64bits)))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 2;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)s->tsi);
			hdrlen += 4;

			word = (unsigned int)((toi >> 32) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((s->tsi < max_16bits) && ((toi > max_48bits) && (toi <= max_64bits)))) {
			def_lct_hdr->flag_s = 1;
			def_lct_hdr->flag_o = 2;
			def_lct_hdr->flag_h = 0;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)s->tsi);
			hdrlen += 4;

			word = (unsigned int)((toi >> 32) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;

			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else {
			printf("TSI & TOI field combination not supported!\n");
			fflush(stdout);
			return -1;
		}
	}
	else {
		def_lct_hdr->flag_s = 1;
		def_lct_hdr->flag_h = 0;
	
		*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)s->tsi);
		hdrlen += 4;

		if(toi <= max_32bits) {
			def_lct_hdr->flag_o = 1;
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else if(((toi > max_32bits) && (toi <= max_64bits))) {
			def_lct_hdr->flag_o = 2;
		
			word = (unsigned int)((toi >> 32) & 0xFFFFFFFF);
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
			hdrlen += 4;
		
			*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)toi);
			hdrlen += 4;
		}
		else {
			printf("TSI & TOI field combination not supported!\n");
			fflush(stdout);
			return -1;
		}
	}

	if(toi == FDT_TOI) {
		hdrlen += add_fdt_lct_he(def_lct_hdr, hdrlen, s->fdt_instance_id);

#ifdef USE_ZLIB
		if(s->encode_content) {
			 hdrlen += add_cenc_lct_he(def_lct_hdr, hdrlen, (unsigned char)ZLIB);
		}	
#endif

	}
	
	if(((toi == FDT_TOI) || (s->use_fec_oti_ext_hdr == 1))) {

		if(fec_enc_id == COM_NO_C_FEC_ENC_ID) {

			hdrlen += add_fti_0_128_130_lct_he(def_lct_hdr, hdrlen, toilen, 0,
											   eslen, max_sblen);
		}
		else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {

                        hdrlen += add_fti_0_128_130_lct_he(def_lct_hdr, hdrlen, toilen, fec_inst_id,
                                                                                           eslen, max_sblen);
                }
		else if((fec_enc_id == COM_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID)) {

			hdrlen += add_fti_0_128_130_lct_he(def_lct_hdr, hdrlen, toilen, fec_inst_id,
											   eslen, max_sblen);	
		}

		else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {

			hdrlen += add_fti_129_lct_he(def_lct_hdr, hdrlen, toilen, fec_inst_id,
							eslen, (unsigned short)tr_block->max_k, (unsigned short)tr_block->max_n);
		}
	}

	/* TODO: add othet LCT header extensions here */
	/*if(nop) {
		hdrlen += add_nop_lct_he();
	}
	if(auth) {
		hdrlen += add_auth_lct_he();
	}*/

	def_lct_hdr->hdr_len = hdrlen >> 2; /* Header length in 32-bit words */	
	*(unsigned short*)def_lct_hdr = htons(*(unsigned short*)def_lct_hdr);

	/* FEC Payload ID */

	if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == COM_FEC_ENC_ID))) {

		hdrlen += add_alc_fpi_0_130(def_lct_hdr, hdrlen, (unsigned short)tr_block->sbn,
						(unsigned short)tr_unit->esid);	
	}
	else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {

                hdrlen += add_alc_fpi_128(def_lct_hdr, hdrlen, tr_block->sbn, tr_unit->esid);
        }
	else if(fec_enc_id == SB_LB_E_FEC_ENC_ID) {

		hdrlen += add_alc_fpi_128(def_lct_hdr, hdrlen, tr_block->sbn, tr_unit->esid);
	}
	else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {

		hdrlen += add_alc_fpi_129(def_lct_hdr, hdrlen, tr_block->sbn,
								  (unsigned short)tr_block->k,
								  (unsigned short)tr_unit->esid);
	}

	memcpy(pkt + hdrlen, tr_unit->data, tr_unit->len);

	sendlen = hdrlen + tr_unit->len;

	if(!(sendbuf = (unsigned char*)calloc((sendlen + 1), sizeof(unsigned char)))) {
		printf("Could not alloc memory for tx queue packet!\n");
		fflush(stdout);
		return -1;
	}

	memcpy(sendbuf, pkt, sendlen);

	while(1) {

		if(s->state == SExiting) {
			return -2;
		}

		if(add_pkt_to_tx_queue(s, sendbuf, sendlen) > 0) {
			break;
		}
		else {
#ifdef WIN32
			Sleep(10);
#else
			usleep(10000);
#endif
		}
	}

	return sendlen;
}


/*
 * This function sends A flag packet to channel.
 *
 * Params:	int s_id: Session identifier,
 *			int ch_id: Channel identifier
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int send_session_close_packet(int s_id, int ch_id) {

	int	sendlen;			     /* number of bytes to send */
	int	hdrlen;					/* length of total ALC header */
	int retval = 0;

	alc_channel_t *ch;

	unsigned char sendbuf[MAX_PACKET_LENGTH];	/* TU (hdrs+data) cannot be larger */

	def_lct_hdr_t *def_lct_hdr;
	
	unsigned short half_word1;
	unsigned short half_word2;
	unsigned int word;

	ch = get_alc_session(s_id)->ch_list[ch_id];
	
	memset(sendbuf, 0, MAX_PACKET_LENGTH);
	def_lct_hdr = (def_lct_hdr_t*)sendbuf;

	def_lct_hdr->version = ALC_VERSION;
	def_lct_hdr->flag_c = 0;
	def_lct_hdr->reserved = 0;

	def_lct_hdr->flag_t = 0;
	def_lct_hdr->flag_r = 0;
	def_lct_hdr->flag_a = 1;
	def_lct_hdr->flag_b = 0;
	def_lct_hdr->codepoint = ch->s->def_fec_enc_id; 

	def_lct_hdr->cci = htonl(0);

	hdrlen = sizeof(def_lct_hdr_t);
	
	if(ch->s->tsi <= 0xFFFFFFFF) {
		
		def_lct_hdr->flag_s = 1;
		def_lct_hdr->flag_o = 0;
		def_lct_hdr->flag_h = 0;
		
		*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl((unsigned int)ch->s->tsi);
		hdrlen += 4;
	}
	else {
		/* TSI field length 48 bits not possible without TOI field */

		def_lct_hdr->flag_s = 1;
		def_lct_hdr->flag_o = 0;
		def_lct_hdr->flag_h = 1;

		word = (unsigned int)((ch->s->tsi >> 16) & 0xFFFFFFFF);
		*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
		hdrlen += 4;
	
		half_word1 = (unsigned short)(ch->s->tsi & 0x0000FFFF);
		half_word2 = (unsigned short)0;

		word = ((half_word1 << 16) | (half_word2 & 0xFFFF));
		*(unsigned int*)((unsigned char*)def_lct_hdr + hdrlen) = htonl(word);
		hdrlen += 4;
	}

	def_lct_hdr->hdr_len = hdrlen >> 2; /* Header length in 32-bit words */	
	*(unsigned short*)def_lct_hdr = htons(*(unsigned short*)def_lct_hdr);

	sendlen = hdrlen;

#ifdef USE_RLC
	if(ch->s->cc_id == RLC) {
		retval = mad_rlc_fill_header(ch->s, (rlc_hdr_t*)(sendbuf + 4), ch->ch_id);
	}
#endif

	retval = sendto(ch->tx_sock, (char *)sendbuf, sendlen, 0,
					ch->addrinfo->ai_addr, ch->addrinfo->ai_addrlen);

	return retval;
}

/*
 * This function simulates packets losses randomly.
 *
 * Params: double lossprob: Loss probability percent.
 *
 * Returns: 1 if packet should be dropped, 0 otherwize
 *
 */

int randomloss(double lossprob) {
	
	int loss = 0;

	double msb;
	double lsb;

	double tmp;
	
	msb = (double)(rand()%100);
	lsb = (double)(rand()%10);
	
	tmp = msb + (double)(lsb/(double)10);

	if(tmp < lossprob) {
		loss = 1;
	}

	/*tmp = (float)(rand()%100);

	if(tmp < (float)lossprob) {
		loss = 1;
	}*/
	
	return loss;
}

/*  
 * This function.
 * 
 * Params:	void *s: Pointer to session.	
 *
 * Return:	void
 *
 */

void* tx_thread(void *s) {

	alc_session_t *session;
	alc_channel_t *channel;
	tx_queue_t *tmp_ptr;

	tx_queue_t *next_pkt;
	tx_queue_t *pkt;

	int i, j;
	int retcode;
	int packet_length;

	double interval;
	double packetpersec;
	double currenttime;
	double lasttime;

	double loss_prob;

	session = (alc_session_t *)s;

	packet_length = calculate_packet_length(session);

	packetpersec = ((double)(session->def_tx_rate * 1000) / (double)(packet_length * 8));   
	interval = ((double)1 / packetpersec);

	/*	interval is too small for FDT Instance, because in packet length calculation FDT Instance's
		extra header fields are not counted */

	while(session->state == SActive) {	
	
		if(session->tx_queue_begin == NULL) {
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif		
			continue;
		}
		else {
			break;
		}
	}

	lasttime = sec();

	while(session->state == SActive) {

		currenttime = sec();

		if(currenttime >= (lasttime + interval)) {
			
			for(i = 0; i < session->nb_channel; i++) {

				channel = session->ch_list[i];
#ifdef USE_RLC
				if(session->cc_id == RLC) {

					if(channel->start_sending == false) {
						continue;
					}

					if(channel->wait_after_sp > 0) {
						channel->wait_after_sp--;
						continue;
					}
				}
#endif
				for(j = 0; j < channel->nb_tx_units; j++) {

					if(channel->queue_ptr == NULL) {

						if(channel->ready == false) {
							session->nb_ready_channel++;
							channel->ready = true;
						}

						if(session->nb_ready_channel == session->nb_sending_channel) {

							pkt = session->tx_queue_begin;

							while(pkt != NULL) {
								next_pkt = pkt->next;
								free(pkt->data);
								free(pkt);
								pkt = next_pkt;
							}	

							session->tx_queue_begin = NULL;
							session->tx_queue_size = 0;
						}

						break;
					}
			
					if(session->first_unit_in_loop) {

#ifdef USE_RLC
						if(session->cc_id == RLC) {
							mad_rlc_reset_tx_sp(session);
						}
#endif

						session->first_unit_in_loop = false;
					}
#ifdef USE_RLC
					if(session->cc_id == RLC) {

						retcode = mad_rlc_fill_header(session, (rlc_hdr_t*)(channel->queue_ptr->data + 4),
												  channel->ch_id);
					
						if(retcode < 0) {
						}
					}
#endif
					loss_prob = 0;

					if(session->simul_losses) {
                                                if(channel->previous_lost == true) {
                                                        loss_prob = channel->s->loss_ratio2;/*P_LOSS_WHEN_LOSS;*/
                                                }
                                                else {
                                                        loss_prob = channel->s->loss_ratio1;/*P_LOSS_WHEN_OK;*/
                                                }
                                        }

					if(!randomloss(loss_prob)) {

						retcode = sendto(channel->tx_sock, (char*)channel->queue_ptr->data,
										 channel->queue_ptr->datalen, 0, channel->addrinfo->ai_addr,
										 channel->addrinfo->ai_addrlen);

						if(retcode < 0) {

#ifdef WIN32
							printf("sendto failed with: %d\n", WSAGetLastError());
#else
							printf("sendto failed with: %d\n", h_errno);
#endif
							break;
						}

						if(session->verbosity) {
							printf("sent %i bytes to: %s:%s\n", retcode, channel->addr,
									channel->port);
							fflush(stdout);
						}

						channel->previous_lost = false;
					}
					else {
						channel->previous_lost = true;
					}
					
					channel->queue_ptr->nb_tx_ch++;

					if(channel->queue_ptr->nb_tx_ch == (unsigned int)session->nb_channel) {

						tmp_ptr = channel->queue_ptr->next;
						free(channel->queue_ptr->data);
						free(channel->queue_ptr);

						channel->queue_ptr = tmp_ptr;
						session->tx_queue_begin = tmp_ptr;
						session->tx_queue_size--;
					}
					else {
						channel->queue_ptr = channel->queue_ptr->next;
					}
				}
			}

			lasttime += interval;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}

#ifdef WIN32
	ExitThread(0);
#else
	pthread_exit(0);
#endif
}

/*  
 * This function calculates packet length used in session.
 * 
 * Params:	alc_session_t *s: Pointer to session.		
 *
 * Return:	int: Packet length for this session.
 *
 */

int calculate_packet_length(alc_session_t *s) {

	int packet_length;


	if(s->addr_family == PF_INET) {

		if(s->use_fec_oti_ext_hdr == 1) {

			/* eslen + DEF_LCT_HDR + TSI + TOI + EXT_FTI + FEC_PL_ID + UDP + IP */

			if(((s->def_fec_enc_id == COM_NO_C_FEC_ENC_ID) ||
				(s->def_fec_enc_id == COM_FEC_ENC_ID))) {	 	
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 4 + 8 + 20); 
			}
			else if(((s->def_fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 8 + 8 + 20); 
			}
		}
		else if(s->use_fec_oti_ext_hdr == 0) {
			/* eslen + DEF_LCT_HDR + TSI + TOI + FEC_PL_ID + UDP + IP */ 

			if(((s->def_fec_enc_id == COM_NO_C_FEC_ENC_ID) ||
				(s->def_fec_enc_id == COM_FEC_ENC_ID))) {	
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 8 + 20); 
			}
			else if(((s->def_fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 8 + 8 + 20); 
			}
		}
	}
	else if(s->addr_family == PF_INET6) {
		if(s->use_fec_oti_ext_hdr == 1) {
			/* eslen + DEF_LCT_HDR + TSI + TOI + EXT_FTI + FEC_PL_ID + UDP + IP */

			if(((s->def_fec_enc_id == COM_NO_C_FEC_ENC_ID) ||
				(s->def_fec_enc_id == COM_FEC_ENC_ID))) {	 	
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 4 + 8 + 40); 
			}
			else if(((s->def_fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 16 + 8 + 8 + 40); 
			}
		}
		else if(s->use_fec_oti_ext_hdr == 0) {
			/* eslen + DEF_LCT_HDR + TSI + TOI + FEC_PL_ID + UDP + IP */

			if(((s->def_fec_enc_id == COM_NO_C_FEC_ENC_ID) ||
				(s->def_fec_enc_id == COM_FEC_ENC_ID))) {	 	
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 4 + 8 + 20); 
			}
			else if(((s->def_fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (s->def_fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				packet_length =  s->def_eslen + (sizeof(def_lct_hdr_t) + 4 + 4 + 8 + 8 + 20); 
			}
		}
	}

	return packet_length;	
}

/*  
 * This function adds packet to tx queue.
 * 
 * Params:	alc_session_t *s: Pointer to session,
 *			unsigned char* sendbuf: Pointer to data to be added to queue,
 *			unsigned int sendlen: Lenght of data.
 *
 * Return:	int: > 0 in success, 0 or -1 in error cases.
 *
 */

int add_pkt_to_tx_queue(alc_session_t *s, unsigned char *sendbuf,  unsigned int sendlen) {
	int retval = 0;
	tx_queue_t *pkt;
	int i;
	alc_channel_t *ch;

	if(s->tx_queue_size == MAX_TX_QUEUE_SIZE) {
		return retval;
	}

	/* Allocate memory for pkt */
	if(!(pkt = (tx_queue_t*)calloc(1, sizeof(tx_queue_t)))) {
		printf("Could not alloc memory for tx_queue pkt!\n");
		return -1;
	}

	pkt->nb_tx_ch = 0;
	pkt->next = NULL;
	pkt->datalen = sendlen;
	pkt->data = sendbuf;

	if(s->tx_queue_begin == NULL) {

		s->tx_queue_begin = pkt;
		s->tx_queue_end = pkt;

		for(i = 0; i < s->nb_channel; i++) {
			ch = s->ch_list[i];
			ch->ready = false;
			ch->queue_ptr = s->tx_queue_begin;

#ifdef USE_RLC
			if(s->cc_id == RLC) {
				if(ch->ch_id != 0) {
					ch->wait_after_sp = RLC_WAIT_AFTER_SP;	
					ch->start_sending = false;
				}
			}
#endif
		}

		s->nb_ready_channel = 0;

#ifdef USE_RLC
		if(s->cc_id == RLC) {
			s->nb_sending_channel = 1;
		}
#endif
	}
	else {
		s->tx_queue_end->next = pkt;
		s->tx_queue_end = pkt;

		for(i = 0; i < s->nb_channel; i++) {
			ch = s->ch_list[i];
			
			if(ch->queue_ptr == NULL) {
				ch->queue_ptr = s->tx_queue_end;
			}
		}
	}

	s->tx_queue_size++;

	return sendlen;
}
