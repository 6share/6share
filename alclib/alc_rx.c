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

trans_obj_t* object_exist(
#ifdef WIN32
						  ULONGLONG toi,
#else
						  unsigned long long toi,
#endif
						  alc_session_t *s, int type);

trans_block_t* block_exist(unsigned int sbn, trans_obj_t *trans_obj);
int analyze_packet(char *data, int len, alc_channel_t *ch);
bool object_completed(trans_obj_t *to);
bool object_completed2(trans_obj_t *to);
bool blocks_completed(trans_obj_t *to);
bool block_ready_to_decode(trans_block_t *tb);
int recv_packet(alc_session_t *s);

/*  
 * This function receives packets from all channels in one session in thread.
 * 
 * Params:	void *s: Pointer to session	
 *
 * Return:	void
 *
 */

void* rx_thread(void *s) {
	
	alc_session_t *session;
	int retval;

	session = (alc_session_t *)s;

	while(session->state == SActive) {
		
		if(session->nb_channel != 0) {
			retval = recv_packet(session);
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif

	}

	/*printf("Rx thread exit\n");
	fflush(stdout);*/

#ifdef WIN32
	ExitThread(0);
#else
	pthread_exit(0);
#endif
}


/* 
 * This function receives unit(s) from session's channels.
 * 
 * Params: alc_session_t *s: Pointer to session	
 *
 * Return: int: Number of correct packets received from ALC session
 * 
 */

int recv_packet(alc_session_t *s) {
	
	char recvbuf[MAX_PACKET_LENGTH];
	int recvlen;
	int nb;
	int i;
	int retval;
	int max_fd = 0;
	int recv_pkts = 0;
	fd_set rfds;
	alc_channel_t *ch;
	struct timeval tv;
	struct sockaddr_storage from;
	char hostname[100];

#ifdef WIN32
	int fromlen;
#else
	socklen_t fromlen;
#endif

	if(s->addr_family == PF_INET) {
		fromlen = sizeof(struct sockaddr_in);
	}
	else if(s->addr_family == PF_INET6) {
		fromlen = sizeof(struct sockaddr_in6);
	}

	memset(recvbuf, 0, MAX_PACKET_LENGTH);

	FD_ZERO(&rfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	for(i = 0; i < s->max_channel; i++) {
		ch = s->ch_list[i];

		if(ch != NULL) {
			FD_SET(ch->rx_sock, &rfds);
			max_fd = max((int)ch->rx_sock, max_fd);
		}
	}

	nb = select((max_fd + 1), &rfds, NULL, NULL, &tv);

	if(nb <= 0) {
		return 0;
	}

	for(i = 0; i < s->max_channel; i++) {
		ch = s->ch_list[i];

		if(ch != NULL) {

			memset((void *)&from, 0, sizeof(from));

			if(FD_ISSET(ch->rx_sock, &rfds)) {

				recvlen = recvfrom(ch->rx_sock, recvbuf, MAX_PACKET_LENGTH, 0,
						   (struct sockaddr*)&from, &fromlen);

				if(recvlen < 0) {

					if(s->state == SExiting) {
						/*printf("recv_packet() SExiting\n");
						fflush(stdout);*/
						return -2;
					}
					else if(s->state == SClosed) {
						/*printf("recv_packet() SClosed\n");
						fflush(stdout);*/
						return 0;
					}
					else {
	#ifdef WIN32
						printf("recvfrom failed: %d\n", WSAGetLastError());
						fflush(stdout);
	#else
						printf("recvfrom failed: %d\n", h_errno);
	#endif
						return -1;
					}
				}
				
				getnameinfo((struct sockaddr*)&from, fromlen, hostname, sizeof(hostname), NULL, 0, NI_NUMERICHOST);

#ifdef FIXME_CHECK_SOURCE
				if(strcmp(hostname, s->src_addr) != 0) {
					printf("Packet to wrong session: wrong source: %s\n", hostname);
					fflush(stdout);
					continue;
				}
#endif

				retval = analyze_packet(recvbuf, recvlen, ch);

#ifdef USE_RLC
				if(ch->s->cc_id == RLC) {

					if(((ch->s->rlc->drop_highest_layer) && (ch->s->nb_channel != 1))) {

						ch->s->rlc->drop_highest_layer = false;
						close_alc_channel(ch->s->ch_list[ch->s->nb_channel - 1], ch->s);
					}
				}
#endif

				if(retval == HDR_ERROR) {
					continue;
				}
				else if(retval == DUP_PACKET) {
					continue;
				}
				else if(retval == MEM_ERROR) {
					return -1;
				}
				
				recv_pkts++;
			}
		}
	}

	return recv_pkts;
}

/*
 * This function parses and analyzes alc packet.
 *
 * Params:	char *data: Pointer to packet's data,
 *			int len: Length of packet's data,
 *			alc_channel_t *ch: Pointer to channel
 *
 * Return:	int: Status of packet (OK, EMPTY_PACKET, HDR_ERROR, MEM_ERROR, DUP_PACKET)
 *
 */

int analyze_packet(char *data, int len, alc_channel_t *ch) {

	def_lct_hdr_t *def_lct_hdr;		
	
	int hdrlen = 0;			/* length of whole header */
	int retval;
	
#ifdef WIN32
	ULONGLONG tsi;
	ULONGLONG toi;
	ULONGLONG toilen;
#else
	unsigned long long tsi;
	unsigned long long toi;
	unsigned long long toilen;
#endif

	unsigned int sbn;
	unsigned int esid;

	unsigned short eslen;
	unsigned short sblen;
	
	unsigned int max_sblen;
	unsigned short max_nb_of_es;

	short fec_enc_id;
	int fec_inst_id;

	int fdt_instance_id;
	unsigned short flute_version;
	unsigned char cont_enc_algo = 0;
	unsigned short reserved;

	unsigned int word;

#ifdef WIN32
	ULONGLONG ull;
#else
	unsigned long long ull;
#endif

	trans_obj_t	*trans_obj;
	trans_block_t *trans_block;
	trans_unit_t *trans_unit;
	wanted_obj_t *wanted_obj;

	int	het;
	int	hel;
	int	exthdrlen;

	char *buf = NULL;

#ifdef WIN32
	ULONGLONG block_len;
	ULONGLONG pos;
#else
	unsigned long long block_len;
	unsigned long long pos;
#endif
	
	char filename[MAX_PATH];

	if(len < (int)(sizeof(def_lct_hdr_t))) {
		printf("analyze_packet: packet too short %d\n", len);
		fflush(stdout);
		return HDR_ERROR;
	}
	
	def_lct_hdr = (def_lct_hdr_t*)data;

	*(unsigned short*)def_lct_hdr = ntohs(*(unsigned short*)def_lct_hdr);

	hdrlen += (int)(sizeof(def_lct_hdr_t));

	if(def_lct_hdr->version != ALC_VERSION) {
	  printf("ALC version: %i not supported!\n", def_lct_hdr->version);
	  fflush(stdout);	
	  return HDR_ERROR;
	}

	if(def_lct_hdr->reserved != 0) {
		printf("Reserved field not zero!\n");
		fflush(stdout);
		return HDR_ERROR;
	}

	if(def_lct_hdr->flag_t != 0) {
		printf("Sender Current Time not supported!\n");
		fflush(stdout);
		return HDR_ERROR;
	}

	if(def_lct_hdr->flag_r != 0) {
		printf("Expected Residual Time not supported!\n");
		fflush(stdout);
		return HDR_ERROR;
	}

	if(def_lct_hdr->flag_b == 1) {
		/**** TODO ****/
	}

	if(def_lct_hdr->flag_c != 0) {
		printf("Only 32 bits CCI-field supported!\n");
		fflush(stdout);
		return HDR_ERROR;
	}
	else {
		if(def_lct_hdr->cci != 0) {

#ifdef USE_RLC
			if(ch->s->cc_id == RLC) {

				retval = mad_rlc_analyze_cci(ch->s, (rlc_hdr_t*)(data + 4));
				
				if(retval < 0) {
					return HDR_ERROR;
				}
			}
#endif
		}
	}

	if(def_lct_hdr->flag_h == 1) {

		if(def_lct_hdr->flag_s == 0) { /* TSI 16 bits */
			word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;

			tsi = (word & 0xFFFF0000) >> 16;
	
			if(tsi != ch->s->tsi) {
				printf("Packet to wrong session: wrong TSI: %i\n", tsi);
				fflush(stdout);
				return HDR_ERROR;
			}
		}
		else if(def_lct_hdr->flag_s == 1) { /* TSI 48 bits */

			ull = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			tsi = ull << 16;
			hdrlen += 4;

			word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;

			tsi += (word & 0xFFFF0000) >> 16;
		
			if(tsi != ch->s->tsi) {
				printf("Packet to wrong session: wrong TSI: %i\n", tsi);
				fflush(stdout);
				return HDR_ERROR;
			}
		}

#ifdef FIXME_ZAP_SESSION_CLOSE
                if(def_lct_hdr->flag_a == 1) {
                        ch->s->state = STxStopped;
                }
#endif

		if(def_lct_hdr->flag_o == 0) { /* TOI 16 bits */
			toi = (word & 0x0000FFFF);
		}
		else if(def_lct_hdr->flag_o == 1) { /* TOI 48 bits */

			ull = (word & 0x0000FFFF);
			toi = ull << 32;

			toi += ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;
		}
		else {
			printf("Only 16, 32, 48 or 64 bits TOI-field supported!\n");
			fflush(stdout);
			return HDR_ERROR;
		}
		/*else if(def_lct_hdr->flag_o == 2) {			
		}
		else if(def_lct_hdr->flag_o == 3) {
		}*/
	}
	else {
		if(def_lct_hdr->flag_s == 1) { /* TSI 32 bits */
			tsi = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;
		
			if(tsi != ch->s->tsi) {
				printf("Packet to wrong session: wrong TSI: %i\n", tsi);
				fflush(stdout);
				return HDR_ERROR;
			}
		}
		else {
			printf("Transport Session Identifier not present!\n");
			fflush(stdout);
			return HDR_ERROR;
		}

#ifdef FIXME_ZAP_SESSION_CLOSE
		if(def_lct_hdr->flag_a == 1) {
			ch->s->state = STxStopped;
		}
#endif

		if(def_lct_hdr->flag_o == 0) { /* TOI 0 bits */

			if(def_lct_hdr->flag_a != 1) {
				printf("Transport Object Identifier not present!\n");
				fflush(stdout);
				return HDR_ERROR;
			}
			else {
				return EMPTY_PACKET;
			}
		}
		else if(def_lct_hdr->flag_o == 1) { /* TOI 32 bits */
			toi = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;
		}
		else if(def_lct_hdr->flag_o == 2) { /* TOI 64 bits */

			ull = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			toi = ull << 32;
			hdrlen += 4;

			toi += ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;
		}
		else {
			printf("Only 16, 32, 48 or 64 bits TOI-field supported!\n");
			fflush(stdout);
			return HDR_ERROR;
		}
                /*else if(def_lct_hdr->flag_o == 3) {
                }*/
	}

	if(!toi == FDT_TOI) {

		wanted_obj = get_wanted_object(ch->s, toi);
		
		if(wanted_obj == NULL) {
			/*printf("Packet to not wanted toi: %i\n", toi);
			fflush(stdout);*/
			return HDR_ERROR;
		}

		eslen = wanted_obj->eslen;
		max_sblen = wanted_obj->max_sblen;
		max_nb_of_es = wanted_obj->max_nb_of_enc_symb;
		fec_enc_id = wanted_obj->fec_enc_id;
		fec_inst_id = wanted_obj->fec_inst_id;
		toilen = wanted_obj->toi_len;

#ifdef USE_ZLIB
		cont_enc_algo = wanted_obj->content_enc_algo;
#endif

	}

	fec_enc_id = def_lct_hdr->codepoint;

	if(fec_enc_id == COM_NO_C_FEC_ENC_ID) {
	}

#ifdef USE_REED_SOLOMON
	else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {
	}
#endif

#ifdef USE_SIMPLE_XOR
	else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
	}
#endif
	else {
		printf("FEC Encoding ID: %i is not supported!\n", fec_enc_id);
                fflush(stdout);
                return HDR_ERROR;
	}

	if(def_lct_hdr->hdr_len > (hdrlen >> 2)) {
		
		/* LCT header extensions(EXT_FDT, EXT_CENC, EXT_FTI, EXT_AUTH, EXT_NOP)
		   go through all possible EH */
		
		exthdrlen = def_lct_hdr->hdr_len - (hdrlen >> 2);
	
		while(exthdrlen > 0) {
			
			word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
			hdrlen += 4;
			exthdrlen--;
		
			het = (word & 0xFF000000) >> 24;

			if(het < 128) {
				hel = (word & 0x00FF0000) >> 16;
			}

			switch(het) {

				case EXT_FDT:

					flute_version = (word & 0x00F00000) >> 20;
					fdt_instance_id = (word & 0x000FFFFF);

					if(flute_version != FLUTE_VERSION) {
						printf("FLUTE version: %i is not supported\n", flute_version);
						return HDR_ERROR;
					}

					break;

				case EXT_CENC:

					cont_enc_algo = (word & 0x00FF0000) >> 16;
					reserved = (word & 0x0000FFFF);

					if(reserved != 0) {
						printf("Bad CENC header extension!\n");
						return HDR_ERROR;
					}

#ifdef USE_ZLIB
					if(cont_enc_algo != ZLIB) {
						printf("Only ZLIB supported with FDT Instance!\n");
						return HDR_ERROR;
					}
#endif

					break;

				case EXT_FTI:
				
					if(hel != 4) {
						printf("Bad FTI header extension, length: %i\n", hel);
						return HDR_ERROR;
					}
	
					toilen = ((word & 0x0000FFFF) << 16);
						
					toilen += ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
					hdrlen += 4;
					exthdrlen--;

					word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
					hdrlen += 4;
					exthdrlen--;

					fec_inst_id = ((word & 0xFFFF0000) >> 16);

#ifdef USE_REED_SOLOMON
					if(fec_enc_id == SB_SYS_FEC_ENC_ID) {
						if(fec_inst_id != REED_SOL_FEC_INST_ID) {
							printf("FEC Encoding %i/%i is not supported!\n", fec_enc_id, fec_inst_id);
							return HDR_ERROR;
						}
					}
#endif
					if(((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id == SB_LB_E_FEC_ENC_ID)
						|| (fec_enc_id == COM_FEC_ENC_ID) || (fec_enc_id == SIMPLE_XOR_FEC_ENC_ID))) {
						
						eslen = (word & 0x0000FFFF);
						
						max_sblen = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
						hdrlen += 4;
						exthdrlen--;
					}
					else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {

						eslen = (word & 0x0000FFFF);

						word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));

						max_sblen = ((word & 0xFFFF0000) >> 16);
						max_nb_of_es = (word & 0x0000FFFF);

						hdrlen += 4;
						exthdrlen--;
						
					}
				break;

				case EXT_AUTH:
					/* ignore */
					hdrlen += (hel-1) << 2;
					exthdrlen -= (hel-1);
				break;

				case EXT_NOP:
					/* ignore */
					hdrlen += (hel-1) << 2;
					exthdrlen -= (hel-1);
				break;

				default:

					printf("Unknown LCT Extension header, het: %i\n", het);
					return HDR_ERROR;
				break;
			}
		}
	}

	if((hdrlen >> 2) != def_lct_hdr->hdr_len) {
		/* Wrong header length */
		printf("analyze_packet: packet header length %d, should be %d\n", (hdrlen >> 2),
				def_lct_hdr->hdr_len);
		return HDR_ERROR;
	}

	/* Check if we have an empty packet without FEC Payload ID */
	if(hdrlen == len) {
		return EMPTY_PACKET;		
	}

#ifdef NO_HUP_PACKETS
	if(((toi == 0) && (is_received_instance(ch->s, fdt_instance_id)))) {
		return DUP_PACKET;
	}
#endif

	if((fec_enc_id == COM_NO_C_FEC_ENC_ID) || (fec_enc_id ==  COM_FEC_ENC_ID)) {
		
		if(len < hdrlen + 4) {
			printf("analyze_packet: packet too short %d\n", len);
			return HDR_ERROR;
		}

		word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
		sbn = (word >> 16);
		esid = (word & 0xFFFF);
		hdrlen += 4;
	}
	else if(((fec_enc_id == SB_LB_E_FEC_ENC_ID) || (fec_enc_id == SIMPLE_XOR_FEC_ENC_ID))) {
		if (len < hdrlen + 8) {
			printf("analyze_packet: packet too short %d\n", len);
			return HDR_ERROR;
		}

		sbn = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
		hdrlen += 4;
		esid = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
		hdrlen += 4;
		
	}
	else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {
		if (len < hdrlen + 8) {
			printf("analyze_packet: packet too short %d\n", len);
			return HDR_ERROR;
		}

		sbn = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));

		hdrlen += 4;
		word = ntohl(*(unsigned int*)((char*)def_lct_hdr + hdrlen));
		sblen = (word >> 16);
		esid = (word & 0xFFFF);
		hdrlen += 4;
	}

	/* TODO: check if instance_id is set --> EXT_FDT header exists in packet */

	if(len - hdrlen != 0) {

		/* check if we have enough information */

		if(((toilen == 0) || (fec_enc_id == -1) || ((fec_enc_id > 127) && (fec_inst_id == -1)) ||
			(eslen == 0) || (max_sblen == 0))) {
#ifdef WIN32
			printf("Not enough information to create Transport Object, TOI: %I64u\n", toi);
#else
			printf("Not enough information to create Transport Object, TOI: %llu\n", toi);
#endif
			fflush(stdout);
			return HDR_ERROR;
		}

		/* Create transport unit */
		trans_unit = create_units(1);

		if(trans_unit == NULL) {
			return MEM_ERROR;
		}

		trans_unit->esid = esid;
		trans_unit->len = len - hdrlen; /* Data length */

		/* Alloc memory for incoming TU data */
		if(!(trans_unit->data = (char*)calloc(eslen, sizeof(char)))) { /* rs_fec&xor_fec: trans_unit->len --> eslen */
			printf("Could not alloc memory for transport unit's data!\n");
			return MEM_ERROR;
		}

		memcpy(trans_unit->data, (data + hdrlen), trans_unit->len);

		/* Check if object already exist */
		if(toi == FDT_TOI) {
			trans_obj = object_exist(fdt_instance_id, ch->s, 0);
		}
		else {
			trans_obj = object_exist(toi, ch->s, 1);
		}
		
		if(trans_obj == NULL) {

			trans_obj = create_object();

			if(trans_obj == NULL) {
				return MEM_ERROR;
			}
			
			if(toi == FDT_TOI) {
				trans_obj->toi = fdt_instance_id;
#ifdef USE_ZLIB
				trans_obj->cont_enc_algo = cont_enc_algo;
#endif
			}
			else {
				trans_obj->toi = toi;
				
				if(ch->s->big_file_mode) {
			
					memset(filename, 0, MAX_PATH);

#ifdef USE_ZLIB
					if(cont_enc_algo == 0) {
#ifdef WIN32
						sprintf(filename, "%s\\object%I64u", ch->s->base_dir, toi);
#else
						sprintf(filename, "%s/object%llu", ch->s->base_dir, toi);
#endif
					}
					else if(cont_enc_algo == GZIP) {
#ifdef WIN32
						sprintf(filename, "%s\\object%I64u%s", ch->s->base_dir, toi, GZ_SUFFIX);
#else
						sprintf(filename, "%s/object%llu%s", ch->s->base_dir, toi, GZ_SUFFIX);
#endif
					}
					else {
#ifdef WIN32
						sprintf(filename, "%s\\object%I64u", ch->s->base_dir, toi);
#else
						sprintf(filename, "%s/object%llu", ch->s->base_dir, toi);
#endif
					}
#else

#ifdef WIN32
					sprintf(filename, "%s\\object%I64u", ch->s->base_dir, toi);
#else
					sprintf(filename, "%s/object%llu", ch->s->base_dir, toi);
#endif

#endif
					/* Alloc memory for tmp_filename */
					if(!(trans_obj->tmp_filename = (char*)calloc(strlen(filename)+1, sizeof(char)))) {
				  		printf("Could not alloc memory for tmp_filename!\n");
				  		return MEM_ERROR;
					}

					memcpy(trans_obj->tmp_filename, filename, strlen(filename));

#ifdef WIN32
        			if((trans_obj->fd = open((const char*)trans_obj->tmp_filename,
											 _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC , _S_IWRITE)) < 0) {
#else
        			if((trans_obj->fd = open(trans_obj->tmp_filename,
											 O_WRONLY | O_CREAT | O_TRUNC , S_IRWXU)) < 0) {
#endif
                		printf("Error: unable to open file %s\n", trans_obj->tmp_filename);
                		fflush(stdout);
                		return MEM_ERROR;
        			}
				}
			}

			trans_obj->len = toilen;
			trans_obj->fec_enc_id = (unsigned char)fec_enc_id;
			trans_obj->fec_inst_id = (unsigned short)fec_inst_id;
			trans_obj->eslen = eslen;
			trans_obj->max_sblen = max_sblen;

			 /* Let's calculate the blocking structure for this object */

			trans_obj->bs = compute_blocking_structure(toilen, max_sblen, eslen);

			if(toi == FDT_TOI) {
				insert_object(trans_obj, ch->s, 0);
			}
			else {
				insert_object(trans_obj, ch->s, 1);
			}
		}

		/* Check if block already exist */
		trans_block = block_exist(sbn, trans_obj);
		
		if(trans_block == NULL) {

			trans_block = create_block();

			if(trans_block == NULL) {
				return MEM_ERROR;
			}

			trans_block->sbn = sbn;	

			if(fec_enc_id == COM_NO_C_FEC_ENC_ID) { 

				if(sbn < trans_obj->bs->I) {
					trans_block->k = trans_obj->bs->A_large;
				}
				else {
					trans_block->k = trans_obj->bs->A_small;
				}
			}
			else if(fec_enc_id == SB_SYS_FEC_ENC_ID) {

				trans_block->k = sblen;
				trans_block->max_k = max_sblen;
				trans_block->max_n = max_nb_of_es;
			}
			else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
				
				if(sbn < trans_obj->bs->I) {
					trans_block->k = trans_obj->bs->A_large;
                }
                else {
					trans_block->k = trans_obj->bs->A_small;
                }

				trans_block->max_k = max_sblen;
			}	
		
			insert_block(trans_block, trans_obj);
		}

		if(!block_ready_to_decode(trans_block)) {

			if(insert_unit(trans_unit, trans_block, trans_obj) != 1) {

#ifdef WIN32
am_progress((double)((double)100 * ((double)(LONGLONG)trans_obj->rx_bytes/(double)(LONGLONG)trans_obj->len)), toi);
#else
am_progress((double)((double)100 * ((double)(long long)trans_obj->rx_bytes/(double)(long long)trans_obj->len)), toi);
#endif
				if(ch->s->verbosity) {
#ifdef WIN32
					printf("%.2f%% of object received,",
						(double)((double)100 * ((double)(LONGLONG)trans_obj->rx_bytes/(double)(LONGLONG)trans_obj->len)));
					
					/*printf("%i%% of object received,",
						(int)((double)100 * ((double)(LONGLONG)trans_obj->rx_bytes/(double)(LONGLONG)trans_obj->len)));
					*/
#else
					printf("%.2f%% of object received,",
                                                (double)((double)100 * ((double)(long long)trans_obj->rx_bytes/(double)(long long)trans_obj->len)));
					/*printf("%i%% of object received,",
                                                (int)((double)100 * ((double)(long long)trans_obj->rx_bytes/(double)(long long)trans_obj->len)));*/
#endif

#ifdef WIN32
					printf(" TOI: %I64u", toi);
#else
					printf(" TOI: %llu", toi);
#endif

					printf(" (SBN: %i ESI: %i LAYERS=%i)\n", sbn, esid, ch->s->nb_channel);
					fflush(stdout);
				}
				retval = OK;
			}
			else {
				free(trans_unit->data);
				free(trans_unit);
				retval = DUP_PACKET;
			}
		}
		else {
			free(trans_unit->data);
			free(trans_unit);
			return DUP_PACKET;
		}

		if(toi != FDT_TOI) {
		
			if(ch->s->big_file_mode) {	
		  		if(block_ready_to_decode(trans_block)) {

					trans_obj->nb_of_ready_blocks++;

					printf("[%i/%i Source Blocks decoded]\n", trans_obj->nb_of_ready_blocks,
							trans_obj->bs->N);
					fflush(stdout);

		    		/* decode and save data */
		
					if(fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
		      			buf = null_fec_decode_src_block(trans_block, &block_len, eslen);

/* Possible your own NULL-FEC library
#elif YOUR_OWN_NULL_FEC
							buf = your_own_null_fec(trans_block, &block_len, eslen);			
*/							
#endif
					}
#ifdef USE_SIMPLE_XOR
                    else if(fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                            buf = xor_fec_decode_src_block(trans_block, &block_len, eslen);
                    }
#endif

#ifdef USE_REED_SOLOMON
					else if(fec_enc_id == SB_SYS_FEC_ENC_ID && fec_inst_id == REED_SOL_FEC_INST_ID) {		
						buf = rs_fec_decode_src_block(trans_block, &block_len, eslen);
					}
#endif
					if(buf == NULL) {
						return MEM_ERROR;
					}

		      		if(trans_block->sbn < trans_obj->bs->I) {
						pos = ((trans_block->sbn) * (trans_obj->bs->A_large)) * eslen;
		      		}
		      		else {
						pos = ( ( (trans_obj->bs->I) * (trans_obj->bs->A_large) ) + 
			       				( (trans_block->sbn - trans_obj->bs->I) * (trans_obj->bs->A_small) ) ) * eslen;
		      		}

				/* We have to check if there is padding in the last source symbol of the last source block */
		
		      		if(trans_block->sbn == ((trans_obj->bs->N) - 1)) {
						block_len = (trans_obj->len - (eslen * (trans_obj->bs->I * trans_obj->bs->A_large +
							    (trans_obj->bs->N - trans_obj->bs->I -1) * trans_obj->bs->A_small)));
		      		}
		      		/*else {
						block_len = trans_block->len * eslen;
		      		}*/
		
					/* set correct position */

					if(lseek(trans_obj->fd, (unsigned int)pos, SEEK_SET) == -1) {
#ifdef WIN32
						printf("lseek error, toi: %I64u\n", toi);
#else
						printf("lseek error, toi: %llu\n", toi);
#endif
						fflush(stdout);
						free(buf);
						set_session_state(ch->s->s_id, SExiting);
						return MEM_ERROR;
					}
					
					if(write(trans_obj->fd, buf, (unsigned int)block_len) == -1) {
#ifdef WIN32
						printf("write error, toi: %I64u, sbn: %i\n", toi, sbn);
#else
						printf("write error, toi: %llu, sbn: %i\n", toi, sbn);
#endif
						fflush(stdout);
						free(buf);
						set_session_state(ch->s->s_id, SExiting);
						return MEM_ERROR;
					}

		      		free(buf);
					trans_block->completed = true;
				}
			}
		}
	}
	else { /* We have an empty packet with FEC Payload ID */
		retval = EMPTY_PACKET;	
	}

	return retval;
}

/* 
 * This function gives an object to application, when it is completely received.
 *
 * Params:	int s_id: Session identifier,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			ULONGLONG/unsigned long long *datalen: Pointer to object length,
 *			int *retval: Return value in error cases/stopping situations.
 *
 * Return:	char*: Pointer to buffer which contains object's data,
 *			NULL: In errors/stopping situations
 *
 */

char* alc_recv(int s_id, 
#ifdef WIN32			   
			   ULONGLONG toi,
			   ULONGLONG *data_len,
#else
			   unsigned long long toi,
			   unsigned long long *data_len,
#endif
			   int *retval) {

	bool obj_completed = false;
	alc_session_t *s;
	char *buf = NULL; /* Buffer where to construct the object from data units */
	trans_obj_t *to;
	int object_exists = 0;
	
	s = get_alc_session(s_id);

	while(!obj_completed) {

		if(s->state == SExiting) {
			/*printf("alc_recv() SExiting\n");
			fflush(stdout);*/
			*retval = -2;
			return NULL;	
		}
		else if(s->state == SClosed) {
			/*printf("alc_recv() SClosed\n");
			fflush(stdout);*/
			*retval = 0;
			return NULL;	
		}

		to = s->obj_list;

		if(!object_exists) {

			while(to != NULL) {
				if(to->toi == toi) {
					object_exists = 1;
					break;
				}
				to = to->next;
			}

			if(to == NULL) {
				continue;
			}
		}

		obj_completed = object_completed(to);

		if(((s->state == STxStopped) && (!obj_completed))) {
			/*printf("alc_recv() STxStopped, toi: %i\n", toi);
			fflush(stdout);*/
			*retval = -3;
			return NULL;	
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}
	printf("\n");

	remove_wanted_object(s_id, toi);

	/* Parse data from object to data buffer, return buffer and buffer length */

    to = object_exist(toi, s, 1);

	if(to->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
		buf = null_fec_decode_object(to, data_len, s);

/* Possible your own NULL-FEC library
#elif YOUR_OWN_NULL_FEC
		buf = your_own_null_fec_decode_object(to, data_len, s);			
*/							
#endif
	}

#ifdef USE_SIMPLE_XOR
        else if(to->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                buf = xor_fec_decode_object(to, data_len, s);
        }
#endif

#ifdef USE_REED_SOLOMON
	else if(to->fec_enc_id == SB_SYS_FEC_ENC_ID && to->fec_inst_id == REED_SOL_FEC_INST_ID) {
		buf = rs_fec_decode_object(to, data_len, s);
	}
#endif
			
	if(buf == NULL) {
		*retval = -1;
	}

	free_object(to, s, 1);
	return buf;
}

/* 
 * This function gives any object to application, when it is completely received.
 *
 * Params:	int s_id: Session identifier,
 *			ULONGLONG/unsigned long long *toi: Pointer to Transport Object Identifier,
 *			ULONGLONG/unsigned long long *datalen: Pointer to object length,
 *			int *retval: Return value in error cases/stopping situations.
 *
 * Return:	char*: Pointer to buffer which contains object's data,
 *			NULL: In errors/stopping situations
 *
 */

char* alc_recv2(int s_id,
#ifdef WIN32			   
			   ULONGLONG *toi,
			   ULONGLONG *data_len,
#else
			   unsigned long long *toi,
			   unsigned long long *data_len,
#endif
			   int *retval) {

	bool obj_completed = false;
	alc_session_t *s;

#ifdef WIN32			   
	ULONGLONG tmp_toi;
#else
	unsigned long long tmp_toi;
#endif

	char *buf = NULL; /* Buffer where to construct the object from data units */
	trans_obj_t *to;
	
	s = get_alc_session(s_id);

	while(1) {
		
		to = s->obj_list;

		if(s->state == SExiting) {
			/*printf("alc_recv2() SExiting\n");
			fflush(stdout);*/
			*retval = -2;
			return NULL;	
		}
		else if(s->state == SClosed) {
			/*printf("alc_recv2() SClosed\n");
			fflush(stdout);*/
			*retval = 0;
			return NULL;	
		}
		else if(((s->state == STxStopped) && (to == NULL))) {
			/*printf("alc_recv2() STxStopped\n");
			fflush(stdout);*/
			*retval = -3;
			return NULL;	
		}

		while(to != NULL) {

			if(s->state == SExiting) {
				/*printf("alc_recv2() SExiting\n");
				fflush(stdout);*/
				*retval = -2;
				return NULL;	
			}
			else if(s->state == SClosed) {
				/*printf("alc_recv2() SClosed\n");
				fflush(stdout);*/
				*retval = 0;
				return NULL;	
			}

			obj_completed = object_completed(to);

			if(obj_completed) {
				tmp_toi = to->toi;
				break;
			}

			if(((s->state == STxStopped) && (!obj_completed))) {
				/*printf("alc_recv2() STxStopped\n");
				fflush(stdout);*/
				*retval = -3;
				return NULL;	
			}

			to = to->next;
		}
	
		if(obj_completed) {
			break;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}

	printf("\n");

	remove_wanted_object(s_id, tmp_toi);

	/* Parse data from object to data buffer, return buffer length */

	to = object_exist(tmp_toi, s, 1);

	if(to->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
		buf = null_fec_decode_object(to, data_len, s);

/* Possible your own NULL-FEC library
#elif YOUR_OWN_NULL_FEC
		buf = your_own_null_fec_decode_object(to, data_len, s);			
*/							
#endif
	}

#ifdef USE_SIMPLE_XOR
        else if(to->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                buf = xor_fec_decode_object(to, data_len, s);
        }
#endif

#ifdef USE_REED_SOLOMON
	else if(to->fec_enc_id == SB_SYS_FEC_ENC_ID && to->fec_inst_id == REED_SOL_FEC_INST_ID) {
		buf = rs_fec_decode_object(to, data_len, s);
	}
#endif
			
	if(buf == NULL) {
		*retval = -1;
	}
	else {
		*toi = tmp_toi;
	}

	free_object(to, s, 1);
	return buf;
}

/*
 * This function gives temp filename of object to application, when object is completely received.
 *
 * Params:	int s_id: Session identifier,
 *			ULONGLONG/unsigned long long *toi: Pointer to Transport Object Identifier,
 *			char *tmp_filename: Pointer to object's temporary filename.
 *			int *retval: Return value in error cases/stopping situations.
 *
 * Return:	char*: Pointer to buffer which contains temporary filename,
 *			NULL: In errors/stopping situations.
 *
 */

char* alc_recv3(int s_id,
#ifdef WIN32			   
				ULONGLONG *toi,
#else
				unsigned long long *toi,
#endif
				int *retval) {

	bool obj_completed = false;
	alc_session_t *s;

#ifdef WIN32			   
	ULONGLONG tmp_toi = 0;
#else
	unsigned long long tmp_toi = 0;
#endif

	trans_obj_t *to;
	char *tmp_filename = NULL;
	
	s = get_alc_session(s_id);

	while(1) {
		
		to = s->obj_list;

		if(s->state == SExiting) {
			/*printf("alc_recv3() SExiting\n");
			fflush(stdout);*/
			*retval = -2;
			return NULL;	
		}
		else if(s->state == SClosed) {
			/*printf("alc_recv3() SClosed\n");
			fflush(stdout);*/
			*retval = 0;
			return NULL;	
		}
		else if(((s->state == STxStopped) && (to == NULL))) {
			/*printf("alc_recv3() STxStopped\n");
			fflush(stdout);*/
			*retval = -3;
			return NULL;	
		}

		while(to != NULL) {

			if(s->state == SExiting) {
				/*printf("alc_recv3() SExiting\n");
				fflush(stdout);*/
				*retval = -2;
				return NULL;	
			}
			else if(s->state == SClosed) {
				/*printf("alc_recv3() SClosed\n");
				fflush(stdout);*/
				*retval = 0;
				return NULL;	
			}

			obj_completed = object_completed2(to);

			if(obj_completed) {
				tmp_toi = to->toi;
				break;
			}

			if(((s->state == STxStopped) && (!obj_completed))) {
				/*printf("alc_recv3() STxStopped\n");
				fflush(stdout);*/
				*toi = tmp_toi;
				*retval = -3;
				return NULL;	
			}

			to = to->next;
		}
	
		if(obj_completed) {
			break;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}

	printf("\n");

	remove_wanted_object(s_id, tmp_toi);

	if(!(tmp_filename = (char*)calloc((strlen(to->tmp_filename) + 1), sizeof(char)))) {
		printf("Could not alloc memory for tmp_filename!\n");
		*retval = -1;
		return NULL;    
	}

	memcpy(tmp_filename, to->tmp_filename, strlen(to->tmp_filename));
	free_object(to, s, 1);
	*toi = tmp_toi;

  	return tmp_filename;
}

/* 
 * This function gives an FDT Instance to application, when it is completely received.
 *
 * Params:	int s_id: Session identifier,
 *			ULONGLONG/unsigne dlong long *data_len: Pointer to FDT Instance length,
 *			int *retval: Return value in error cases/stopping situations.
 *			unsigned char *cont_enc_algo: Location where to store used content encoding,
 *
 * Return:	char*: Pointer to buffer which contains FDT Instance's data,
 *			NULL: in errors/stopping situations
 *
 */
 

char* fdt_recv(int s_id,
#ifdef WIN32
				ULONGLONG *data_len,
#else
				unsigned long long *data_len,
#endif
				int *retval
#ifdef USE_ZLIB
				, unsigned char *cont_enc_algo
#endif
			   ) {
	
	alc_session_t *s;                                                                                                                                          
	char *buf = NULL; /* Buffer where to construct the object from data units */                                                                                                                                     
	trans_obj_t *to;
        
	s = get_alc_session(s_id);
        
	while(1) {
		to = s->fdt_list;

		if(s->state == SExiting) {
			/*printf("fdt_recv() SExiting\n");
			fflush(stdout);*/
			*retval = -2;
			return NULL;
		} else if(s->state == SClosed) {
            /*printf("fdt_recv() SClosed\n");
            fflush(stdout);*/
			*retval = 0;
			return NULL;
		} else if(s->state == STxStopped) {
			/*printf("fdt_recv() STxStopped\n");
			fflush(stdout);*/
			*retval = -3;
			return NULL;	
		}
	
		if(to == NULL) {
			                                                                                                                                              
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
			continue;	
		}
		
		do {

			if(object_completed(to)) {
				set_received_instance(s, (unsigned int)to->toi);

#ifdef USE_ZLIB
				*cont_enc_algo = to->cont_enc_algo;
#endif

				if(to->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
		      		buf = null_fec_decode_object(to, data_len, s);

/* Possible your own NULL-FEC library
#elif YOUR_OWN_NULL_FEC
					buf = your_own_null_fec_decode_object(to, data_len, s);			
*/							
#endif
				}
#ifdef USE_SIMPLE_XOR
        			else if(to->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                			buf = xor_fec_decode_object(to, data_len, s);
        			}
#endif

#ifdef USE_REED_SOLOMON
				else if(to->fec_enc_id == SB_SYS_FEC_ENC_ID && to->fec_inst_id == REED_SOL_FEC_INST_ID) {
					buf = rs_fec_decode_object(to, data_len, s);
				}
#endif
				if(buf == NULL) {
					*retval = -1;
				}

				free_object(to, s, 0);
				return buf;
			}
			to = to->next;
		} while(to != NULL);

#ifdef WIN32
        Sleep(1);
#else
		usleep(1000);
#endif
	}

	return buf;
}


/* 
 * This function checks if received unit belongs to an object which already exists in session or not. 
 *
 * Params:	ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			alc_session_t *s: Pointer to session,
 *			int type: Type of object to be checked (0 = FDT Instance, 1 = normal object)
 *
 * Return:	trans_obj_t*: Pointer to object in success, NULL otherwise 
 *
 */

trans_obj_t* object_exist(
#ifdef WIN32
						  ULONGLONG toi,
#else
						  unsigned long long toi,
#endif
						  alc_session_t *s, int type) {
	
	trans_obj_t *trans_obj;

	if(type == 0) {
		trans_obj = s->fdt_list;
	}
	else if(type == 1) {
		trans_obj = s->obj_list;
	}

	if(trans_obj != NULL) {
		for(;;) {
			if(trans_obj->toi == toi) {
				break;
			}
			if(trans_obj->next == NULL) {
				trans_obj = NULL;
				break;
			}
			trans_obj = trans_obj->next;
		}
	}

	return trans_obj;
}

/*
 * This function checks if received unit belongs to a source block, which already exists or not. 
 *
 * Params:	unsigned int sbn: Source Block Number,
 *			trans_obj_t *trans_obj: Pointer to object
 *
 * Return:	trans_block_t*: Pointer to block in success, NULL otherwise 
 *
 */

trans_block_t* block_exist(unsigned int sbn, trans_obj_t *trans_obj) {
	
	trans_block_t *trans_block;

	trans_block = trans_obj->block_list;

	if(trans_block != NULL) {
		for(;;) {
			if(trans_block->sbn == sbn) {
				break;
			}
			if(trans_block->next == NULL) {
				trans_block = NULL;
				break;
			}
			trans_block = trans_block->next;
		}
	}

	return trans_block;
}

/*
 * This function checks if the object has been received completely.
 *
 * Params:	trans_obj_t *to: Transport Object to be checked
 *
 * Return:	bool: true when object ready, false otherwise
 *
 */

bool object_completed(trans_obj_t *to) {

	bool ready = false;
	trans_block_t *tb = to->block_list;

	if(to->nb_of_created_blocks != to->bs->N) {
		return ready;
	}

    while(tb != NULL) {

        if(tb->nb_of_rx_units < tb->k) {
			return ready;
        }
        tb = tb->next;
    }

    ready = true;

	return ready;
}

/*
 * This function checks if the object has been received completely.
 *
 * Params:      trans_obj_t *to: Transport Object to be checked
 *
 * Return:      bool: true when object ready, false otherwise
 *
 */

bool object_completed2(trans_obj_t *to) {

    bool ready = false;
    trans_block_t *tb = to->block_list;

    if(to->nb_of_created_blocks != to->bs->N) {
		return ready;
	}

    while(tb != NULL) {

        if(!tb->completed) {
			return ready;
        }
        tb = tb->next;
    }

	ready = true;

    return ready;
}

/*
 * This function checks if the block has been received completely.
 *
 * Params:      trans_block_t *tb: Pointer to block
 *
 * Return:      bool: true when object ready, false otherwise
 *
 */

bool block_ready_to_decode(trans_block_t *tb) {

	bool ready = true;

	if(tb->nb_of_rx_units < tb->k) {
		ready = false;
	}

	return ready;
}
