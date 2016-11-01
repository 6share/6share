/* $Author: peltotal $ $Date: 2004/12/28 10:16:12 $ $Revision: 1.72 $ */
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
#include "fdt.h"

/* Global variables */

int s_id;

/**** Private functions ****/

void signal_handler(int sig);
void usage(void);
void validateargs(int argc, char **argv, arguments_t *a);

/*
 * This function is programs main function.
 *
 * Params:	int argc: Number of command line arguments,
 *			char **argv: Pointer to command line arguments.
 *
 * Return: int: Different values (0, -1, -2, -3) depending on how program ends.
 *
 */

int main(int argc, char **argv) {

	int i;
	int retval = 0;
	int retcode = 0;
	
#ifdef WIN32
	WSADATA wsd;
#endif

	arguments_t a;

	int ch_id[MAX_CHANNELS_IN_SESSION];           /* Channel identifiers */

	/* Set signal handler */

	signal(SIGINT, signal_handler);
#ifdef WIN32
	signal(SIGBREAK, signal_handler);
#endif

	validateargs(argc, argv, &a);

	s_id = -1; 

#ifdef WIN32

	if(WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup() failed\n");

		if(a.logfd != -1) {
			close(a.logfd);
		}

        return -1;
    }
#endif

	memset(ch_id, -1, MAX_CHANNELS_IN_SESSION);

	if(a.mode == SENDER) {
		
		retval = flute_sender(&a, &s_id, &ch_id[0]);

		if(s_id == -1) {

			if(a.logfd != -1) {
				close(a.logfd);
			}

#ifdef WIN32
			WSACleanup();
#endif
			return retval;
		}

		if(retval == -4) { /* bind failed */

			if(a.logfd != -1) {
				close(a.logfd);
			}

#ifdef WIN32
			WSACleanup();
#endif
			return retval;	
		}
	
		if(retval != -1) {
			printf("\nSending session close packets\n");
			fflush(stdout);
		}

		while(a.nb_channel) {

			if(retval != -1) {

				if(a.cc_id == Null) {

					for(i = 0; i < 3; i++) {
						retcode = send_session_close_packet(s_id, ch_id[a.nb_channel - 1]);
						
						if(retcode == -1) {
							break;
						}
					}
#ifdef WIN32
					Sleep(500);
#else
					usleep(500000);
#endif
				}

#ifdef USE_RLC
				else if(a.cc_id == RLC) {

					if(a.nb_channel - 1 == 0) {
				
						for(i = 0; i < 3; i++) {
							retcode = send_session_close_packet(s_id, ch_id[a.nb_channel - 1]);

							if(retcode == -1) {
								break;
							}
						}
#ifdef WIN32
						Sleep(500);
#else
						usleep(500000);
#endif
					}
				}
#endif
			}

			retcode = remove_alc_channel(s_id, ch_id[a.nb_channel - 1]);
			a.nb_channel--;
		}
		
		close_alc_session(s_id);
	}
	else if(a.mode == RECEIVER) {
		
		retval = flute_receiver(&a, &s_id, &ch_id[0]);

		if(((retval == -1) && (s_id == -1))) {

#ifdef USE_SDPLIB
			if(strcmp(a.sdp_file, "") != 0) {
				sf_free(a.src_filt);
				free(a.src_filt);
				sdp_free(a.sdp);
				free(a.sdp);
			}
#endif
			if(a.logfd != -1) {
				close(a.logfd);
			}

#ifdef WIN32
			WSACleanup();
#endif
			return retval;
		}
		
		if(retval == -2) {
		}

		if(retval == -3) {
			printf("Sender closed the session\n");
			fflush(stdout);
		}

		if(retval == -4) { /* bind failed */

#ifdef USE_SDPLIB
			if(strcmp(a.sdp_file, "") != 0) {
				sf_free(a.src_filt);
				free(a.src_filt);
				sdp_free(a.sdp);
				free(a.sdp);
			}
#endif
			if(a.logfd != -1) {
				close(a.logfd);
			}

#ifdef WIN32
			WSACleanup();
#endif			
			return retval;	
		}

		if(retval == -5) { /* Exiting in start time waiting period */
		
#ifdef USE_SDPLIB
			if(strcmp(a.sdp_file, "") != 0) {
				sf_free(a.src_filt);
				free(a.src_filt);
				sdp_free(a.sdp);
				free(a.sdp);
			}
#endif
			if(a.logfd != -1) {
				close(a.logfd);
			}

#ifdef WIN32
			WSACleanup();
#endif		
			close_alc_session(s_id);

			return retval;
		}

#ifdef USE_SDPLIB
		if(strcmp(a.sdp_file, "") != 0) {
			sf_free(a.src_filt);
			free(a.src_filt);
			sdp_free(a.sdp);
			free(a.sdp);
		}
#endif

		/* To avoid recvfrom() failed, because of socket closing. */
		set_session_state(s_id, SExiting);
		
		if(a.cc_id == Null) {
			
			while(a.nb_channel) {
				retcode = remove_alc_channel(s_id, ch_id[a.nb_channel - 1]);
				a.nb_channel--;
			}	
		}
#ifdef USE_RLC
		else if(a.cc_id == RLC) {
			
			retcode = remove_alc_channel(s_id, ch_id[0]);
			printf("\n%i packets lost in session.\n", get_alc_session(s_id)->lost_packets);
			fflush(stdout);
		}
#endif

		close_alc_session(s_id);
	}

	if(a.logfd != -1) {
		close(a.logfd);
	}

#ifdef WIN32
	WSACleanup();
#endif

	return retval;
}

/*
 * This function receives and handles signals.
 *
 * Params:	int sig: Must be, not used for anything.
 *
 * Return:	void
 *
 */

void signal_handler(int sig){

	int session_state = get_session_state(s_id);

	if(session_state != -1) {

		printf("\nExiting...\n\n");
		fflush(stdout);

		set_session_state(s_id, SExiting);
	}
}


/*
 * This function print programs usage information.
 *
 * Params:	void
 *
 * Return:	void
 *
 */

void usage(void) {

	char c;

	printf("\nFLUTE Version %s, %s\n\n", MAD_FCL_RELEASE_VERSION, MAD_FCL_RELEASE_DATE);
	printf("  Copyright (c) 2003-2004 TUT - Tampere University of Technology\n");
	printf("  main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi\n");
	printf("  web site: http://www.atm.tut.fi/mad\n\n");
	printf("  This is free software, and you are welcome to redistribute it\n");
	printf("  under certain conditions; See the GNU General Public License\n");
	printf("  as published by the Free Software Foundation, version 2 or later,\n");
	printf("  for more details.\n");

#ifdef USE_RLC
	printf("\n  * MAD-RCL library\n");
	printf("  * mad_rlc.c & mad_rlc.h -- Portions of code derived from MCL library by\n");
	printf("  * Vincent Roca et al. (http://www.inrialpes.fr/planete/people/roca/mcl/)\n");
	printf("  *\n");
	printf("  * Copyright (c) 1999-2004 INRIA - Universite Paris 6 - All rights reserved\n");
	printf("  * (main author: Julien Laboure - julien.laboure@inrialpes.fr\n");
	printf("  *               Vincent Roca - vincent.roca@inrialpes.fr)\n");
#endif

#ifdef USE_REED_SOLOMON
	printf("\n  * MAD-RS-FEC library\n");
	printf("  * fec.c & fec.h -- forward error correction based on Vandermonde matrices\n");
	printf("  * 980624\n");
	printf("  * (C) 1997-98 Luigi Rizzo (luigi@iet.unipi.it)\n");
	printf("  *\n");
	printf("  * Portions derived from code by Phil Karn (karn@ka9q.ampr.org),\n");
	printf("  * Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) and\n");
	printf("  * Hari Thirumoorthy (harit@spectra.eng.hawaii.edu), Aug 1995\n");
	printf("  *\n");
	printf("  * Redistribution and use in source and binary forms, with or without\n");
	printf("  * modification, are permitted provided that the following conditions\n");
	printf("  * are met:\n");
	printf("  *\n");
	printf("  * 1. Redistributions of source code must retain the above copyright\n");
	printf("  *    notice, this list of conditions and the following disclaimer.\n");
	printf("  * 2. Redistributions in binary form must reproduce the above\n");
	printf("  *    copyright notice, this list of conditions and the following\n");
	printf("  *    disclaimer in the documentation and/or other materials\n");
	printf("  *    provided with the distribution.\n");
	printf("  *\n");
	printf("  * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND\n");
	printf("  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,\n");
	printf("  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A\n");
	printf("  * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS\n");
	printf("  * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,\n");
	printf("  * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n");
	printf("  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,\n");
	printf("  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
	printf("  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR\n");
	printf("  * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT\n");
	printf("  * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY\n");
	printf("  * OF SUCH DAMAGE.\n");
#endif

	printf("\nUsage: flute [-S] [-U] [-m:str] [-p:int] [-i:str] [-c:int] [-t:ull]\n"); 
	printf("             [-o:ull] [-e:int] [-F:str,str,...] [-a:str] [-b:int] [-h]\n");
	printf("             [-f:str] [-k:int] [-l:int] [-r:int] [-v:int] [[-n:int] | [-C]]\n");
	printf("             [-T:int] [-P:float,float] [-L:int] [-H] [-A] [-B:str] [-s:str]\n");
	printf("             [-D:ull] [-E:int] [-V:str]");
		
#ifdef WIN32
	printf(" [-I:int]");
#else
	printf(" [-I:str]");
#endif

#ifdef USE_SDPLIB
	printf(" [-d:str]");
#endif

#if defined (USE_REED_SOLOMON) || defined (USE_SIMPLE_XOR)
	printf(" [-x:int]");
#endif

#ifdef USE_REED_SOLOMON
	printf(" [-X:int]");
#endif
	
#ifdef USE_RLC
	printf("\n             [-w:int]");
#endif

#ifdef USE_ZLIB
	printf(" [-z:int]");
#endif

#ifdef WIN32
	printf(" [-O]");
#endif
#ifdef SSM
	printf(" [-M]");
#endif
	printf("\n\n");
	printf("Common options:\n\n");
    
	printf("   -S               Act as sender, send data; otherwise receive data.\n");
	printf("   -U               Address type is unicast, default: multicast\n");
	printf("   -m:str           IP4 or IP6 address for base channel,\n");
	printf("                    default: %s or %s\n", DEF_MCAST_IPv4_ADDR, DEF_MCAST_IPv6_ADDR);
	printf("   -p:int           Port number for base channel, default: %s\n", DEF_MCAST_PORT);
	printf("   -i:str           Local interface to bind to, default: INADDR_ANY\n");
	printf("   -t:ull           TSI for this session, default: %i\n", DEF_TSI);

#ifdef USE_RLC
	printf("   -w:int           Congestion control scheme [0 = Null, 1 = RLC],\n");
	printf("                    default: %i; the number of channels, defined by -c option,\n", DEF_CC);
	printf("                    are used with both schemes and bitrate of each channel is\n");
	printf("                    set according to RLC rules\n");
#endif

	printf("   -a:str           Address family (IP4 or IP6), default: IP4\n");
	printf("   -v:int           Verbosity level, [0 = low, 1 = high] default: 1\n");
	printf("   -V:str           Print logs to 'str' file, default: print to stdout\n");

#ifdef USE_SDPLIB
	printf("   -d:str           SDP file (start/join FLUTE session based on SDP file),\n");
	printf("                    default: no\n");
#endif

	printf("   -h               Print this help.\n");
		
	printf("\nSender options:\n\n");
	
	printf("   -c:int           Number of used channels, default: %i\n", DEF_NB_CHANNEL);

#ifdef USE_ZLIB
	printf("   -z:int           Encode content [0 = no, 1 = FDT, 2 = FDT and files],\n");
	printf("                    default: %i\n", 0);
#endif	

	printf("   -D:ull           Duration of the session in seconds, default: %i\n", DEF_DURATION);

	printf("   -f:str           FDT file (send based on FDT), default: %s\n", DEF_FDT);
	printf("   -k:int           Number of files defined in one FDT Instance,\n");
	printf("                    default: %i\n", DEF_FILES_IN_FDT_INST);
	printf("   -l:int           Encoding symbol length in bytes, default: %i\n", DEF_SYMB_LENGTH);
	printf("   -r:int           Transmission rate at base channel in kbits/s, default: %i\n", DEF_TX_RATE);
	printf("   -T:int           Time To Live or Hop Limit for this session, default: %i\n", DEF_TTL);
	printf("   -e:int           Use EXT_FTI extensions header for file objects [1 = yes,\n");
	printf("                    0 = no], default: 1\n");
	printf("   -F:str           File or directory to be sent\n");
	printf("   -n:int           Number of transmissions, default: %i\n", DEF_TX_NB);
	printf("   -C               Continuous transmission, default: not used\n");
	printf("   -P[:float,float] Simulate packet losses, default: %.1f,%.1f\n", (float)P_LOSS_WHEN_OK,
		    (float)P_LOSS_WHEN_LOSS);

#if defined (USE_REED_SOLOMON) || defined (USE_SIMPLE_XOR)
	printf("   -x:int           FEC Encoding [0 = Null-FEC, 1 = Simple XOR,\n");
	printf("                    2 = Reed-Solomon], default: %i\n", DEF_FEC);
#endif

#ifdef USE_REED_SOLOMON
        printf("   -X:int           FEC ratio percent, default: %i\n", DEF_FEC_RATIO);
#endif


	printf("   -L:int           Maximum source block length in multiple of encoding\n");
	printf("                    symbols, default: %i\n", DEF_MAX_SB_LEN);
	printf("   -H               Use Half-word (if used TSI field could be 16, 32 or 48\n");
	printf("                    bits long and TOI field could be 16, 32, 48 or 64 bits\n");
	printf("                    long), default: not used\n");
	printf("   -B:str           Base directory for files to be sent,\n");
	printf("                    default: working directory\n");

	printf("\nReceiver options:\n\n");

	printf("   -A               Receive files automatically.\n");
	printf("   -F:str,str,...   File(s) to be received\n");
	printf("   -b:int           Receiver in big file mode [0 = no, 1 = yes], default: 1\n"); 
	printf("   -c:int           Maximum number of channels, default: %i\n", DEF_NB_CHANNEL);

/*#ifdef USE_SDPLIB
	printf("   -d:str           SDP file (join based on SDP file), default: no\n");
#endif*/

	printf("   -B:str           Base directory for downloaded files, default: %s\n", DEF_BASE_DIR);
	printf("   -s:str           Source IP4 or IP6 address of this session\n");
	printf("   -o:ull           TOI for the file to be received\n");

#ifdef WIN32
	printf("   -O               Open received file(s) automatically, default: no\n");
#endif

#ifdef SSM
	printf("   -M               Use Source Specific Multicast, default: no\n");
#endif

	printf("   -E:int           Accept Expired FDT Instances [0 = no, 1 = yes], default: 0\n");

#ifdef WIN32
	printf("   -I:int           Local interface index for IPv6 multicast join, use for\n");
	printf("                    example 'ipv6 if' command to see interface indexes;\n");
	printf("                    otherwise OS default\n");
#else
	printf("   -I:str           Local interface name for IPv6 multicast join, use for\n");
	printf("                    example 'ifconfig' command to see interface names;\n");
	printf("                    otherwise OS default\n");
#endif

	printf("\nPress Enter for more help");
	scanf("%c", &c);

	printf("\nExample use cases:\n\n");
	printf("1. Send file or directory n times\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -F:files/flute-draft.txt\n");
	printf("\t      -o:1 -n:2\n\n");
	printf("2. Send file or directory in a loop\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -F:files/flute-draft.txt\n");
	printf("\t      -o:1 -C\n\n");
	printf("3. Send files defined in an FDT file\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -f:fdt2.xml\n\n");
	printf("4. Send files defined in an FDT file in a loop\n\n\tflute -S -m:224.1.1.1 -p:4000 -t:2 -r:100 -f:fdt2.xml -C\n\n");
	printf("5. Send using UDP\n\n\tflute -S -U -m:1.2.3.4 -p:4000 -t:2 -r:100 -f:fdt2.xml -C\n\n");
	printf("6. Receive one object\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n");
	printf("\t      -F:download/flute-draft.txt -o:1\n\n");
	printf("7. Receive file(s) with filename(s)\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n");
	printf("\t      -F:files/flute-man.txt,flute-draft.txt\n\n");
	printf("8. Receive file(s) defined by wild card option\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2 -F:*.jpg\n\n");
	printf("9. Receive file(s) with User Interface\n\n\tflute -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n\n");
	printf("10. Receive file(s) automatically from session\n\n\tflute -A -m:224.1.1.1 -p:4000 -t:2 -s:2.2.2.2\n\n");
	printf("11. Receive using UDP\n\n\tflute -A -U -p:4000 -t:2 -s:2.2.2.2\n\n");

	exit(1);
}

/*
 * This function validates and parses command line arguments
 *
 * Params:	int argc: Number of command line arguments,
 *			char **argv: Pointer to command line arguments,
 *			arguments_t *a: Pointer to arguments structure where command line arguments are parsed.
 *
 * Return:	void
 *
 */

void validateargs(int argc, char **argv, arguments_t *a) {
	int i;
	unsigned int tmp_max_sblen = 0;
	unsigned int tmp_eslen = 0;
	char *tmp_addr = NULL;
	char *tmp_srcaddr = NULL;
	unsigned char fec_enc = DEF_FEC;
	char *tmp_base_dir = NULL;

#ifdef WIN32
	ULONGLONG tmp_tsi;
	ULONGLONG duration = DEF_DURATION;
#else
	char *ep;
	unsigned long long tmp_tsi;
	unsigned long long duration = DEF_DURATION;
#endif
	
	int errno;
	div_t div_max_k;
	div_t div_max_n;

	char *loss_ratio2 = NULL;
	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	time(&systime);
	curr_time = systime + 2208988800;

	/* Set default values for global parameters */

	a->cc_id = DEF_CC;									/* congestion control */

	a->mode = RECEIVER;									/* sender or receiver? */			
	
	tmp_tsi = DEF_TSI;									/* transport session identifier */
	a->toi = 0;											/* transport object identifier */
	
	a->port = DEF_MCAST_PORT;							/* local port number for base channel */
	a->nb_channel = DEF_NB_CHANNEL;						/* number of channels */
	a->intface = NULL;									/* interface */
	a->intface_name = NULL;								/* interface name/index for IPv6 multicast join */

	a->addr_family = DEF_ADDR_FAMILY;
	a->addr_type = 0;
	a->eslen = DEF_SYMB_LENGTH;							/* encoding symbol length */
	a->max_sblen = DEF_MAX_SB_LEN;							/* source block length */
	a->txrate = DEF_TX_RATE;							/* transmission rate in kbit/s */
	a->ttl = DEF_TTL;									/* time to live  */
	a->nb_tx = DEF_TX_NB;								/* nb of time to send the file/directory */
	a->cont = 0;										/* continuous transmission */
	a->simul_losses = false;							/* simulate packet losses */
	a->loss_ratio1 = P_LOSS_WHEN_OK;
	a->loss_ratio2 = P_LOSS_WHEN_LOSS;
	a->fec_ratio = DEF_FEC_RATIO;						/* FEC ratio percent */	
	a->fec_enc_id = DEF_FEC_ENC_ID;						/* FEC Encoding */
	a->fec_inst_id = DEF_FEC_INST_ID; 

	a->files_in_fdt_inst = DEF_FILES_IN_FDT_INST;		/* number of files defined in FDT Instance */

	a->big_file_mode = 1;
	a->verbosity = 1;

	memset(a->file_path, 0, MAX_LENGTH);				/* file(s) or directory to send */
	
	memset(a->fdt_file, 0, MAX_LENGTH);
	strcpy(a->fdt_file, DEF_FDT);						/* fdt file (fdt based send) */
		
	a->rx_automatic = 0;								/* download files defined in IDT automatically */

#ifdef USE_SDPLIB
	memset(a->sdp_file, 0, MAX_LENGTH);
#endif

	a->file_list_mode = 1;

	a->use_fec_oti_ext_hdr = 1;							/* include fec oti extension header to flute header */

#ifdef USE_ZLIB
	a->encode_content = 0;								/* encode content using zlib library */
#endif

#ifdef WIN32
	a->openfile = 0;									/* open received file automatically */
#endif

#ifdef SSM
	a->use_ssm = false;										/* use source specific multicast */
#endif

	a->half_word = false;
	a->accept_expired_fdt_inst = 0;

	a->logfd = -1;

    for(i = 1; i < argc ; i++) {

        if(argv[i][0] == '-') {
            
			switch(argv[i][1])
			{
				case 'A':  /* automatic download */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->rx_automatic = 1;
					a->file_list_mode = 0;
                    break;
				case 'B':  /* base directory for downloaded files */
                    if(strlen(argv[i]) > 3) {
                        tmp_base_dir = &argv[i][3];
					}
					else {
						usage();
					}
					break;
				case 'C':  /* Continuous transmission */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->cont = 1;
                    break;
				case 'e':  /* Use fec oti extension header */
                    if(strlen(argv[i]) > 3) {
                        a->use_fec_oti_ext_hdr = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
				case 'E':  /* Accept Expired FDT Instances */
                    if(strlen(argv[i]) > 3) {
                        a->accept_expired_fdt_inst = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
				case 'F':  /* file(s) or directory to send/receive */
                    if(strlen(argv[i]) > 3) {
                        memset(a->file_path, 0, MAX_LENGTH);
                        strcpy(a->file_path, &argv[i][3]);
                    }
					else {
						usage();
					}
					memset(a->fdt_file, 0, MAX_LENGTH);
					break;
				case 'H':  /* use half-word */
                    if(strlen(argv[i]) > 2) {
                            usage();
                    }
                    a->half_word = true;
                    break;
#ifdef WIN32
				case 'O':  /* automatic opening */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->openfile = 1;
                    break;
#endif

#ifdef SSM
				case 'M':  /* use source specific multicast */
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->use_ssm = true;
                    break;
#endif
				
				case 'P':  /* simulate packet losses */
					if(strlen(argv[i]) >= 2) {
						a->simul_losses = true;
					}
					if(strlen(argv[i]) > 3) {
						a->loss_ratio1 = atof(strtok(&argv[i][3], ","));
						loss_ratio2 = strtok(NULL, "\0");
						if(loss_ratio2 == NULL) {
							usage();
						}
							a->loss_ratio2 = atof(loss_ratio2);
					}
					if(strlen(argv[i]) == 3) {
						usage();
					}
                    break;

#ifdef USE_REED_SOLOMON
                case 'X':  /* FEC ratio percent*/
                    if(strlen(argv[i]) > 3) {
                        a->fec_ratio = atoi(&argv[i][3]);
                    }
                    else {
                        usage();
                    }
                    break;
#endif

				case 'S':  /* sender */
					if(strlen(argv[i]) > 2) {
						usage();
					}
                    a->mode = SENDER;
					a->toi = FDT_TOI;
                    break;
				case 'T':  /* ttl */
                    if(strlen(argv[i]) > 3) {
                       a->ttl = atoi(&argv[i][3]);
                    }
					else {
						usage();
					}
                    break;
				case 'U':  /* unicast address */                    
					if(strlen(argv[i]) > 2) {
						usage();
					}
					a->addr_type = 1;
                    break;
				case 'V': /* redirect stderr and stdout into given file */
					if (strlen(argv[i]) > 3) {
#ifdef WIN32
						a->logfd = _open(&argv[i][3], _O_RDWR | _O_CREAT | _O_TRUNC, 0666);
#else 
						a->logfd = open(&argv[i][3], O_RDWR | O_CREAT | O_TRUNC, 0666);
#endif

						if(a->logfd < 0) {
						
							printf("Log file: %s cannot be opened\n", &argv[i][3]);
							fflush(stdout);
							exit(-1);
						}

#ifdef WIN32
						_dup2(a->logfd, _fileno(stdout));
						_dup2(a->logfd, _fileno(stderr));
#else
						dup2(a->logfd, fileno(stdout));
						dup2(a->logfd, fileno(stderr));
#endif
					}
					break;
				case 'a':  /* address family */
					if(strlen(argv[i]) > 3) {
						if(!(strcmp(&argv[i][3], "IP4"))) {
							a->addr_family = PF_INET;
						}
						else if(!(strcmp(&argv[i][3], "IP6"))) {
							a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130;
							a->addr_family = PF_INET6;
						}
					}
					else {
						usage();
					}
                    break;
				case 'v':  /* verbosity level */
					if(strlen(argv[i]) > 3) {
							a->verbosity = atoi(&argv[i][3]);
					}
					else {
							usage();
					}
                    break;
				case 'b':  /* big file mode */
                    if(strlen(argv[i]) > 3) {
						a->big_file_mode = atoi(&argv[i][3]);
                    }
                    else {
						usage();
                    }
                    break;
				case 'c':  /* number of channels */
                    if(strlen(argv[i]) > 3) {
						a->nb_channel = atoi(&argv[i][3]);

						if(a->nb_channel > MAX_CHANNELS_IN_SESSION) {
							printf("Channel number set to maximum value: %i\n", MAX_CHANNELS_IN_SESSION);
							a->nb_channel = MAX_CHANNELS_IN_SESSION;
						}
						else if(a->nb_channel == 0) {
							printf("Channel number set to 1\n");
							a->nb_channel = 1;
						}
					}
					else {
						usage();
					}
                    break;

#ifdef USE_SDPLIB
				case 'd':  /* sdp file */
                    if(strlen(argv[i]) > 3) {
						memset(a->sdp_file, 0, MAX_LENGTH);
                        strcpy(a->sdp_file, &argv[i][3]);
					}
					else {
						usage();
					}
					break;
#endif

#ifdef USE_ZLIB
				case 'z':  /* encode content */
                    if(strlen(argv[i]) > 3) {
                        a->encode_content = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
#endif
				case 'f':  /* fdt based send */
                    if(strlen(argv[i]) > 3) {
                        memset(a->fdt_file, 0, MAX_LENGTH);
                        strcpy(a->fdt_file, &argv[i][3]);
                    }
					else {
						usage();
					}
					memset(a->file_path, 0, MAX_LENGTH);
					break;
 				case 'i':  /* local interface to use */
                    if(strlen(argv[i]) > 3) {
                        a->intface = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'I':  /* local interface name/index for IPv6 multicast join */
					if(strlen(argv[i]) > 3) {
						a->intface_name = &argv[i][3];
					}
					else {
						usage();
					}
					break;
				case 'h':  /* help/usage */
					usage();
					break;
				case 'k':  /* number of files defined in FDT Instance */
                    if(strlen(argv[i]) > 3) {
                        a->files_in_fdt_inst = atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
				case 'l':  /*  encoding symbol length */
                    if(strlen(argv[i]) > 3) {
						tmp_eslen = (unsigned short)atoi(&argv[i][3]);						
                    }
					else {
						usage();
					}
                    break;
				case 'm':  /* Multicast group for base channel */
                    if(strlen(argv[i]) > 3) {
						tmp_addr = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'n':  /* tx loops */
                    if(strlen(argv[i]) > 3) {
                       a->nb_tx = atoi(&argv[i][3]);
                    }
					else {
						usage();
					}
                    break;
				case 'o':  /* toi */
                    if(strlen(argv[i]) > 3) {
#ifdef WIN32			  
						a->toi = _atoi64(&argv[i][3]);

						if(a->toi > 0xFFFFFFFFFFFFFFFF) {
							printf("TOI: %I64u too big (max=%I64u)\n", a->toi,  0xFFFFFFFFFFFFFFFF);
                            fflush(stdout);
                            exit(-1);
						}
#else
						/*a->toi = atoll(&argv[i][3]);*/
  						
						a->toi = strtoull(&argv[i][3], &ep, 10);

                        if(&argv[i][3] == '\0' || *ep != '\0') {
                                printf("TOI not a number\n");
                                fflush(stdout);
                                exit(-1);
                        }

                        if(errno == ERANGE && a->toi == 0xFFFFFFFFFFFFFFFF) {
                                printf("TOI too big for unsigned long long (64 bits)\n");
                                fflush(stdout);
                                exit(-1);
                        }
#endif	
						a->file_list_mode = 0;
                    }
					else {
						usage();
					}
                    break;
				case 'D':  /* session duration time */
                    if(strlen(argv[i]) > 3) {
#ifdef WIN32			  
						duration = _atoi64(&argv[i][3]);

						if(duration > 0xFFFFFFFFFFFFFFFF) {
							printf("Duration: %I64u too big (max=%I64u)\n", duration,  0xFFFFFFFFFFFFFFFF);
                            fflush(stdout);
                            exit(-1);
						}
#else
						/*a->duration = atoll(&argv[i][3]);*/
  						
						duration = strtoull(&argv[i][3], &ep, 10);

                        if(&argv[i][3] == '\0' || *ep != '\0') {
                            printf("Duration not a number\n");
                            fflush(stdout);
                            exit(-1);
                        }

                        if(errno == ERANGE && duration == 0xFFFFFFFFFFFFFFFF) {
                            printf("Duration too big for unsigned long long (64 bits)\n");
                            fflush(stdout);
                            exit(-1);
                        }
#endif	
                    }
					else {
						usage();
					}
                    break;
                case 'p':  /* Port number for base channel*/
                    if(strlen(argv[i]) > 3) {
						a->port = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 'r':  /* Transmission rate in kbits/s */
                    if(strlen(argv[i]) > 3) {      
						a->txrate = atoi(&argv[i][3]);
                    }
					else {
						usage();
					}
					break;
				case 's':  /* first alc session identifier */
                    if(strlen(argv[i]) > 3) {
						tmp_srcaddr = &argv[i][3];
					}
					else {
						usage();
					}
                    break;
				case 't':  /* second alc session identifier */
                    if(strlen(argv[i]) > 3) {
#ifdef WIN32			  
						tmp_tsi = _atoi64(&argv[i][3]);
#else
						/*tmp_tsi = atoll(&argv[i][3]);*/
				
           				tmp_tsi = strtoull(&argv[i][3], &ep, 10);
           					
						if(&argv[i][3] == '\0' || *ep != '\0') {                   
							printf("TSI not a number\n");
                            fflush(stdout);
                            exit(-1);
						}

           				if(errno == ERANGE && tmp_tsi == 0xFFFFFFFFFFFFFFFF) {   
							printf("TSI too big for unsigned long long (64 bits)\n");
                            fflush(stdout);
                            exit(-1);
						}
#endif 
						if(tmp_tsi > 0xFFFFFFFFFFFF) {
#ifdef WIN32
							printf("TSI: %I64u too big (max=%I64u)\n", tmp_tsi, 0xFFFFFFFFFFFF);
#else
							printf("TSI: %llu too big (max=%llu)\n", tmp_tsi, 0xFFFFFFFFFFFF);
#endif
                            fflush(stdout);
                            exit(-1);
						}
                    }
					else {
						usage();
					}
                    break;

#if defined (USE_REED_SOLOMON) || defined (USE_SIMPLE_XOR)
				case 'x':  /* fec encoding */
                    if(strlen(argv[i]) > 3) {
                        fec_enc = (unsigned char)atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
#endif

				case 'L':  /* sbl length */
                    if(strlen(argv[i]) > 3) {
                        tmp_max_sblen = (unsigned int)atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;

#ifdef USE_RLC
				case 'w':  /* congestion control */
                    if(strlen(argv[i]) > 3) {
						a->cc_id = (unsigned int)atoi(&argv[i][3]);
					}
					else {
						usage();
					}
                    break;
#endif

                default:
					usage();
                    break;
            }
        }
		else {
			usage();
		}
    }

	if(tmp_addr == NULL) {
		if(a->addr_family == PF_INET) {
			tmp_addr = DEF_MCAST_IPv4_ADDR;
		}
		else if(a->addr_family == PF_INET6) {
			tmp_addr = DEF_MCAST_IPv6_ADDR;
		}
	}
	a->addr = tmp_addr;

	if(tmp_srcaddr == NULL) {
		if(a->addr_family == PF_INET) {
			tmp_srcaddr = DEF_SRC_IPv4_ADDR;
		}
		else if(a->addr_family == PF_INET6) {
			tmp_srcaddr = DEF_SRC_IPv6_ADDR;
		}
	}
	a->srcaddr = tmp_srcaddr;

#ifdef USE_SIMPLE_XOR
        if(fec_enc == 1) {
                a->fec_enc_id = SIMPLE_XOR_FEC_ENC_ID;

                if(a->addr_family == PF_INET) {
                        a->eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129;
                }
                else if(a->addr_family == PF_INET6) {
                        a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129;
                }
        }
#endif

#ifdef USE_REED_SOLOMON
	if(fec_enc == 2) {
		a->fec_enc_id = SB_SYS_FEC_ENC_ID;
		a->fec_inst_id = REED_SOL_FEC_INST_ID;
	
		if(a->addr_family == PF_INET) {
			a->eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129;
		}
		else if(a->addr_family == PF_INET6) {
			a->eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129;
		}
	}
#endif

	if(a->mode == RECEIVER) {
		a->tsi = tmp_tsi;

		memset(a->base_dir, 0, MAX_LENGTH);

		if(tmp_base_dir == NULL) {
			strcpy(a->base_dir, DEF_BASE_DIR);
		}
		else {
			strcpy(a->base_dir, tmp_base_dir);
		}

		a->starttime = curr_time;
		a->stoptime = 0;
	}
	else {
		if(((a->half_word == false) && (tmp_tsi > 0xFFFFFFFF))) {
			printf("TSI too big for unsigned long (32 bits)\n");
			fflush(stdout);
			exit(-1);
		}
		else {
			a->tsi = tmp_tsi;
		}

		memset(a->base_dir, 0, MAX_LENGTH);

		if(tmp_base_dir != NULL) {
			strcpy(a->base_dir, tmp_base_dir);
		}

		a->starttime = curr_time;
		a->stoptime = curr_time + duration;
	}

	if(tmp_max_sblen != 0) {
		if(a->fec_enc_id == COM_NO_C_FEC_ENC_ID) {
#ifdef USE_NULL_FEC
			if(tmp_max_sblen > MAX_SB_LEN_NULL_FEC) {
				printf("Maximum source block length set to maximum value: %i\n", MAX_SB_LEN_NULL_FEC);
				fflush(stdout);
				tmp_max_sblen = MAX_SB_LEN_NULL_FEC;
			}
			a->max_sblen = tmp_max_sblen;
#endif
		}

#ifdef USE_SIMPLE_XOR
                else if(a->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) {
                        if(tmp_max_sblen > MAX_SB_LEN_SIMPLE_XOR_FEC) {
                                printf("Maximum source block length set to maximum value: %i\n", MAX_SB_LEN_SIMPLE_XOR_FEC);
                                fflush(stdout);
                                tmp_max_sblen = MAX_SB_LEN_SIMPLE_XOR_FEC;
                        }
                        a->max_sblen = tmp_max_sblen;
                }
#endif

#ifdef USE_REED_SOLOMON
		else if(((a->fec_enc_id == SB_SYS_FEC_ENC_ID) && (a->fec_inst_id == REED_SOL_FEC_INST_ID))) {
			
			div_max_n = div((tmp_max_sblen * (100 + a->fec_ratio)), 100);
			
			if(div_max_n.quot > MAX_N_REED_SOLOMON) {
				
				div_max_k = div((MAX_N_REED_SOLOMON * 100), (100 + a->fec_ratio));
				
				printf("Maximum source block length set to maximum value: %i\n", div_max_k.quot);
				fflush(stdout);
				tmp_max_sblen = div_max_k.quot;
			}
			a->max_sblen = tmp_max_sblen;
		}
#endif
	}

	if(tmp_eslen != 0) {
		if(a->addr_family == PF_INET) {
			if(((a->fec_enc_id == COM_NO_C_FEC_ENC_ID) || (a->fec_enc_id == COM_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130;
				}
			}
			else if(((a->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (a->fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (a->fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129;
				}	
			}
		}
		else if(a->addr_family == PF_INET6) {	
			if(((a->fec_enc_id == COM_NO_C_FEC_ENC_ID) || (a->fec_enc_id == COM_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130;
				}
			}
			else if(((a->fec_enc_id == SIMPLE_XOR_FEC_ENC_ID) || (a->fec_enc_id == SB_LB_E_FEC_ENC_ID) ||
				 (a->fec_enc_id == SB_SYS_FEC_ENC_ID))) {
				if(tmp_eslen > MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129) {
					printf("Encoding symbol length set to maximum value: %i\n",
						MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129);
					fflush(stdout);
					tmp_eslen = MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129;
				}	
			}
		}

		a->eslen = tmp_eslen;
	}
}
