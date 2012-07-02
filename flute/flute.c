/* $Author: peltotal $ $Date: 2004/12/28 10:16:12 $ $Revision: 1.65 $ */
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

#include "../alclib/inc.h"

#include "flute.h"
#include "sender.h"
#include "receiver.h"
#include "uri.h"
#include "fdt_gen.h"

#ifdef USE_ZLIB
#include "mad_zlib.h"
#endif

#ifdef USE_OPENSSL
#include "mad_md5.h"
#endif

/**** Private functions ****/

int all_files_received(char *filetable[10], int file_nb);
void file_received(char *filetable[10], int file_nb, char *filepath);

/*
 * This function was flute sender's main function.
 *
 * Params:	arguments_t *a: Pointer to arguments structure where command line arguments are parsed,
 *			int *s_id: Pointer to session identifier,
 *			int *ch_id: Pointer to channel identifier.
 *
 * Return:	int: Different values (0, -1, -2, -4) depending on how function ends.
 *
 */

int flute_sender_prepare(arguments_t *a, int *s_id, int *ch_id)
{
	unsigned short i;
	int j, k, l, m;
	int retval = 0;
	int retcode = 0;

	struct sockaddr_in ipv4;
	unsigned long addr_nb;
	int port_nb;
	unsigned short ipv6addr[8];
	int nb_ipv6_part;
	int dup_sep = 0;
	char tmp[5];

	char addrs[MAX_CHANNELS_IN_SESSION][MAX_LENGTH];	/* Mcast addresses on which to send */
	char ports[MAX_CHANNELS_IN_SESSION][MAX_LENGTH];	/* Local port numbers  */

	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

#ifdef USE_SDPLIB
	if(strcmp(a->sdp_file, "") != 0) {

		if(parse_sdp_file(a, addrs, ports) == -1) {
			return -1;
		}
	}
	else {

		if(a->addr_family == PF_INET) {
 
			addr_nb = ntohl(inet_addr(a->addr));
			port_nb = atoi(a->port);

			for(j = 0; j < a->nb_channel; j++) {

				ipv4.sin_addr.s_addr = htonl(addr_nb);

				memset(addrs[j], 0, 256);
				memset(ports[j], 0, 256);

				sprintf(addrs[j], "%s", inet_ntoa(ipv4.sin_addr));
				sprintf(ports[j], "%i", port_nb);

				addr_nb++;
				port_nb++;
			}
		}
		else if(a->addr_family == PF_INET6) {
			
			port_nb = atoi(a->port);

			retcode = ushort_ipv6addr(a->addr, &ipv6addr[0], &nb_ipv6_part);

			if(retcode == -1) {
				return -1;				
			}

			for(j = 0; j < a->nb_channel; j++) {

				memset(addrs[j], 0, 256);
				memset(ports[j], 0, 256);
				dup_sep = 0;

				for(k = 0; k < nb_ipv6_part + 1; k++) {

					memset(tmp, 0, 5);

					if(ipv6addr[k] != 0) {
						sprintf(tmp, "%x", ipv6addr[k]);
						strcat(addrs[j], tmp);
					}
					else {
						if(dup_sep == 0) {
							dup_sep = 1;
						}
						else {
							printf("Invalid IPv6 address!\n");
							fflush(stdout);
							return -1;	
						}	
					}

					if(k != nb_ipv6_part) {
						strcat(addrs[j], ":");
					}
				}

				sprintf(ports[j], "%i", port_nb);

				port_nb++;

				if(j != a->nb_channel - 1) {

					for(l = nb_ipv6_part;; l--) {

						if(l == 0) {
							printf("Only %i channel possible!\n", (j + 1));
							fflush(stdout);
							return -1;	
						}

						if(ipv6addr[l] + 1 > 0xFFFF) {
							continue;
						}
							
						if(ipv6addr[l] == 0) {

							if(nb_ipv6_part == 7) {
								ipv6addr[l]++;
								break;
							}

							for(m = nb_ipv6_part; m > l; m--) {
								ipv6addr[m + 1] = ipv6addr[m];	
							}

							ipv6addr[l + 1] = 1;

							nb_ipv6_part++;
							break;
						}
						else {
							ipv6addr[l]++;
							break;
						}
					}
				}
			}
		}
	}
#else
	if(a->addr_family == PF_INET) {
 
		addr_nb = ntohl(inet_addr(a->addr));
		port_nb = atoi(a->port);

		for(j = 0; j < a->nb_channel; j++) {

			ipv4.sin_addr.s_addr = htonl(addr_nb);

			memset(addrs[j], 0, 256);
			memset(ports[j], 0, 256);

			sprintf(addrs[j], "%s", inet_ntoa(ipv4.sin_addr));
			sprintf(ports[j], "%i", port_nb);

			addr_nb++;
			port_nb++;
		}
	}
	else if(a->addr_family == PF_INET6) {
			
		port_nb = atoi(a->port);

		retcode = ushort_ipv6addr(a->addr, &ipv6addr[0], &nb_ipv6_part);

		if(retcode == -1) {
			return -1;				
		}

		for(j = 0; j < a->nb_channel; j++) {

			memset(addrs[j], 0, 256);
			memset(ports[j], 0, 256);
			dup_sep = 0;

			for(k = 0; k < nb_ipv6_part + 1; k++) {

				memset(tmp, 0, 5);

				if(ipv6addr[k] != 0) {
					sprintf(tmp, "%x", ipv6addr[k]);
					strcat(addrs[j], tmp);
				}
				else {
					if(dup_sep == 0) {
						dup_sep = 1;
					}
					else {
						printf("Invalid IPv6 address!\n");
						fflush(stdout);
						return -1;	
					}	
				}

				if(k != nb_ipv6_part) {
					strcat(addrs[j], ":");
				}
			}

			sprintf(ports[j], "%i", port_nb);

			port_nb++;

			if(j != a->nb_channel - 1) {

				for(l = nb_ipv6_part;; l--) {

					if(l == 0) {
						printf("Only %i channel possible!\n", (j + 1));
						fflush(stdout);
						return -1;	
					}

					if(ipv6addr[l] + 1 > 0xFFFF) {
						continue;
					}
						
					if(ipv6addr[l] == 0) {

						if(nb_ipv6_part == 7) {
							ipv6addr[l]++;
							break;
						}

						for(m = nb_ipv6_part; m > l; m--) {
							ipv6addr[m + 1] = ipv6addr[m];	
						}

						ipv6addr[l + 1] = 1;

						nb_ipv6_part++;
						break;
					}
					else {
						ipv6addr[l]++;
						break;
					}
				}
			}
		}
	}
#endif

	if(a->stoptime != 0) {
		time(&systime);
		curr_time = systime + 2208988800;

		if(a->stoptime <= curr_time) {
			printf("Session end time reached\n");
			fflush(stdout);
			return -1;
		}
	}

	*s_id = open_alc_session(a->mode, a->tsi, a->starttime, a->stoptime, NULL, a->addr_family, a->addr_type,
							 a->ttl, a->fec_enc_id, a->fec_inst_id, a->max_sblen, a->eslen, a->cc_id,
							 a->use_fec_oti_ext_hdr, a->txrate, 0, 0, a->simul_losses, a->fec_ratio, 
							 a->verbosity, a->base_dir, a->half_word, 0, a->loss_ratio1, a->loss_ratio2
#ifdef SSM
							 , false
#endif

#ifdef USE_ZLIB
							 , a->encode_content);
#else
							 );
#endif

	for(i = 0; (int)i < a->nb_channel; i++) {
	
		if(a->addr_type == 1) {
			*(ch_id + i) = add_alc_channel(*s_id, ports[i], addrs[0], a->intface, a->intface_name);
		}
		else {
			*(ch_id + i) = add_alc_channel(*s_id, ports[i], addrs[i], a->intface, a->intface_name);
		}

		if(*(ch_id + i) == -1) {

			while(i) {
				retcode = remove_alc_channel(*s_id, *(ch_id + i - 1));
				i--;
			}

			close_alc_session(*s_id);
			return -4;
		}	
	}

#ifndef WIN32
	pthread_attr_t attr;
#endif
	alc_session_t *s = get_alc_session(*s_id);

#ifdef USE_RLC
	if(((a->cc_id == RLC) || ((a->cc_id == Null) && (a->nb_channel != 1)))) {
#else
	if(((a->cc_id == Null) && (a->nb_channel != 1))) {
#endif
		/**** Start tx_thread ****/

#ifdef WIN32
		if(CreateThread(NULL, 0, (void*)tx_thread, (void*)s, 0, &s->tx_thread_id) == NULL) {
			perror("flute_sender_prepare: CreateThread");
			return -1;
		}	
#else
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 

		if (pthread_create(&s->tx_thread_id, &attr, tx_thread, (void*)s) != 0) {
			perror("flute_sender_prepare: pthread_create");
			return -1;
		}
#endif

	}
#if 0
	/* Generate fdt file first */
	if(strcmp(a->fdt_file, "") == 0) {

		memset(a->fdt_file, 0, MAX_LENGTH);
		strcpy(a->fdt_file, "fdt_tsi");

		memset(tmp, 0, 5);

#ifdef WIN32
		sprintf(tmp, "%I64u", a->tsi);
#else
		sprintf(tmp, "%llu", a->tsi);
#endif
		
		strcat(a->fdt_file, tmp);
		strcat(a->fdt_file, ".xml");

		retcode = generate_fdt(a->file_path, a->base_dir, s_id, a->fdt_file);

		if(retcode < 0) {
			return -1;
		}
	}

	/***** FDT based send *****/
	retval = sender_in_fdt_based_mode(s_id, ch_id, a);

	return retval;
#endif
	return 0;
}

/*
 * This function is flute receiver's main function.
 *
 * Params:	arguments_t *a: Pointer to arguments structure where command line arguments are parsed,
 *			int *s_id: Pointer to session identifier,
 *			int *ch_id: Pointer to channel identifier.
 *
 *
 * Return:	int: Different values (0, -1, -2, -3, -4, -5) depending on how function ends.
 *
 */

int flute_receiver_prepare(arguments_t *a, int *s_id, int *ch_id) {
	
	unsigned short i;
	int j, k, l, m;
	int retval = 0;
	int retcode = 0;

	char filetoken[MAX_PATH];

	struct sockaddr_in ipv4;
	unsigned long addr_nb;
	int port_nb;
	unsigned short ipv6addr[8];
	int nb_ipv6_part;
	int dup_sep = 0;
	char tmp[5];

	char addrs[MAX_CHANNELS_IN_SESSION][MAX_LENGTH];	/* Mcast addresses on which to send */
	char ports[MAX_CHANNELS_IN_SESSION][MAX_LENGTH];	/* Local port numbers  */

	time_t systime;
	bool is_printed = false;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

#ifdef WIN32
	if(mkdir(a->base_dir) < 0) {					
#else		
	if(mkdir(a->base_dir, S_IRWXU) < 0) {
#endif
		if(errno != EEXIST) {
			printf("mkdir failed: cannot create directory %s (errno=%i)\n", a->base_dir, errno);
			fflush(stdout);
	      	return -1;
	    }
	}

#ifdef USE_SDPLIB
	if(strcmp(a->sdp_file, "") != 0) {

		if(parse_sdp_file(a, addrs, ports) == -1) {
			return -1;
		}

		if(a->nb_channel == 0) {
			printf("Error: No acceptable channels found in SDP.");
			fflush(stdout);
			return -1;
		}
	}
	else {
		if(((a->cc_id == Null) && (a->nb_channel != 1))) {

			if(a->addr_family == PF_INET) {
 
				addr_nb = ntohl(inet_addr(a->addr));
				port_nb = atoi(a->port);

				for(j = 0; j < a->nb_channel; j++) {

					ipv4.sin_addr.s_addr = htonl(addr_nb);

					memset(addrs[j], 0, MAX_LENGTH);
					memset(ports[j], 0, MAX_LENGTH);

					sprintf(addrs[j], "%s", inet_ntoa(ipv4.sin_addr));
					sprintf(ports[j], "%i", port_nb);

					addr_nb++;
					port_nb++;
				}
			}
			else if(a->addr_family == PF_INET6) {
					
				port_nb = atoi(a->port);

				retcode = ushort_ipv6addr(a->addr, &ipv6addr[0], &nb_ipv6_part);

				if(retcode == -1) {
					return -1;				
				}

				for(j = 0; j < a->nb_channel; j++) {

					memset(addrs[j], 0, MAX_LENGTH);
					memset(ports[j], 0, MAX_LENGTH);
					dup_sep = 0;

					for(k = 0; k < nb_ipv6_part + 1; k++) {

						memset(tmp, 0, 5);

						if(ipv6addr[k] != 0) {
							sprintf(tmp, "%x", ipv6addr[k]);
							strcat(addrs[j], tmp);
						}
						else {
							if(dup_sep == 0) {
								dup_sep = 1;
							}
							else {
								printf("Invalid IPv6 address!\n");
								fflush(stdout);
								return -1;	
							}	
						}

						if(k != nb_ipv6_part) {
							strcat(addrs[j], ":");
						}
					}

					sprintf(ports[j], "%i", port_nb);

					port_nb++;

					if(j != a->nb_channel - 1) {

						for(l = nb_ipv6_part;; l--) {

							if(l == 0) {
								printf("Only %i channel possible!\n", (j + 1));
								fflush(stdout);
								return -1;	
							}

							if(ipv6addr[l] + 1 > 0xFFFF) {
								continue;
							}
								
							if(ipv6addr[l] == 0) {

								if(nb_ipv6_part == 7) {
									ipv6addr[l]++;
									break;
								}

								for(m = nb_ipv6_part; m > l; m--) {
									ipv6addr[m + 1] = ipv6addr[m];	
								}

								ipv6addr[l + 1] = 1;

								nb_ipv6_part++;
								break;
							}
							else {
								ipv6addr[l]++;
								break;
							}
						}
					}
				}
			}
		}
		else {
			memset(addrs[0], 0, MAX_LENGTH);
			memset(ports[0], 0, MAX_LENGTH);

			memcpy(addrs[0], a->addr, strlen(a->addr));
			memcpy(ports[0], a->port, strlen(a->port));
		}
	}
#else
	if(((a->cc_id == Null) && (a->nb_channel != 1))) {

		if(a->addr_family == PF_INET) {
 
			addr_nb = ntohl(inet_addr(a->addr));
			port_nb = atoi(a->port);

			for(j = 0; j < a->nb_channel; j++) {

				ipv4.sin_addr.s_addr = htonl(addr_nb);

				memset(addrs[j], 0, MAX_LENGTH);
				memset(ports[j], 0, MAX_LENGTH);

				sprintf(addrs[j], "%s", inet_ntoa(ipv4.sin_addr));
				sprintf(ports[j], "%i", port_nb);

				addr_nb++;
				port_nb++;
			}
		}
		else if(a->addr_family == PF_INET6) {
				
			port_nb = atoi(a->port);

			retcode = ushort_ipv6addr(a->addr, &ipv6addr[0], &nb_ipv6_part);

			if(retcode == -1) {
				return -1;				
			}

			for(j = 0; j < a->nb_channel; j++) {

				memset(addrs[j], 0, MAX_LENGTH);
				memset(ports[j], 0, MAX_LENGTH);
				dup_sep = 0;

				for(k = 0; k < nb_ipv6_part + 1; k++) {

					memset(tmp, 0, 5);

					if(ipv6addr[k] != 0) {
						sprintf(tmp, "%x", ipv6addr[k]);
						strcat(addrs[j], tmp);
					}
					else {
						if(dup_sep == 0) {
							dup_sep = 1;
						}
						else {
							printf("Invalid IPv6 address!\n");
							fflush(stdout);
							return -1;	
						}	
					}

					if(k != nb_ipv6_part) {
						strcat(addrs[j], ":");
					}
				}

				sprintf(ports[j], "%i", port_nb);

				port_nb++;

				if(j != a->nb_channel - 1) {

					for(l = nb_ipv6_part;; l--) {

						if(l == 0) {
							printf("Only %i channel possible!\n", (j + 1));
							fflush(stdout);
							return -1;	
						}

						if(ipv6addr[l] + 1 > 0xFFFF) {
							continue;
						}
							
						if(ipv6addr[l] == 0) {

							if(nb_ipv6_part == 7) {
								ipv6addr[l]++;
								break;
							}

							for(m = nb_ipv6_part; m > l; m--) {
								ipv6addr[m + 1] = ipv6addr[m];	
							}

							ipv6addr[l + 1] = 1;

							nb_ipv6_part++;
							break;
						}
						else {
							ipv6addr[l]++;
							break;
						}
					}
				}
			}
		}
	}
	else {
		memset(addrs[0], 0, MAX_LENGTH);
		memset(ports[0], 0, MAX_LENGTH);

		memcpy(addrs[0], a->addr, strlen(a->addr));
		memcpy(ports[0], a->port, strlen(a->port));
	}
#endif

	if(a->stoptime != 0) {
		time(&systime);
		curr_time = systime + 2208988800;

		if(a->stoptime <= curr_time) {
			printf("Session end time reached\n");
			fflush(stdout);
			return -1;
		}
	}

	*s_id = open_alc_session(a->mode, a->tsi, a->starttime, a->stoptime, a->srcaddr, a->addr_family, a->addr_type,
							 0, 0, 0, 0, 0, a->cc_id, 0, 0, a->big_file_mode, a->nb_channel, false, 0, a->verbosity,
							 a->base_dir, false, a->accept_expired_fdt_inst, 0, 0
#ifdef SSM
							 , a->use_ssm
#endif

#ifdef USE_ZLIB
							 , 0);
#else
							 );
#endif

	if(a->starttime != 0) {
		while(1) {

			time(&systime);
			curr_time = systime + 2208988800;

			if((a->starttime - 3) > curr_time) {

				if(!is_printed) {
					printf("Waiting for session start time...\n");
					fflush(stdout);
					is_printed = true;
				}
#ifdef WIN32
				Sleep(1000);
#else
				sleep(1);
#endif
			}
			else {
				break;
			}

			if(get_session_state(*s_id) == SExiting) {
				return -5;
			}
		}
	}

	if(a->cc_id == Null) {
		
		for(i = 0; (int)i < a->nb_channel; i++) {

			if(a->addr_type == 1) {
				*(ch_id + i) = add_alc_channel(*s_id, ports[i], addrs[0], a->intface, a->intface_name);
			}
			else {
				*(ch_id + i) = add_alc_channel(*s_id, ports[i], addrs[i], a->intface, a->intface_name);
			}

			if(*(ch_id + i) == -1) {

				while(i) {
					retcode = remove_alc_channel(*s_id, *(ch_id + i - 1));
					i--;
				}

				close_alc_session(*s_id);
				return -4;
			}
		}
	}
	else if(a->cc_id == RLC) {

		*ch_id = add_alc_channel(*s_id, ports[0], addrs[0], a->intface, a->intface_name);

		if(*ch_id == -1) {

			close_alc_session(*s_id);
			return -4;	
		}
	}

#if 0
	if(strcmp(a->file_path, "") == 0) {

		retval = receiver_in_fdt_based_mode(s_id, a);
	}
	else {
			
		if(a->file_list_mode) {

			if(strstr(a->file_path, "*") != NULL) {

				if(((a->file_path[0] == '*') && (a->file_path[(strlen(a->file_path) - 1)] == '*'))) {
					
					if(a->file_path[1] == '.') {
						printf("Only *something* or *.something valid values in wild card mode!\n");
						fflush(stdout);
						return -1;
					}

					memset(filetoken, 0, MAX_PATH);
					memcpy(filetoken, (a->file_path + 1), (strlen(a->file_path) - 2));
				}
				else if(((a->file_path[0] == '*') && (a->file_path[1] == '.'))) {
					
					memset(filetoken, 0, MAX_PATH);
					memcpy(filetoken, (a->file_path + 1), (strlen(a->file_path) - 1));
				}
				else {

					printf("Only *something* or *.something valid values in wild card mode!\n");
					fflush(stdout);
					return -1;
				}

			
				retval = receiver_in_wild_card_mode(s_id, a, filetoken);
			}
			else {
				retval = receiver_in_file_list_mode(s_id, a);
			}
		}
		else {
			
			retval = receiver_in_object_mode(s_id, a);
		}
	}		
#endif

	return retval;
}


/*
 * This function is flute sender's fdt based sending function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			int *ch_id: Pointer to channel identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 *
 * Return:	int: Different values (0, -1, -2) depending on how function ends.
 *
 */

int sender_in_fdt_based_mode(int *s_id, int *ch_id, arguments_t *a) {

#ifndef WIN32
	pthread_attr_t attr;
#endif

	int retval = 0;
	int retcode = 0;

	alc_session_t *s;

	time_t systime;
	bool is_printed = false;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	s = get_alc_session(*s_id);

	if (s->state == SExiting) return -2; /* FIXME */

#if 0
#ifdef USE_RLC
	if(((a->cc_id == RLC) || ((a->cc_id == Null) && (a->nb_channel != 1)))) {
#else
	if(((a->cc_id == Null) && (a->nb_channel != 1))) {
#endif
		/**** Start tx_thread ****/

#ifdef WIN32
		if(CreateThread(NULL, 0, (void*)tx_thread, (void*)s, 0, &s->tx_thread_id) == NULL) {
			perror("sender_in_fdt_based_mode: CreateThread");
			return -1;
		}	
#else
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 

		if (pthread_create(&s->tx_thread_id, &attr, tx_thread, (void*)s) != 0) {
			perror("sender_in_fdt_based_mode: pthread_create");
			return -1;
		}
#endif

	}
#endif

	if(a->starttime != 0) {
		while(1) {

			time(&systime);
			curr_time = systime + 2208988800;

			if(a->starttime > curr_time) {

				if(!is_printed) {
					printf("Waiting for session start time...\n");
					fflush(stdout);
					is_printed = true;
				}

#ifdef WIN32
				Sleep(1000);
#else
				sleep(1);
#endif
			}
			else {
				break;
			}

			if(s->state == SExiting) {
				return -2;
			}
		}
	}

	if(a->cont) { /* In continuous mode */
				
		while(1) {

			/*retcode = generate_fdt(a->file_path, a->base_dir, s_id, a->fdt_file);

			if(retcode < 0) {
				return -1;
			}*/

			if(a->cc_id == Null) {

				if(a->nb_channel == 1) {
					retcode = fdtbasedsend2(a->fdt_file, a->base_dir, a->file_path, s_id, ch_id, a->eslen,
											a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
											a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
										, a->encode_content, 0);
#else
										, 0);
#endif
				}
				else {
					retcode = fdtbasedsend3(a->fdt_file, a->base_dir, a->file_path, s_id, a->nb_channel, a->eslen,
										a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
										a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
										, a->encode_content);
#else
										);
#endif

				}
			}

#ifdef USE_RLC
			else if(a->cc_id == RLC) {
				retcode = fdtbasedsend3(a->fdt_file, a->base_dir, a->file_path, s_id, a->nb_channel, a->eslen,
										a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
										a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
										, a->encode_content);
#else
										);
#endif
			}
#endif

			if(retcode == -1) {
				printf("\nError: fdtbasedsend() failed\n");
				retval = -1;
				break;
			}
			else if(retcode == -2) {
				retval = -2;
				break;
			}
			else if(retcode == 866181) {
				retval = 866181;
				break;
			}

#ifdef USE_RLC
			if(((a->cc_id == RLC) || ((a->cc_id == Null) && (a->nb_channel != 1)))) {
#else
			if(((a->cc_id == Null) && (a->nb_channel != 1))) {
#endif
				while(s->tx_queue_begin != NULL) {

					if(s->state == SExiting) {
#ifdef USE_ZLIB
						if(a->encode_content) {
							remove_gz_files(a->fdt_file, a->base_dir, a->file_path);
						}
#endif
						return -2;
					}
#ifdef WIN32
					Sleep(1);
#else
					usleep(1000);
#endif
				}

				s->first_unit_in_loop = true;
			}

		}
	}
	else { /* fdt based send n times */
			
		while(a->nb_tx) {

			/*retcode = generate_fdt(a->file_path, s_id, a->fdt_file);

			if(retcode < 0) {
				return -1;
			}*/
				
			if(a->nb_tx == 1) {
				
				if(a->cc_id == Null) {

					if(a->nb_channel == 1) {
						retcode = fdtbasedsend2(a->fdt_file, a->base_dir, a->file_path, s_id, ch_id, a->eslen,
												a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
												a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
												, a->encode_content, 1);
#else
												, 1);
#endif
					}
					else {
						retcode = fdtbasedsend3(a->fdt_file, a->base_dir, a->file_path, s_id, a->nb_channel, a->eslen,
											a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
											a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
											, a->encode_content);
#else
											);
#endif
					}
				}
#ifdef USE_RLC
				else if(a->cc_id == RLC) {
					retcode = fdtbasedsend3(a->fdt_file, a->base_dir, a->file_path, s_id, a->nb_channel, a->eslen,
											a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
											a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
											, a->encode_content);
#else
											);
#endif
				}
#endif
			}
			else {
				
				if(a->cc_id == Null) {

					if(a->nb_channel == 1) {
						retcode = fdtbasedsend2(a->fdt_file, a->base_dir, a->file_path, s_id, ch_id, a->eslen,
												a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
												a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
												, a->encode_content, 0);
#else
												, 0);
#endif
					}
					else {
						retcode = fdtbasedsend3(a->fdt_file, a->base_dir, a->file_path, s_id, a->nb_channel, a->eslen,
											a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
											a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
											, a->encode_content);
#else
											);
#endif
					}
				}

#ifdef USE_RLC
				else if(a->cc_id == RLC) {
					retcode = fdtbasedsend3(a->fdt_file, a->base_dir, a->file_path, s_id, a->nb_channel, a->eslen,
											a->max_sblen, a->files_in_fdt_inst, a->fec_enc_id, a->fec_inst_id,
											a->use_fec_oti_ext_hdr
#ifdef USE_ZLIB
											, a->encode_content);
#else
											);
#endif
				}
#endif
			}
					
			if(retcode == -1) {
				printf("\nError: fdtbasedsend() failed\n");	
				retval = -1;
				break;
			}
			else if(retcode == -2) {
				retval = -2;
				break;
			}
			
			a->nb_tx--;

#ifdef USE_RLC
			if(((a->cc_id == RLC) || ((a->cc_id == Null) && (a->nb_channel != 1)))) {
#else
			if(((a->cc_id == Null) && (a->nb_channel != 1))) {
#endif
				while(s->tx_queue_begin != NULL) {

					if(s->state == SExiting) {
#ifdef USE_ZLIB
						if(a->encode_content) {
							remove_gz_files(a->fdt_file, a->base_dir, a->file_path);
						}
#endif
						return -2;
					}
#ifdef WIN32
					Sleep(1);
#else
					usleep(1000);
#endif			
				}
			}
		}
	}

#ifdef USE_ZLIB
	if(a->encode_content) {
		remove_gz_files(a->fdt_file, a->base_dir, a->file_path);
	}
#endif

	return retval;
}

/*
 * This function is flute sender's file sending function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			int *ch_id: Pointer to channel identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 *
 * Return:	int: Different values (0, -1, -2) depending on how function ends.
 *
 * Not used in flute anymore.
 */

int sender_in_file_mode(int *s_id, int *ch_id, arguments_t *a) {

	int i;
	int retval = 0;
	int retcode = 0;

	if(a->cont) { /* in continuous mode */
		while(1) { /* stopped by ctrl^c */
					
			for(i = 0; i < a->nb_channel; i++) {

				retcode = send_file(a->file_path, *s_id, *(ch_id + i), a->toi, a->eslen, a->max_sblen,
									a->fec_enc_id, a->fec_inst_id);
					
				if(retcode == -1) {
					printf("\nError: send_file() failed\n");
					retval = -1;
					break;
				}
				else if(retcode == -2) { /* ctrl^c pressed */
					retval = -2;
					break;
				}
			}
		}
	}
	else { /* in n times mode */
		while(a->nb_tx) {
					
			for(i = 0; i < a->nb_channel; i++) {

				if(a->nb_tx == 1) {
					set_session_a_flag(*s_id);
				}
			
				retcode = send_file(a->file_path, *s_id, *(ch_id + i), a->toi, a->eslen, a->max_sblen,
									a->fec_enc_id, a->fec_inst_id);
						
				if(retcode == -1) {
					printf("\nError: send_file() failed\n");
					retval = -1;
					break;
				}
				else if(retcode == -2) { /* ctrl^c pressed */
					retval = -2;
					break;
				}
			}
					
			a->nb_tx--;
		}
	}

	return retval;
}

/*
 * This function is flute receiver's fdt based receiving function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 * Return:	int: Different values (0, -1, -2, -3) depending on how function ends.
 *
 */

int receiver_in_fdt_based_mode(int *s_id, arguments_t *a) {

	int retval = 0;
	fdt_th_args_t fdt_th_args;

#ifdef WIN32
	unsigned long fdt_thread_id;
#else
	pthread_t fdt_thread_id;
#endif

	fdt_th_args.s_id = *s_id;
	fdt_th_args.fdt = NULL;
	fdt_th_args.rx_automatic = a->rx_automatic;
	fdt_th_args.filetoken = NULL;
	fdt_th_args.accept_expired_fdt_inst = a->accept_expired_fdt_inst;

	/* Create FDT receiving thread */

#ifdef WIN32

	if(CreateThread(NULL, 0, (void*)fdt_thread, (void*)&fdt_th_args, 0,
					&fdt_thread_id) == NULL) {

		printf("Error: receiver_in_fdt_based_mode, CreateThread\n");
		fflush(stdout);
		
		return -1;
	}
#else
	if((pthread_create(&fdt_thread_id, NULL, fdt_thread, (void*)&fdt_th_args)) != 0) {
		
		printf("Error: receiver_in_fdt_based_mode, pthread_create\n");
		fflush(stdout);
		
		return -1;
	}
#endif
	
	if(a->rx_automatic) {
		retval = receiver_in_automatic_mode(s_id, a, &fdt_th_args);
	}
	else {
		retval = receiver_in_ui_mode(s_id, a, &fdt_th_args);
	}

	if(fdt_th_args.fdt != NULL) {
		FreeFDT(fdt_th_args.fdt);
	}

	return retval;
}

/*
 * This function is flute receiver's fdt based automatic receiving function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed,
 *			fdt_th_args_t *fdt_th_args: Pointer to structure conmtaining FDT and some other information.
 *
 * Return:	int: Different values (0, -1, -2, -3) depending on how function ends.
 *
 */

int receiver_in_automatic_mode(int *s_id, arguments_t *a, fdt_th_args_t *fdt_th_args) {

	int retval = 0;
	int retcode = 0;
	char *cont_desc = NULL;

	if(strcmp(a->sdp_file, "") != 0) {
		cont_desc = sdp_attr_get(a->sdp, "content-desc");
	}

	printf("FLUTE Receiver in automatic mode\n\n");

	if(cont_desc != NULL) {
		printf("Session content information available at:\n");
		printf("%s\n\n", cont_desc);
	}

	fflush(stdout);

	while(fdt_th_args->fdt == NULL) {

		if(get_session_state(*s_id) == SExiting) {
			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			return -3;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
		continue;
	}

	if(a->big_file_mode) {
		retcode = fdtbasedrecv3(fdt_th_args->fdt, *s_id,
#ifdef WIN32
								a->openfile, a->accept_expired_fdt_inst);
#else 
								0, a->accept_expired_fdt_inst);
#endif

	}
	else {
		retcode = fdtbasedrecv2(fdt_th_args->fdt, *s_id,
#ifdef WIN32
								a->openfile, a->accept_expired_fdt_inst);
#else 
								0, a->accept_expired_fdt_inst);
#endif
	}	
				
	if(retcode == -1) {

		printf("Error: fdtbasedrecv() failed\n");
		fflush(stdout);
		retval = -1;
	}
	else if(retcode == -2) {
		retval = -2;
	}
	else if(retcode == -3) {
		retval = -3;
	}

	return retval;
}

/*
 * This function is flute receiver's fdt based ui mode receiving function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed,
 *			fdt_th_args_t *fdt_th_args: Pointer to structure conmtaining FDT and some other information.
 *
 * Return:	int: Different values (0, -1, -2, -3) depending on how function ends.
 *
 */

int receiver_in_ui_mode(int *s_id, arguments_t *a, fdt_th_args_t *fdt_th_args) {
	
	file_t *file;
	char input[100];

#ifdef WIN32
	ULONGLONG rec_toi;
#else
	unsigned long long rec_toi;
#endif

	char command;
	bool valid_toi = false;
	bool expired_toi = false;
	char *filepath = NULL;
	uri_t *uri = NULL;

	int retval = 0;
	int retcode = 0;

#ifdef USE_ZLIB
	int cont_enc_algo;
#endif

	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	char *cont_desc = NULL;

	if(strcmp(a->sdp_file, "") != 0) {
		cont_desc = sdp_attr_get(a->sdp, "content-desc");
	}

	printf("FLUTE Receiver in UI-mode\n\n");

	if(cont_desc != NULL) {
		printf("Session content information available at:\n");
		printf("%s\n\n", cont_desc);
	}

	fflush(stdout);

	while(fdt_th_args->fdt == NULL) {

		if(get_session_state(*s_id) == SExiting) {
			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			return -3;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
		continue;
	}

	while(1) {

		if(get_session_state(*s_id) == SExiting) {			
			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			return -3;
		}
	
		printf(">> Command: (D)ownload file (Q)uit\n");
		printf("-> ");

		if(fgets(input, 100, stdin) != NULL) {

			if(sscanf(input, "%c", &command) != EOF) {

				command = tolower(command);
							
				if(command == 'd') {
					printf("\nFiles to download:\n\n");
					fflush(stdout);
					
					file = fdt_th_args->fdt->file_list;

					while(file != NULL) {
#ifdef WIN32
						printf("\t%I64u) %s\n", file->toi, file->location);
#else
						printf("\t%llu) %s\n", file->toi, file->location);
#endif
						fflush(stdout);
						file = file->next;
					}

					printf("\nWhich one to download: ");
					fflush(stdout);

					if(fgets(input, 100, stdin) != NULL) {
						
#ifdef WIN32
						retcode = sscanf(input, "%I64u", &rec_toi);
#else
						retcode = sscanf(input, "%llu", &rec_toi);
#endif
						if(!((retcode == 0) || (retcode == EOF))) {
				
							file = fdt_th_args->fdt->file_list;

							valid_toi = false;
							expired_toi = false;
			
							while(file != NULL) {

								if(file->toi == rec_toi) {

									time(&systime);
									curr_time = systime + 2208988800;

									if(file->expires < curr_time) {
													
										if(a->accept_expired_fdt_inst == 0) {

											expired_toi = true;
#ifdef WIN32
											printf("\nToi: %I64u Expired.\n", rec_toi);        
#else
											printf("\nToi: %llu Expired.\n", rec_toi);  
#endif
											if(file->next != NULL) {
												file->next->prev = file->prev;
											}
											if(file->prev != NULL) {
												file->prev->next = file->next;
											}
											if(file == fdt_th_args->fdt->file_list) {
												fdt_th_args->fdt->file_list = file->next;
											}

											free_file(file);
											break;
										}
										else {
#ifdef WIN32
											printf("\nToi: %I64u Expired, receiving anyway.\n", rec_toi);        
#else
											printf("\nToi: %llu Expired, receiving anyway.\n", rec_toi);  
#endif											
										}
									}

									valid_toi = true;

									uri = parse_uri(file->location, strlen(file->location));
	
									filepath = get_uri_host_and_path(uri);

#ifdef USE_ZLIB																						
									if(file->encoding == NULL) {
										cont_enc_algo = 0;	
									}
									else {
										if(strcmp(file->encoding, "gzip") == 0) {
											cont_enc_algo = GZIP;	
										}
										else {
											cont_enc_algo = -1;
											break;
										}
									}

									retval = set_wanted_object(*s_id, file->toi, file->toi_len,
																   file->encoding_symbol_length,
																   file->max_source_block_length,
																   file->fec_instance_id,
																   file->fec_encoding_id,
																   file->max_nb_of_encoding_symbols,
																   (unsigned char)cont_enc_algo);
#else 
									retval = set_wanted_object(*s_id, file->toi, file->toi_len,
																   file->encoding_symbol_length,
																   file->max_source_block_length,
																   file->fec_instance_id,
																   file->fec_encoding_id,
																   file->max_nb_of_encoding_symbols);
#endif
									break;
								}
								file = file->next;
							}

							if(expired_toi) {
								free_uri(uri);
								free(filepath);
								continue;
                            }

							if(!valid_toi) {
#ifdef WIN32
								printf("\nToi: %I64u invalid.\n", rec_toi);        
#else
								printf("\nToi: %llu invalid.\n", rec_toi);  
#endif       
								fflush(stdout);
								continue;
                            }

#ifdef USE_ZLIB
							if(cont_enc_algo == -1) {
								printf("Content-Encoding: %s not supported\n", file->encoding);
								fflush(stdout);
								continue;
							}
#endif
							retcode = recvfile(*s_id, filepath, file->toi, file->md5, a->big_file_mode
#ifdef USE_ZLIB
											   , file->encoding, file->file_len
#endif
											   );
														
							if(retcode == -1) {
								free(filepath);
								free_uri(uri);
								printf("\nError: recvfile() failed\n");
								fflush(stdout);
								return -1;
							}
							else if(retcode == -2) {
								free(filepath);
								free_uri(uri);
								return -2;
							}
							else if(retcode == -3) {
								free(filepath);
								free_uri(uri);
								return -3;
							}

#ifdef WIN32
							if(((retcode != -4) && (a->openfile))) {
								ShellExecute(NULL, "Open", filepath, NULL, NULL, SW_SHOWNORMAL); 
							}
#endif
							free(filepath);
						}
					}
				}
				else if(command == 'q') {
					break;
				}
				else {
					continue;
				}
			}
		}	
	}

	return 0;
}

/*
 * This function is flute receiver's file list receiving function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 * Return:	int: Different values (0, -1, -2, -3) depending on how function ends.
 *
 */

int receiver_in_file_list_mode(int *s_id, arguments_t *a) {

	file_t *file;
	file_t *next_file;

	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	char *buf = NULL;

#ifdef WIN32
	ULONGLONG toi;
	ULONGLONG toilen;
#else
	unsigned long long toi;
	unsigned long long toilen;	
#endif

	int fd;

	char *ptr;
	int point; 
	int ch = '/';
	int i;

	char *tmp = NULL;
	char fullpath[MAX_PATH];
	char filename[MAX_PATH];
	char *filepath = NULL;
	
	bool file_to_receive = false;

	int retval = 0;
	int retcode = 0;

	char *filetoken = NULL;
	char *filetable[100];
	int file_nb = 0;
	int b;

	char *tmp_file_name = NULL;
	uri_t *uri = NULL;

#ifdef USE_ZLIB
	char *uncomprBuf = NULL;
	unsigned short cont_enc_algo;
	struct stat	file_stats;
#endif

#ifdef USE_OPENSSL
	char *md5 = NULL;
#endif

#ifdef WIN32
	unsigned long fdt_thread_id;
#else
	pthread_t fdt_thread_id;
#endif

	fdt_th_args_t fdt_th_args;

	char* session_basedir = get_session_basedir(*s_id);
	char *cont_desc = NULL;

	fdt_th_args.s_id = *s_id;
	fdt_th_args.fdt = NULL;
	fdt_th_args.rx_automatic = a->rx_automatic;
	fdt_th_args.filetoken = NULL;
	fdt_th_args.accept_expired_fdt_inst = a->accept_expired_fdt_inst;

	if(strcmp(a->sdp_file, "") != 0) {
		cont_desc = sdp_attr_get(a->sdp, "content-desc");
	}
	
	printf("FLUTE Receiver in file list mode\n\n");

	if(cont_desc != NULL) {
		printf("Session content information available at:\n");
		printf("%s\n\n", cont_desc);
	}

	/* Without TOI-number */

	/* check wanted file names */
	
	filetoken = strtok(a->file_path, ",");
				
	printf("Wanted file(s):");
	fflush(stdout);

	while(filetoken != NULL) {

		filetable[file_nb] = filetoken;
		printf(" %s", filetable[file_nb]);
		fflush(stdout);
					
		file_nb++;
		filetoken = strtok(NULL, ",");
	}

	printf("\n\n");
	fflush(stdout);

	/* Create FDT receiving thread */

#ifdef WIN32
	if(CreateThread(NULL, 0, (void*)fdt_thread, (void*)&fdt_th_args, 0, &fdt_thread_id) == NULL) {

		printf("Error: receiver_in_file_list_mode, CreateThread\n");
		fflush(stdout);
		return -1;
	}
#else
	if((pthread_create(&fdt_thread_id, NULL, fdt_thread, (void*)&fdt_th_args)) != 0) {
		
		printf("Error: receiver_in_jni_file_list_mode, phtread_create\n");
		fflush(stdout);
		return -1;
	}
#endif		

	while(fdt_th_args.fdt == NULL) {

		if(get_session_state(*s_id) == SExiting) {
			
#ifdef WIN32
			Sleep(500);
#else
			usleep(500000);
#endif

			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			
#ifdef WIN32
			Sleep(500);
#else
			usleep(500000);
#endif
			return -3;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
		continue;
	}

	/* receive wanted files */

	while(!all_files_received(filetable, file_nb)) {

		if(get_session_state(*s_id) == SExiting) {
			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			return -3;
		}

		file_to_receive = false;
					
		file = fdt_th_args.fdt->file_list;

		while(file != NULL) {

			for(b = 0; b < file_nb; b++) {

				if(filetable[b] == NULL) {
					continue;
				}
				else {

					if(strstr(file->location, filetable[b]) != NULL) {

						time(&systime);
						curr_time = systime + 2208988800;

						if(((file->expires < curr_time) && (a->accept_expired_fdt_inst == 0))) {
																	
							if(file->next != NULL) {
								file->next->prev = file->prev;
							}
							if(file->prev != NULL) {
								file->prev->next = file->next;
							}
							if(file == fdt_th_args.fdt->file_list) {
								fdt_th_args.fdt->file_list = file->next;
							}
												
							free_file(file);
							continue;
						}

						if(file->is_downloaded != 2) {

#ifdef USE_ZLIB
							if(file->encoding == NULL) {
								cont_enc_algo = 0;	
							}
							else {
								if(strcmp(file->encoding, "gzip") == 0) {
									cont_enc_algo = GZIP;	
								}
								else {
									printf("Content-Encoding: %s not supported\n", file->encoding);
									fflush(stdout);
									file = file->next;
									file->is_downloaded = 2;
									continue;
								}
							}

							file_to_receive = true;

							retval = set_wanted_object(*s_id, file->toi, file->toi_len,
														   file->encoding_symbol_length,
														   file->max_source_block_length,
														   file->fec_instance_id,
														   file->fec_encoding_id,
														   file->max_nb_of_encoding_symbols,
														   (unsigned char)cont_enc_algo);
#else
							file_to_receive = true;

							retval = set_wanted_object(*s_id, file->toi, file->toi_len,
														   file->encoding_symbol_length,
														   file->max_source_block_length,
														   file->fec_instance_id,
														   file->fec_encoding_id,
														   file->max_nb_of_encoding_symbols);
#endif

							if(retcode < 0) {
								set_session_state(*s_id, SExiting);
								return -1;
							}

							file->is_downloaded = 1;
						}	

						continue;
					}
								
				}
			}		
			file = file->next;
		}

		i = 0;
		toi = 0;
		toilen = 0;

		if(file_to_receive == false) {
			
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
			continue;
		}
		
		if(a->big_file_mode) {

			tmp_file_name = alc_recv3(*s_id, &toi, &retcode);

			if(tmp_file_name == NULL) {
				return retcode;
			}	
		}
		else {

			buf = alc_recv2(*s_id, &toi, &toilen, &retcode);

			if(buf == NULL) {
				return retcode;
			}
		}

		next_file = fdt_th_args.fdt->file_list;

		/* Find correct file structure, to get the file->location for file creation purpose */

		while(next_file != NULL) {
			file = next_file;

			if(file->toi == toi) {
				file->is_downloaded = 2;
				break;
			}

			next_file = file->next;
		}

		if(a->big_file_mode) {

#ifdef USE_OPENSSL
			md5 = file_md5(tmp_file_name);

			if(md5 == NULL) {
				printf("MD5 check failed!\n\n");
				fflush(stdout);
				remove(tmp_file_name);
				free(tmp_file_name);
				file->is_downloaded = 1;

#ifdef USE_ZLIB
				if(file->encoding == NULL) {

					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols,
									0);
				}
				else {

					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols,
									GZIP);
				}
#else 
				set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols);
#endif

				continue;
			}
			else{
				if(strcmp(md5, file->md5) != 0) {
					printf("MD5 check failed!\n\n");
					fflush(stdout);
					remove(tmp_file_name);
					free(tmp_file_name);
					free(md5);
					file->is_downloaded = 1;

#ifdef USE_ZLIB
					if(file->encoding == NULL) {

						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols,
										0);
					}
					else {

						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols,
										GZIP);
					}
#else 
					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols);
#endif

					continue;
				}

				free(md5);
			}
#endif

#ifdef USE_ZLIB

			if(file->encoding != NULL) {

				if(strcmp(file->encoding, "gzip") == 0) {
					
					retcode = file_gzip_uncompress(tmp_file_name);

					if(retcode == -1) {
						free(tmp_file_name);
						return -1;
					}

					*(tmp_file_name + (strlen(tmp_file_name) - GZ_SUFFIX_LEN)) = '\0';

					if(stat(tmp_file_name, &file_stats) == -1) {
						printf("Error: %s is not valid file name\n", tmp_file_name);
						fflush(stdout);
						free(tmp_file_name);
						return -1;
					}

					if(file_stats.st_size != file->file_len) {
						printf("Error: uncompression failed\n");
						fflush(stdout);
						remove(tmp_file_name);
						free(tmp_file_name);
						return -1;
					}
				}
			}
#endif
		}
		else {

#ifdef USE_OPENSSL
			md5 = buffer_md5(buf, toilen);

			if(md5 == NULL) {
				printf("MD5 check failed!\n\n");
				fflush(stdout);
				
#ifdef USE_ZLIB
				if(file->encoding != NULL) {
					if(strcmp(file->encoding, "gzip") == 0) {
						free(uncomprBuf);
					}
				}
				else {
					free(buf);
				}
#else
				free(buf);
#endif

				file->is_downloaded = 1;

#ifdef USE_ZLIB
				if(file->encoding == NULL) {

					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols,
									0);
				}
				else {

					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols, GZIP);
				}
#else 
				set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols);
#endif
				continue;
			}
			else{
				if(strcmp(md5, file->md5) != 0) {
					printf("MD5 check failed!\n\n");
					fflush(stdout);
					
#ifdef USE_ZLIB
					if(file->encoding != NULL) {
						if(strcmp(file->encoding, "gzip") == 0) {
							free(uncomprBuf);
						}
					}
					else {
						free(buf);
					}
#else
					free(buf);
#endif
					free(md5);
					file->is_downloaded = 1;

#ifdef USE_ZLIB
					if(file->encoding == NULL) {

						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols, 0);
					}
					else {

						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols, GZIP);
					}
#else 
					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols);
#endif
					continue;
				}

				free(md5);
			}

#endif

#ifdef USE_ZLIB
			if(file->encoding != NULL) {

				if(strcmp(file->encoding, "gzip") == 0) {
					
					uncomprBuf = buffer_gzip_uncompress(buf, toilen, file->file_len);

					if(uncomprBuf == NULL) {
						free(buf);
						return -1;
					}
				}

				free(buf);
			}
#endif
		}

		uri = parse_uri(file->location, strlen(file->location));
	
		filepath = get_uri_host_and_path(uri);
	
		if(!(tmp = (char*)calloc((strlen(filepath) + 1), sizeof(char)))) {
			printf("Could not alloc memory for tmp (file location)!\n");
			
			if(a->big_file_mode) {
				free(tmp_file_name);
            }
            else {

#ifdef USE_ZLIB
				if(file->encoding != NULL) {
					if(strcmp(file->encoding, "gzip") == 0) {
						free(uncomprBuf);
					}
				}
				else {
					free(buf);
				}
#else
				free(buf);
#endif
            }

			free(filepath);
			free_uri(uri);
			return -1;
		}

		memcpy(tmp, filepath, strlen(filepath));
														
		ptr = strchr(tmp, ch);

		memset(fullpath, 0, MAX_PATH);
		memcpy(fullpath, session_basedir, strlen(session_basedir));

		if(ptr != NULL) {
	
			while(ptr != NULL) {
				i++;

				point = (int)(ptr - tmp);

				memset(filename, 0, MAX_PATH);

#ifdef WIN32
				memcpy((fullpath + strlen(fullpath)), "\\", 1);
#else
				memcpy((fullpath + strlen(fullpath)), "/", 1);
#endif
				memcpy((fullpath + strlen(fullpath)), tmp, point);

				memcpy(filename, (tmp + point + 1), (strlen(tmp) - (point + 1)));

#ifdef WIN32
				if(mkdir(fullpath) < 0) {					
#else		
				if(mkdir(fullpath, S_IRWXU) < 0) {
#endif
					if(errno != EEXIST) {
						printf("mkdir failed: cannot create directory %s (errno=%i)\n", fullpath, errno);
						fflush(stdout);
						
						if(a->big_file_mode) {
							free(tmp_file_name);
                        }
                        else {

#ifdef USE_ZLIB
							if(file->encoding != NULL) {
								if(strcmp(file->encoding, "gzip") == 0) {
									free(uncomprBuf);
								}
							}
							else {
								free(buf);
							}
#else
							free(buf);
#endif
						}

						free(tmp);
						free(filepath);
						free_uri(uri);
						return -1;
					}
				}

				strcpy(tmp, filename);
				ptr = strchr(tmp, ch);
			}
#ifdef WIN32
			memcpy((fullpath + strlen(fullpath)), "\\", 1);
#else
			memcpy((fullpath + strlen(fullpath)), "/", 1);
#endif
			memcpy((fullpath + strlen(fullpath)), filename, strlen(filename));
		}
		else{

#ifdef WIN32
			memcpy((fullpath + strlen(fullpath)), "\\", 1);
#else
			memcpy((fullpath + strlen(fullpath)), "/", 1);
#endif
			memcpy((fullpath + strlen(fullpath)), filepath, strlen(filepath));
		}

		if(!a->big_file_mode) {

#ifdef WIN32
			if((fd = open((const char*)fullpath, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC , _S_IWRITE)) < 0) {
#else
			if((fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC , S_IRWXU)) < 0) {
#endif
				printf("Error: unable to open file %s\n", fullpath);
				fflush(stdout);
				
#ifdef USE_ZLIB
				if(file->encoding != NULL) {
					if(strcmp(file->encoding, "gzip") == 0) {
						free(uncomprBuf);
					}
				}
				else {
					free(buf);
				}
#else
				free(buf);
#endif

				free(tmp);
				free_uri(uri);
				free(filepath);
				return -1;
			}

#ifdef USE_ZLIB
			if(file->encoding != NULL) {
				if(strcmp(file->encoding, "gzip") == 0) {
					write(fd, uncomprBuf, (unsigned int)file->file_len);
					free(uncomprBuf);
				}
			}
			else {
				write(fd, buf, (unsigned int)toilen);
				free(buf);
			}
#else
			write(fd, buf, (unsigned int)toilen);
			free(buf);
#endif
			
			free(tmp);
			free_uri(uri);
			close(fd);
			
#ifdef WIN32
			printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
			printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif
			fflush(stdout);

#ifdef WIN32
			if(a->openfile) {
				ShellExecute(NULL, "Open", fullpath, NULL, NULL, SW_SHOWNORMAL); 
			}
#endif
			file_received(filetable, file_nb, filepath);
			free(filepath);
		}

		if(a->big_file_mode) {
		
			/* move and rename tmp_file */

			if(rename(tmp_file_name, fullpath) < 0) {

				if(errno == EEXIST) {
					retval = remove(fullpath);

					if(retval == -1) {
						printf("errno: %i\n", errno);
					}

					if(rename(tmp_file_name, fullpath) < 0) {
						printf("rename() error1\n");
					}
				}
				else {
					printf("rename() error2\n");
				}
			}

			free(tmp_file_name);
			free_uri(uri);
			free(tmp);

#ifdef WIN32
			printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
			printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif

			printf("Ollaan tassa!\n");

			fflush(stdout);

#ifdef WIN32
			if(a->openfile) {
				ShellExecute(NULL, "Open", fullpath, NULL, NULL, SW_SHOWNORMAL); 
			}
#endif
			file_received(filetable, file_nb, filepath);
			free(filepath);
		}
	}

	return retval;
}

/*
 * This function is flute receiver's object mode receiving function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 * Return:	int: Different values (0, -1, -2, -3) depending on how function ends.
 *
 */

int receiver_in_object_mode(int *s_id, arguments_t *a) {

	int retval = 0;
	int retcode;
	div_t div_max_n;
	unsigned short max_n;

	char *cont_desc = NULL;
	
	div_max_n = div((a->max_sblen * (100 + a->fec_ratio)), 100);
	max_n = (unsigned short)div_max_n.quot;

	if(strcmp(a->sdp_file, "") != 0) {
		cont_desc = sdp_attr_get(a->sdp, "content-desc");
	}

	printf("FLUTE Receiver in object mode\n\n");

	if(cont_desc != NULL) {
		printf("Session content information available at:\n");
		printf("%s\n\n", cont_desc);
	}

	fflush(stdout);

#ifdef USE_ZLIB
	set_wanted_object(*s_id, a->toi, 0, a->eslen, a->max_sblen, a->fec_inst_id, a->fec_enc_id, max_n, 0);
#else
	set_wanted_object(*s_id, a->toi, 0, a->eslen, a->max_sblen, a->fec_inst_id, a->fec_enc_id, max_n);
#endif

	retcode = recvfile(*s_id, a->file_path, a->toi, NULL, a->big_file_mode
#ifdef USE_ZLIB
					   , NULL, 0
#endif
					   );	

	if(retcode == -1) {
		printf("\nError: recvfile() failed\n");
		fflush(stdout);
		retval = -1;
	}
	else if(retcode == -2) {
		retval =  -2;
	}
	else if(retcode == -3) {
		retval =  -3;
	}
	else {
#ifdef WIN32
		if(a->openfile) {
			ShellExecute(NULL, "Open", a->file_path, NULL, NULL, SW_SHOWNORMAL); 
		}
#endif
	}

	return retval;
}

/*
 * This function is flute receiver's wild card receiving function.
 *
 * Params:	int *s_id: Pointer to session identifier,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed,
 *			char *filetoken: Pointer to string containing file ending.
 *
 * Return:	int: Different values (0, -1, -2, -3) depending on how function ends.
 *
 */

int receiver_in_wild_card_mode(int *s_id, arguments_t *a, char *filetoken) {

	file_t *file;
	file_t *next_file;

	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	char *buf = NULL;

#ifdef WIN32
	ULONGLONG toi;
	ULONGLONG toilen;
#else
	unsigned long long toi;
    unsigned long long toilen;
#endif

	int fd;

	char *ptr;
	int point; 
	int ch = '/';
	int i;

	char *tmp = NULL;
	char fullpath[MAX_PATH];
	char filename[MAX_PATH];
	char *filepath = NULL;
	
	bool file_to_receive = false;
	bool is_printed = true;

	int retval = 0;
	int retcode = 0;

	char *tmp_file_name = NULL;
	uri_t *uri = NULL;
	
#ifdef USE_ZLIB
	char *uncomprBuf = NULL;
	unsigned short cont_enc_algo;
	struct stat	file_stats;
#endif

#ifdef USE_OPENSSL
	char *md5 = NULL;
#endif

#ifdef WIN32
	unsigned long fdt_thread_id;
#else
	pthread_t fdt_thread_id;
#endif

	fdt_th_args_t fdt_th_args;

	char* session_basedir = get_session_basedir(*s_id);	
	char *cont_desc = NULL;
	
	fdt_th_args.s_id = *s_id;
	fdt_th_args.fdt = NULL;
	fdt_th_args.rx_automatic = a->rx_automatic;
	fdt_th_args.filetoken = filetoken;
	fdt_th_args.accept_expired_fdt_inst = a->accept_expired_fdt_inst;
	
	if(strcmp(a->sdp_file, "") != 0) {
		cont_desc = sdp_attr_get(a->sdp, "content-desc");
	}

	printf("FLUTE Receiver in wild card mode\n\n");

	if(cont_desc != NULL) {
		printf("Session content information available at:\n");
		printf("%s\n\n", cont_desc);
	}

	printf("Wanted files: %s\n", a->file_path);
	fflush(stdout);
			
	/* Create FDT receiving thread */

#ifdef WIN32
	if(CreateThread(NULL, 0, (void*)fdt_thread, (void*)&fdt_th_args, 0, &fdt_thread_id) == NULL) {

		printf("Error: receiver_in_wild_card_mode, CreateThread\n");
		fflush(stdout);
		return -1;
	}
#else
	if((pthread_create(&fdt_thread_id, NULL, fdt_thread, (void*)&fdt_th_args)) != 0) {
		
		printf("Error: receiver_in_jni_wild_card_mode, phtread_create\n");
		fflush(stdout);
		return -1;
	}
#endif		

	while(fdt_th_args.fdt == NULL) {

		if(get_session_state(*s_id) == SExiting) {
			
#ifdef WIN32
			Sleep(500);
#else
			usleep(500000);
#endif
			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			
#ifdef WIN32
			Sleep(500);
#else
			usleep(500000);
#endif
			return -3;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
		continue;
	}

	/* receive wanted files */

	while(1) {

		if(get_session_state(*s_id) == SExiting) {
			return -2;
		}
		else if(get_session_state(*s_id) == STxStopped) {
			return -3;
		}

		file_to_receive = false;
					
		file = fdt_th_args.fdt->file_list;

		while(file != NULL) {

			if(strstr(file->location, filetoken) != NULL) {

				time(&systime);
				curr_time = systime + 2208988800;

				if(((file->expires < curr_time) && (a->accept_expired_fdt_inst == 0))) {
															
					if(file->next != NULL) {
						file->next->prev = file->prev;
					}
					if(file->prev != NULL) {
						file->prev->next = file->next;
					}
					if(file == fdt_th_args.fdt->file_list) {
						fdt_th_args.fdt->file_list = file->next;
					}
										
					free_file(file);
					continue;
				}

				if(file->is_downloaded != 2) {

#ifdef USE_ZLIB
					if(file->encoding == NULL) {
						cont_enc_algo = 0;	
					}
					else {
						if(strcmp(file->encoding, "gzip") == 0) {
							cont_enc_algo = GZIP;	
						}
						else {
							printf("Content-Encoding: %s not supported\n", file->encoding);
							fflush(stdout);
							file->is_downloaded = 2;
							file = file->next;
							continue;
						}
					}

					file_to_receive = true;
					is_printed = false;
#else
					file_to_receive = true;
					is_printed = false;
#endif
					if(retcode < 0) {
						set_session_state(*s_id, SExiting);
						return -1;
					}				
      			}
			}
  
      		file = file->next;
		}
    
		i = 0;
		toi = 0;
		toilen = 0;

		if(file_to_receive == false) {
  
      		if(!is_printed) {
				printf("All files received, waiting for new files\n\n");
				fflush(stdout);
				is_printed = true;
      		}
  
#ifdef WIN32
      		Sleep(1);
#else
      		usleep(1000);
#endif
      		continue;
    	}
		
		if(a->big_file_mode) {

			tmp_file_name = alc_recv3(*s_id, &toi, &retcode);

			if(tmp_file_name == NULL) {
				return retcode;
			}	
		}
		else {

			buf = alc_recv2(*s_id, &toi, &toilen, &retcode);

			if(buf == NULL) {
				return retcode;
			}
		}

		next_file = fdt_th_args.fdt->file_list;

		/* Find correct file structure, to get the file->location for file creation purpose */

		while(next_file != NULL) {
			file = next_file;

			if(file->toi == toi) {
				file->is_downloaded = 2;
				break;
			}

			next_file = file->next;
		}

		if(a->big_file_mode) {

#ifdef USE_OPENSSL
			md5 = file_md5(tmp_file_name);

			if(md5 == NULL) {
				printf("MD5 check failed!\n\n");
				fflush(stdout);
				remove(tmp_file_name);
				free(tmp_file_name);
				file->is_downloaded = 1;

#ifdef USE_ZLIB
				if(file->encoding == NULL) {

					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									  file->max_source_block_length,
									  file->fec_instance_id,
									  file->fec_encoding_id,
									  file->max_nb_of_encoding_symbols, 0);
				}
				else {

					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									  file->max_source_block_length,
									  file->fec_instance_id, file->fec_encoding_id,
									  file->max_nb_of_encoding_symbols, GZIP);
				}
#else 
				set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
								  file->max_source_block_length,
								  file->fec_instance_id,
								  file->fec_encoding_id,
								  file->max_nb_of_encoding_symbols);
#endif
				continue;
			}
			else{
				if(strcmp(md5, file->md5) != 0) {
					printf("MD5 check failed!\n\n");
					fflush(stdout);
					remove(tmp_file_name);
					free(tmp_file_name);
					free(md5);
					file->is_downloaded = 1;
#ifdef USE_ZLIB
					if(file->encoding == NULL) {

						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id, file->fec_encoding_id,
										file->max_nb_of_encoding_symbols, 0);
					}
					else {

						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id, file->fec_encoding_id,
										file->max_nb_of_encoding_symbols, GZIP);
					}
#else 
					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										file->max_source_block_length,
										file->fec_instance_id, file->fec_encoding_id,
										file->max_nb_of_encoding_symbols);
#endif
					continue;
				}

				free(md5);
			}
#endif

#ifdef USE_ZLIB

			if(file->encoding != NULL) {

				if(strcmp(file->encoding, "gzip") == 0) {
					
					retcode = file_gzip_uncompress(tmp_file_name);

					if(retcode == -1) {
						free(tmp_file_name);
						return -1;
					}

					*(tmp_file_name + (strlen(tmp_file_name) - GZ_SUFFIX_LEN)) = '\0';

					if(stat(tmp_file_name, &file_stats) == -1) {
						printf("Error: %s is not valid file name\n", tmp_file_name);
						fflush(stdout);
						free(tmp_file_name);
						return -1;
					}

					if(file_stats.st_size != file->file_len) {
						printf("Error: uncompression failed\n");
						fflush(stdout);
						remove(tmp_file_name);
						free(tmp_file_name);
						return -1;
					}
				}
			}
#endif
		}
		else {

#ifdef USE_OPENSSL
			md5 = buffer_md5(buf, toilen);

			if(md5 == NULL) {
				printf("MD5 check failed!\n\n");
				fflush(stdout);

#ifdef USE_ZLIB
				if(file->encoding != NULL) {
					if(strcmp(file->encoding, "gzip") == 0) {
						free(uncomprBuf);
					}
				}
				else {
					free(buf);
				}
#else
				free(buf);
#endif

				file->is_downloaded = 1;

#ifdef USE_ZLIB
				if(file->encoding == NULL) {
					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									 file->fec_instance_id,
									 file->fec_encoding_id,
								     file->max_nb_of_encoding_symbols, 0);
				}
				else {
					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									 file->fec_instance_id, file->fec_encoding_id,
								     file->max_nb_of_encoding_symbols, GZIP);
				}
#else
				set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									 file->fec_instance_id,
									 file->fec_encoding_id,
									 file->max_nb_of_encoding_symbols);
#endif
				continue;
			}
			else{
				if(strcmp(md5, file->md5) != 0) {
					printf("MD5 check failed!\n\n");
					fflush(stdout);
				
#ifdef USE_ZLIB
					if(file->encoding != NULL) {
						if(strcmp(file->encoding, "gzip") == 0) {
							free(uncomprBuf);
						}
					}
					else {
						free(buf);
					}
#else
					free(buf);
#endif

					free(md5);
					file->is_downloaded = 1;

#ifdef USE_ZLIB
					if(file->encoding == NULL) {
						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										 file->max_source_block_length,
										 file->fec_instance_id, file->fec_encoding_id,
										 file->max_nb_of_encoding_symbols, 0);
					}
					else {
						set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										 file->max_source_block_length,
										 file->fec_instance_id, file->fec_encoding_id,
										 file->max_nb_of_encoding_symbols, GZIP);
					}
#else
					set_wanted_object(*s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										 file->max_source_block_length,
										 file->fec_instance_id,
										 file->fec_encoding_id,
										 file->max_nb_of_encoding_symbols);
#endif
					continue;
				}

				free(md5);
			}
#endif

#ifdef USE_ZLIB
			if(file->encoding != NULL) {

				if(strcmp(file->encoding, "gzip") == 0) {
					
					uncomprBuf = buffer_gzip_uncompress(buf, toilen, file->file_len);

					if(uncomprBuf == NULL) {
						free(buf);
						return -1;
					}
				}

				free(buf);
			}
#endif
		}

		uri = parse_uri(file->location, strlen(file->location));
	
		filepath = get_uri_host_and_path(uri);
	
		if(!(tmp = (char*)calloc((strlen(filepath) + 1), sizeof(char)))) {
			printf("Could not alloc memory for tmp (file location)!\n");
			
			if(a->big_file_mode) {
				free(tmp_file_name);
            }
            else {

#ifdef USE_ZLIB
				if(file->encoding != NULL) {
					if(strcmp(file->encoding, "gzip") == 0) {
						free(uncomprBuf);
					}
				}
				else {
					free(buf);
				}
#else
				free(buf);
#endif
            }

			free(filepath);
			free_uri(uri);
			return -1;
		}

		memcpy(tmp, filepath, strlen(filepath));
														
		ptr = strchr(tmp, ch);

		memset(fullpath, 0, MAX_PATH);
		memcpy(fullpath, session_basedir, strlen(session_basedir));

		if(ptr != NULL) {
	
			while(ptr != NULL) {
				i++;

				point = (int)(ptr - tmp);

				memset(filename, 0, MAX_PATH);

#ifdef WIN32
				memcpy((fullpath + strlen(fullpath)), "\\", 1);
#else
				memcpy((fullpath + strlen(fullpath)), "/", 1);
#endif
				memcpy((fullpath + strlen(fullpath)), tmp, point);

				memcpy(filename, (tmp + point + 1), (strlen(tmp) - (point + 1)));

#ifdef WIN32
				if(mkdir(fullpath) < 0) {					
#else		
				if(mkdir(fullpath, S_IRWXU) < 0) {
#endif
					if(errno != EEXIST) {
						printf("mkdir failed: cannot create directory %s (errno=%i)\n", a->base_dir, errno);
						fflush(stdout);
			
						if(a->big_file_mode) {
							free(tmp_file_name);
                        }
                        else {

#ifdef USE_ZLIB
							if(file->encoding != NULL) {
								if(strcmp(file->encoding, "gzip") == 0) {
									free(uncomprBuf);
								}
							}
							else {
								free(buf);
							}
#else
							free(buf);
#endif
						}

						free(tmp);
						free(filepath);
						free_uri(uri);
						return -1;
					}
				}

				strcpy(tmp, filename);
				ptr = strchr(tmp, ch);
			}
#ifdef WIN32
			memcpy((fullpath + strlen(fullpath)), "\\", 1);
#else
			memcpy((fullpath + strlen(fullpath)), "/", 1);
#endif
			memcpy((fullpath + strlen(fullpath)), filename, strlen(filename));
		}
		else{

#ifdef WIN32
			memcpy((fullpath + strlen(fullpath)), "\\", 1);
#else
			memcpy((fullpath + strlen(fullpath)), "/", 1);
#endif
			memcpy((fullpath + strlen(fullpath)), filepath, strlen(filepath));
		}

		if(!a->big_file_mode) {

#ifdef WIN32
			if((fd = open((const char*)fullpath, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC , _S_IWRITE)) < 0) {
#else
			if((fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC , S_IRWXU)) < 0) {
#endif
				printf("Error: unable to open file %s\n", fullpath);
				fflush(stdout);

#ifdef USE_ZLIB
				if(file->encoding != NULL) {
					if(strcmp(file->encoding, "gzip") == 0) {
						free(uncomprBuf);
					}
				}
				else {
					free(buf);
				}
#else
				free(buf);
#endif
				free(tmp);
				free_uri(uri);
				free(filepath);
				return -1;
			}

#ifdef USE_ZLIB
			if(file->encoding != NULL) {
				if(strcmp(file->encoding, "gzip") == 0) {
					write(fd, uncomprBuf, (int)file->file_len);
					free(uncomprBuf);
				}
			}
			else {
				write(fd, buf, (unsigned int)toilen);
				free(buf);
			}
#else
			write(fd, buf, (unsigned int)toilen);
			free(buf);
#endif

			free(tmp);
			free_uri(uri);
			close(fd);

#ifdef WIN32	
			printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
			printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif
			fflush(stdout);

#ifdef WIN32
			if(a->openfile) {
				ShellExecute(NULL, "Open", fullpath, NULL, NULL, SW_SHOWNORMAL); 
			}
#endif
			free(filepath);
		}

		if(a->big_file_mode) {
		
			/* move and rename tmp_file */

			if(rename(tmp_file_name, fullpath) < 0) {

				if(errno == EEXIST) {
					retval = remove(fullpath);

					if(retval == -1) {
						printf("errno: %i\n", errno);
					}

					if(rename(tmp_file_name, fullpath) < 0) {
						printf("rename() error1\n");
					}
				}
				else {
					printf("rename() error2\n");
				}
			}

			free(tmp_file_name);
			free_uri(uri);
			free(tmp);
#ifdef WIN32
			printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
			printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif
			fflush(stdout);

#ifdef WIN32
			if(a->openfile) {
				ShellExecute(NULL, "Open", fullpath, NULL, NULL, SW_SHOWNORMAL); 
			}
#endif
			free(filepath);
		}
	}

	return retval;
}


/*
 * This function checks if all wanted files are received.
 *
 * Params:      char *filetable[10]: Pointers to wanted file names,
 *              int file_nb: Number of wanted files.
 *
 * Return:      int: 1 when all files are received, 0 otherwise
 *
 */

int all_files_received(char *filetable[10], int file_nb) {

        int i;
        int retval = 1;

        for(i = 0; i < file_nb; i++) {
                if(filetable[i] != NULL) {
                        retval = 0;
                }
        }

        return retval;
}

/*
 * This function sets file defined as received.
 * 
 * Params:      char *filetable[10]: Pointers to wanted file names,
 *				int file_nb: Number of wanted files,
 *				char *filepath: Pointer to file's path
 *
 * Return:      void
 *
 */

void file_received(char *filetable[10], int file_nb, char *filepath) {

    int i;

    for(i = 0; i < file_nb; i++) {

		if(filetable[i] != NULL) {

			if(strstr(filepath, filetable[i]) != NULL) {
				filetable[i] = NULL;
				break;
			}
		}
    }
}

#ifdef USE_SDPLIB
/*
 * This function parse session information from SDP file.
 *
 * Params:	arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 * char addrs[MAX_CHANNELS_IN_SESSION][MAX_LENGTH]:,
 * char ports[MAX_CHANNELS_IN_SESSION][MAX_LENGTH]:.
 *
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int parse_sdp_file(arguments_t *a, char addrs[MAX_CHANNELS_IN_SESSION][MAX_LENGTH],
				   char ports[MAX_CHANNELS_IN_SESSION][MAX_LENGTH]) {

	char *buf = NULL;
	FILE *sdp_fp;
	struct stat	sdp_file_stats;
	int nbytes;
	int i;
	int flute_ch_number;
	int m_lines = 0;
	int j = 0;
	int number_of_port;
	int port;
	int number_of_address;
	char *address;
	int nb_of_accepted_ch = 0;
	int nb_of_defined_ch = 0;
	struct sockaddr_in ipv4;
	unsigned long addr_nb;
	unsigned short ipv6addr[8];
	int nb_ipv6_part;
	int dup_sep = 0;
	char tmp[5];
	int k, l, m;
	int retcode = 0;
	int position = 0;

	char *start_time = NULL;
	char *stop_time = NULL;
	int errno;

#ifndef WIN32
	char *ep;
#endif

	fec_dec_t *fec_dec;
	fec_dec_t *current_fec_dec;
	int m_line_att_pos;
	char *att_name;
	char *att_value;
	bool supported_fec = false;
	bool fec_inst_exists = false;

	if(stat(a->sdp_file, &sdp_file_stats) == -1) {
		printf("Error: %s is not valid file name\n", a->sdp_file);
		fflush(stdout);
		return -1;
	}
	
	/* Allocate memory for buf */
	if(!(buf = (char*)calloc((sdp_file_stats.st_size + 1), sizeof(char)))) {
		printf("Could not alloc memory for sdp buffer!\n");
		fflush(stdout);
		return -1;
	}

	if((sdp_fp = fopen(a->sdp_file, "rb")) == NULL) {
		printf("Error: unable to open sdp_file %s\n", a->sdp_file);
		fflush(stdout);
		free(buf);
		return -1;
	}
	
	sdp_init(&a->sdp);

	nbytes = fread(buf, 1, sdp_file_stats.st_size, sdp_fp); 

	if(sdp_parse(a->sdp, buf) != 0) {
		printf("Error: sdp_parse()\n");
		fflush(stdout);
		free(buf);
		sdp_free(a->sdp);
		return -1;
	}

	a->src_filt = sf_char2struct(sdp_attr_get(a->sdp, "source-filter"));
	a->srcaddr = a->src_filt->src_addr;
	a->tsi = (unsigned int)atoi(sdp_attr_get(a->sdp, "flute-tsi"));

	/* Default channel number is one and it is overwrited if there is
	   'a:flute-ch' in the SDP file. */

	flute_ch_number = 1;
	
	if(sdp_attr_get(a->sdp, "flute-ch") != NULL) {
		flute_ch_number = (unsigned int)atoi(sdp_attr_get(a->sdp, "flute-ch"));
	}

	if(strcmp(sdp_c_addrtype_get(a->sdp, 0, 0), "IP4") == 0) {
		a->addr_family = PF_INET;
	}
	else if(strcmp(sdp_c_addrtype_get(a->sdp, 0, 0), "IP6") == 0) {
		a->addr_family = PF_INET6;
	}

	/* fetch session starttime */

	start_time = sdp_t_start_time_get (a->sdp, 0);

	if(start_time != NULL) {
#ifdef WIN32			  
		a->starttime = _atoi64(start_time);

		if(a->starttime > 0xFFFFFFFFFFFFFFFF) {
			printf("Error: Invalid SDP, starttime too big.\n");
			fflush(stdout);
			free(buf);
			sdp_free(a->sdp);
			return -1;
		}
#else				
		a->starttime = strtoull(start_time, &ep, 10);

        if(errno == ERANGE && a->starttime == 0xFFFFFFFFFFFFFFFF) {
            printf("Error: Invalid SDP, starttime too big.\n");
			fflush(stdout);
			free(buf);
			sdp_free(a->sdp);
			return -1;
        }
#endif	
	}
	
	/* fetch session stoptime */

	stop_time = sdp_t_stop_time_get (a->sdp, 0);

	if(stop_time != NULL) {
#ifdef WIN32			  
		a->stoptime = _atoi64(stop_time);

		if(a->stoptime > 0xFFFFFFFFFFFFFFFF) {
			printf("Error: Invalid SDP, stoptime too big.\n");
			fflush(stdout);
			free(buf);
			sdp_free(a->sdp);
			return -1;
		}
#else				
		a->stoptime = strtoull(stop_time, &ep, 10);

        if(errno == ERANGE && a->stoptime == 0xFFFFFFFFFFFFFFFF) {
            printf("Error: Invalid SDP, stoptime too big.\n");
			fflush(stdout);
			free(buf);
			sdp_free(a->sdp);
			return -1;
        }
#endif
		if(a->stoptime == 0) {
			a->cont = 1;
		}
	}
	
	/* Session level FEC declaration */
	fec_dec = sdp_fec_dec_get(a->sdp);

	/* Search how many m-lines is defined in SDP */

	while(1) {
		if(sdp_endof_media(a->sdp, position) == 0) {

			/* check that 'proto' field is FLUTE/UDP */
			if(strcmp(sdp_m_proto_get(a->sdp, position), "FLUTE/UDP") == 0) {

				/* check that payload number exists */
				if(sdp_m_payload_get(a->sdp, position, 0) != NULL) {
					m_lines++;
				}
			}

			position++;
		}
		else {
			break;
		}
	}

	if(m_lines == 0) {
		printf("Error: Invalid SDP, no valid 'm' field.\n");
		fflush(stdout);
		free(buf);
		sdp_free(a->sdp);
		return -1;
	}

	for(i = 0; i < m_lines; i++) {

		m_line_att_pos = 0;

		while((att_name = sdp_a_att_field_get(a->sdp, i, m_line_att_pos)) != NULL) {
			if(strcmp(att_name, "FEC") == 0) {
				fec_inst_exists = true;
				att_value = sdp_a_att_value_get(a->sdp, i, m_line_att_pos);
				
				if(fec_dec == NULL) {
					printf("Error: Invalid SDP, FEC-declaration does not exists.\n");
					fflush(stdout);
					free(buf);
					sdp_free(a->sdp);
					return -1;
				}

				current_fec_dec = fec_dec;

				while(current_fec_dec != NULL) {

					if(current_fec_dec->index == atoi(att_value)) {
							
#ifdef USE_NULL_FEC
						if(current_fec_dec->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
							a->fec_enc_id = COM_NO_C_FEC_ENC_ID;
							a->fec_inst_id = 0;
							supported_fec = true;
						}
#endif

#ifdef USE_SIMPLE_XOR
						if(current_fec_dec->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
							a->fec_enc_id = SIMPLE_XOR_FEC_ENC_ID;
							a->fec_inst_id = 0;
							supported_fec = true;
						}
#endif

#ifdef USE_REED_SOLOMON
						if(current_fec_dec->fec_enc_id	== SB_SYS_FEC_ENC_ID &&
							current_fec_dec->fec_inst_id == REED_SOL_FEC_INST_ID) {
							a->fec_enc_id = SB_SYS_FEC_ENC_ID;
							a->fec_inst_id = REED_SOL_FEC_INST_ID;
							supported_fec = true;
						}
#endif
					}
					current_fec_dec = current_fec_dec->next;
				}
			}
			else if(strcmp(att_name, "FEC-declaration") == 0) {
				fec_inst_exists = true;
				att_value = sdp_a_att_value_get(a->sdp, i, m_line_att_pos);
	
				current_fec_dec = fec_dec_char2struct(att_value);
			
#ifdef USE_NULL_FEC
				if(current_fec_dec->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
					a->fec_enc_id = COM_NO_C_FEC_ENC_ID;
					a->fec_inst_id = 0;
					supported_fec = true;
				}
#endif

#ifdef USE_SIMPLE_XOR
				if(current_fec_dec->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
					a->fec_enc_id = SIMPLE_XOR_FEC_ENC_ID;
					a->fec_inst_id = 0;
					supported_fec = true;
				}
#endif

#ifdef USE_REED_SOLOMON
				if(current_fec_dec->fec_enc_id	== SB_SYS_FEC_ENC_ID &&
					current_fec_dec->fec_inst_id == REED_SOL_FEC_INST_ID) {
					a->fec_enc_id = SB_SYS_FEC_ENC_ID;
					a->fec_inst_id = REED_SOL_FEC_INST_ID;
					supported_fec = true;
				}
#endif
			}
			m_line_att_pos++;
		}

		if(fec_inst_exists == false) {
			supported_fec = true;
			a->fec_enc_id = COM_NO_C_FEC_ENC_ID;
			a->fec_inst_id = 0;
		}

		/* how many ports in m-line */

		if(sdp_m_number_of_port_get(a->sdp, i) == NULL) {
			number_of_port = 1;
		}
		else {
			number_of_port = atoi(sdp_m_number_of_port_get(a->sdp, i));
		}
		
		/* how many addresses in c-line */

		if(sdp_c_addr_multicast_int_get(a->sdp, i, 0) == NULL) {
			number_of_address = 1;
		}
		else {
			number_of_address = atoi(sdp_c_addr_multicast_int_get(a->sdp, i, 0));
		}

		if(((number_of_port != 1) && (number_of_address != 1))) {
			printf("Error: Invalid SDP, confusing number of ports and addresses.\n");
			fflush(stdout);
			free(buf);
			sdp_free(a->sdp);
			return -1;
		}
	
		if(number_of_address == 1) {
		
			/* base port number */
			port = atoi(sdp_m_port_get(a->sdp, i));

			for(j = 0; j < number_of_port; j++) {

				if(supported_fec == true) {
					memset(ports[nb_of_accepted_ch], 0, MAX_LENGTH);
					memset(addrs[nb_of_accepted_ch], 0, MAX_LENGTH);

					sprintf(ports[nb_of_accepted_ch], "%i", (port + j));
					strcpy(addrs[nb_of_accepted_ch], sdp_c_addr_get(a->sdp, i, 0));
					nb_of_accepted_ch++;
				}
				nb_of_defined_ch++;
			}
		}
		else if(number_of_port == 1) {
		
			/* base address */
			address = sdp_c_addr_get(a->sdp, i, 0);

			if(a->addr_family == PF_INET) {
				/* IPv4 address */
				addr_nb = ntohl(inet_addr(address));
			}
			else if(a->addr_family == PF_INET6) {
				/*IPv6 address */
				retcode = ushort_ipv6addr(address, &ipv6addr[0], &nb_ipv6_part);

				if(retcode == -1) {
					free(buf);
					sdp_free(a->sdp);
					return -1;				
				}
			}

			for(j = 0; j < number_of_address; j++) {
				if(supported_fec == true) {
					memset(ports[nb_of_accepted_ch], 0, MAX_LENGTH);
					memset(addrs[nb_of_accepted_ch], 0, MAX_LENGTH);

					strcpy(ports[nb_of_accepted_ch], sdp_m_port_get(a->sdp, i));
					
					if(a->addr_family == PF_INET) {
						ipv4.sin_addr.s_addr = htonl(addr_nb + j);
						sprintf(addrs[nb_of_accepted_ch], "%s", inet_ntoa(ipv4.sin_addr));
					}
					else if(a->addr_family == PF_INET6) {
						dup_sep = 0;

						for(k = 0; k < nb_ipv6_part + 1; k++) {

							memset(tmp, 0, 5);

							if(ipv6addr[k] != 0) {
								sprintf(tmp, "%x", ipv6addr[k]);
								strcat(addrs[nb_of_accepted_ch], tmp);
							}
							else {
								if(dup_sep == 0) {
									dup_sep = 1;
								}
								else {
									printf("Invalid IPv6 address!\n");
									fflush(stdout);
									free(buf);
									sdp_free(a->sdp);
									return -1;	
								}	
							}

							if(k != nb_ipv6_part) {
								strcat(addrs[nb_of_accepted_ch], ":");
							}
						}

						for(l = nb_ipv6_part;; l--) {

							if(l == 0) {
								printf("Only %i channel possible!\n", (j + 1));
								fflush(stdout);
								free(buf);
								sdp_free(a->sdp);
								return -1;	
							}

							if(ipv6addr[l] + 1 > 0xFFFF) {
								continue;
							}
							
							if(ipv6addr[l] == 0) {

								if(nb_ipv6_part == 7) {
									ipv6addr[l]++;
									break;
								}

								for(m = nb_ipv6_part; m > l; m--) {
									ipv6addr[m + 1] = ipv6addr[m];	
								}

								ipv6addr[l + 1] = 1;

								nb_ipv6_part++;
								break;
							}
							else {
								ipv6addr[l]++;
								break;
							}
						}
					}
				}
				nb_of_defined_ch++;
			}
		}
	}

	if(flute_ch_number != nb_of_defined_ch) {
		printf("Error: Invalid SDP, channel number not correct.\n");
		fflush(stdout);
		free(buf);
		sdp_free(a->sdp);
		fec_dec_free(fec_dec);
		return -1;
	}

	a->nb_channel = nb_of_accepted_ch;

	free(buf);
	fclose(sdp_fp);
	fec_dec_free(fec_dec);

	return 0;
}
#endif
