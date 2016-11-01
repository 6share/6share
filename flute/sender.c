/* $Author: peltotal $ $Date: 2004/06/22 05:38:06 $ $Revision: 1.57 $ */
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
#include "sender.h"
#include "fdt.h"
#include "uri.h"

#ifdef USE_ZLIB
#include "mad_zlib.h"
#endif

/*
 * This function generates random number.
 *
 * Params:	int max: Maximum value for random number.
 *
 * Return:	unsigned int: random number between 0 and max.
 *
 */

unsigned int randomnumber(int max) {
	return (unsigned int)(rand()%max);
}

/*
 * This function checks if defined source block is already sent.
 *
 * Params:	int *sent_sbn: Pointer to source block list,
 *			int sbn: Number of Source Block.
 *
 * Return:	int: 1 if source block is sent, 0 otherwise.
 *
 */

int is_sbn_already_sent(int *sent_sbn, int sbn) {
	
	int sent = 0;

	if(*(sent_sbn+sbn)) {
		sent = 1;
	}
	else {
		*(sent_sbn+sbn) = 1;
	}

	return sent;
}

/*
 * This function checks if all source block are sent.
 *
 * Params:	int *sent_sbn: Pointer to source block list,
 *			int nb_sbn: Number of Source Blocks.
 *
 * Return:	int: 1 if all source blocks are sent, 0 otherwise.
 *
 */

int is_all_sbn_sent(int *sent_sbn, int nb_sbn) {
	
	int sent = 1;
	int i;

	for(i = 0; i < nb_sbn; i++) {
		if(*(sent_sbn+i) == 0) {
			sent = 0;
			break;
		}
	}

	return sent;
}

/*
 * This function checks is defined SBN is last one.
 *
 * Params:	int *sent_sbn: Pointer to source block list,
 *			int nb_sbn: Number of Source Blocks.
 *
 * Return:	int: 1 if source block is sent, 0 otherwise.
 *
 */

int is_last_sbn(int *sent_sbn, int nb_sbn) {
	
	int not_sent = 0;
	int ret = 0;
	int i;

	for(i = 0; i < nb_sbn; i++) {
		if(*(sent_sbn+i) == 0) {
			not_sent++;
		}
	}

	if(not_sent > 0) {
		ret = 0;
	}
	else {
		ret = 1;
	}

	return ret;
}

/*
 * This function sends one file.
 *
 * Params:	char *tx_file: Pointer to buffer containing filepath,
 *			int s_id: Session identifier,
 *			int ch_id: Channel identifier,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID.
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int send_file(char *tx_file, int s_id, int ch_id,
#ifdef WIN32
	      ULONGLONG toi,
#else
	      unsigned long long toi,	
#endif

	      unsigned short eslen, unsigned int max_sblen,
		  unsigned char fec_enc_id, unsigned short fec_inst_id) {

#ifdef WIN32
	ULONGLONG toilen;
	ULONGLONG sent = 0;
	ULONGLONG nbytes;
#else
	unsigned long long toilen;
	unsigned long long sent = 0;
	unsigned long long nbytes;
#endif

	FILE *fp;	
	struct stat file_stats;
	char *buf = NULL;

	unsigned int sbn = 0;
	/*long pos;*/
	/*unsigned int random_nb;*/
	/*int *sent_sbn;*/
	blocking_struct_t *bs;
	int is_last_sb = 0;

	/*
	srand((unsigned)time(NULL));
	*/

	if(stat(tx_file, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", tx_file);
		fflush(stdout);
		return -1;
	}

#ifdef WIN32
	toilen = (ULONGLONG)file_stats.st_size;
#else
	toilen = (unsigned long long)file_stats.st_size;
#endif
	
	if(toilen == 0) {
		printf("Tx_file %s size = 0\n", tx_file);
		fflush(stdout);
		return -1;
	}

	if(toilen > 0xFFFFFFFFFFFF) {
		printf("Tx_file %s too big!!\n", tx_file);
		fflush(stdout);
		return -1;
	}
	
	/* Allocate memory for buf */
	if(!(buf = (char*)calloc((unsigned int)(eslen*max_sblen), sizeof(char)))) {
		printf("Could not alloc memory for buf!\n");
		fflush(stdout);
		return -1;
	}

	/* File to send */
	if((fp = fopen(tx_file, "rb")) == NULL) {
		printf("Error: unable to open tx_file %s\n", tx_file);
		fflush(stdout);
		free(buf);
		return -1;
	}

	printf("\nSending file: %s\n\n", tx_file);
	fflush(stdout);

	set_session_sent_bytes(s_id, 0);

	/* Let's compute the blocking structure */
	bs = compute_blocking_structure(toilen, max_sblen, eslen);

	/*
        if(!(sent_sbn = (int*)calloc(bs->N, sizeof(int)))) {
                printf("Could not alloc memory for sent_sbn!\n");
                fflush(stdout);
                fclose(fp);
                free(buf);
		free(bs);
                return -1;
        }
	*/

	while(sbn < bs->N) {

        if(sbn == (bs->N - 1)) {
			is_last_sb = 1;
        }

        if(sbn < bs->I) {
			nbytes = eslen*(bs->A_large);
        }
        else {
			nbytes = eslen*(bs->A_small);
        }

        memset(buf, 0, (eslen * max_sblen));
        nbytes = fread(buf, 1, (unsigned int)nbytes, fp);

		if(ferror(fp)) {
			printf("fread error, file: %s\n", tx_file);
			fflush(stdout);
			free(buf);
			fclose(fp);
			return -1;
		}

		sent = alc_send(s_id, ch_id, buf, nbytes, toi, toilen, eslen, max_sblen, sbn,
						fec_enc_id, fec_inst_id, is_last_sb);

		if(sent == -1) {
            /*free(sent_sbn);*/
            fclose(fp);
            free(buf);
            free(bs);
            return -1;
		}
		else if(sent == -2) {
            /*free(sent_sbn);*/
            fclose(fp);
            free(buf);
            free(bs);
            return -2;
		}

        /*if(get_session_state(s_id) == SExiting) {
            if(is_last_sb){
				free(sent_sbn);
                free(buf);
                free(bs);
                fclose(fp);
                return -2;
            }
        }*/

        sbn++;
	}

/*
	while(1) {

		if(is_all_sbn_sent(sent_sbn, nb_sbn)) {
			
			if(get_session_state(s_id) == SExiting) {

				free(sent_sbn);
				fclose(fp);
				free(buf);
				return -2;
			}

			break;
		}

		random_nb = randomnumber(nb_sbn);

		if(is_sbn_already_sent(sent_sbn, (int)random_nb)) {
			continue;
		}

		pos = (int)(random_nb) * (eslen * max_sblen);

		if((fseek(fp, pos, SEEK_SET)) < 0) {
			printf("fseek error\n");
			fflush(stdout);
			free(sent_sbn);
			free(buf);
			fclose(fp);
			return -1;
		}
      
		memset(buf, 0, (eslen * max_sblen));
		nbytes = fread(buf, 1, (eslen * max_sblen), fp);   

		sent = alc_send(s_id, ch_id, buf, nbytes, toi, toilen, eslen, max_sblen, random_nb,
						is_last_sbn(sent_sbn, nb_sbn));
	
		if(sent == -1) {
			free(sent_sbn);
			fclose(fp);
			free(buf);
			fflush(stdout);
			return -1;
		}
	}
*/
	free(bs);
	fclose(fp);
	free(buf);
	/*free(sent_sbn);*/

	printf("\nFile: %s sent.\n", tx_file);
	fflush(stdout);

	return 0;
}

/*
 * This function sends one file.
 *
 * Params:	char *tx_file: Pointer to buffer containing filepath,
 *			int s_id: Session identifier,
 *			int ch_id: Channel identifier,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID.
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */

int send_file2(char *tx_file, int s_id,
#ifdef WIN32
	       ULONGLONG toi,
#else
	       unsigned long long toi,
#endif

	       unsigned short eslen, unsigned int max_sblen, unsigned char fec_enc_id,
		   unsigned short fec_inst_id) {

#ifdef WIN32
	ULONGLONG toilen;
	ULONGLONG sent = 0;
	ULONGLONG nbytes;
#else
	unsigned long long toilen;
	unsigned long long sent = 0;
	unsigned long long nbytes;
#endif

	FILE *fp;	
	struct stat file_stats;
	char *buf = NULL;
	unsigned int sbn = 0;
	blocking_struct_t *bs;
	int is_last_sb = 0;

	if(stat(tx_file, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", tx_file);
		fflush(stdout);
		return -1;
	}

#ifdef WIN32
	toilen = (ULONGLONG)file_stats.st_size;
#else
	toilen = (unsigned long long)file_stats.st_size;
#endif			
	
	if(toilen == 0) {
		printf("Tx_file %s size = 0\n", tx_file);
		fflush(stdout);
		return -1;
	}

	if(toilen > 0xFFFFFFFFFFFF) {
		printf("Tx_file %s too big!\n", tx_file);
		fflush(stdout);
		return -1;
	}

	/* Allocate memory for buf */
	if(!(buf = (char*)calloc(((eslen*max_sblen) + 1), sizeof(char)))) {
		printf("Could not alloc memory for buf!\n");
		fflush(stdout);
		return -1;
	}

	/* File to send */
	if((fp = fopen(tx_file, "rb")) == NULL) {
		printf("Error: unable to open tx_file %s\n", tx_file);
		fflush(stdout);
		free(buf);
		return -1;
	}

	/* Let's compute the blocking structure */
	bs = compute_blocking_structure(toilen, max_sblen, eslen);

	while(sbn < bs->N) {

		if(sbn == (bs->N - 1)) {
			is_last_sb = 1;
		}

		if(sbn < bs->I) {
			nbytes = eslen*(bs->A_large);
		}
		else {
			nbytes = eslen*(bs->A_small);
		}

		memset(buf, 0, (eslen * max_sblen));
		nbytes = fread(buf, 1, (unsigned int)nbytes, fp);

		if(ferror(fp)) {
			printf("Fread error, file: %s\n", tx_file);
			fflush(stdout);
			free(buf);
			return -1;
		}

		sent = alc_send2(s_id, buf, nbytes, toi, toilen, eslen, max_sblen, sbn, fec_enc_id,
						fec_inst_id);
    
		if(sent == -1) {
			fclose(fp);
			free(buf);
			free(bs);
			return -1;
		}
		else if(sent == -2) {
			fclose(fp);
			free(buf);
			free(bs);
			return -2;
		}

		/*if(get_session_state(s_id) == SExiting) {
			if(is_last_sb){
				free(buf);
				free(bs);
				fclose(fp);
				return -2;
			}
		}*/

		sbn++;
	}

	free(bs);
	fclose(fp);
	free(buf);
//	printf("File: %s added to tx_queue.\n", tx_file);
	fflush(stdout);

	return 0;
}

/*
 * This function sends one FDT Instance.
 *
 * Params:	char *fdt_instance: Pointer to buffer which contains FDT Instance,
 *			ULONGLONG/unsigned long long fdt_inst_len:,
 *			int s_id: Session identifier,
 *			int ch_id: Channel identifier,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID.
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */
                                                                                                                                              
int send_fdt_instance(char *fdt_instance,
#ifdef WIN32
					ULONGLONG fdt_inst_len
#else
					unsigned long long fdt_inst_len
#endif
					, int s_id, int ch_id, unsigned short eslen,
					unsigned int max_sblen, unsigned char fec_enc_id,
					unsigned short fec_inst_id) {

#ifdef WIN32
	ULONGLONG toilen;
	ULONGLONG sentbytes = 0;
	ULONGLONG bytes_left;
    ULONGLONG nbytes;
    ULONGLONG sent = 0;
#else
	unsigned long long toilen;
	unsigned long long sentbytes = 0;
	unsigned long long bytes_left;
    unsigned long long nbytes;
    unsigned long long sent = 0;
#endif
	
	unsigned int sbn = 0;
	int is_last_sb = 0;                                                                                                      char *buf = NULL;

	blocking_struct_t *bs;                
                                                                                                                          
	toilen = fdt_inst_len;
	bytes_left = toilen;
                                                                                                                                           
    printf("\nSending FDT Instance\n\n");
	fflush(stdout);
    
	set_session_sent_bytes(s_id, 0);

    /* Let's compute the blocking structure */

    bs = compute_blocking_structure(toilen, max_sblen, eslen);

	while(sbn < bs->N) {

        if(get_session_state(s_id) == SExiting) {
            free(buf);
            return -2;
        }

        if(sbn == (bs->N -1)) {
            is_last_sb = 1;
        }

        if(sbn < bs->I) {
            nbytes = eslen*(bs->A_large);
        }
        else {
            nbytes = eslen*(bs->A_small);
        }

		nbytes = bytes_left < nbytes ? bytes_left : nbytes; /*min(nbytes, bytes_left);*/

        buf = fdt_instance + (unsigned int)sentbytes;

        sent = alc_send(s_id, ch_id, buf, nbytes, FDT_TOI, toilen, eslen, max_sblen, sbn,
						fec_enc_id, fec_inst_id, is_last_sb);

		if(sent == -1) {
            free(bs);
            return -1;
		}
		else if(sent == -2) {
            free(bs);
            return -2;
		}


		bytes_left -= nbytes;
        sentbytes += sent;
        sbn++;
	}

	free(bs);

	printf("FDT Instance sent.\n");
	fflush(stdout);
	
	return 0;
}

/*
 * This function sends one FDT Instance.
 *
 * Params:	char *fdt_instance: Pointer to buffer which contains FDT Instance,
 *			ULONGLONG/unsigned long long fdt_inst_len:,
 *			int s_id: Session identifier,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID.
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */
                                                                                                                                              
int send_fdt_instance2(char *fdt_instance,
#ifdef WIN32
					ULONGLONG fdt_inst_len
#else
					unsigned long long fdt_inst_len
#endif				
					, int s_id, unsigned short eslen,
					unsigned int max_sblen, unsigned char fec_enc_id,
					unsigned short fec_inst_id) {

#ifdef WIN32
	ULONGLONG toilen;
	ULONGLONG sentbytes = 0;
	ULONGLONG bytes_left;
    ULONGLONG nbytes;
    ULONGLONG sent = 0;
#else
	unsigned long long toilen;
	unsigned long long sentbytes = 0;
	unsigned long long bytes_left;
    unsigned long long nbytes;
    unsigned long long sent = 0;
#endif

	unsigned int sbn = 0;
	int is_last_sb = 0;                                                                                                                                              
    char *buf = NULL;

	blocking_struct_t *bs;
	                                                                                                                          
	toilen = fdt_inst_len;
	bytes_left = toilen;

    /* Let's compute the blocking structure */

    bs = compute_blocking_structure(toilen, max_sblen, eslen);

	while(sbn < bs->N) {

        if(get_session_state(s_id) == SExiting) {
            free(buf);
            return -2;
        }

        if(sbn == (bs->N -1)) {
            is_last_sb = 1;
        }

        if(sbn < bs->I) {
            nbytes = eslen*(bs->A_large);
        }
        else {
            nbytes = eslen*(bs->A_small);
        }

		nbytes = bytes_left < nbytes ? bytes_left : nbytes; /*min(nbytes, bytes_left);*/

        buf = fdt_instance + (unsigned int)sentbytes;

		sent = alc_send2(s_id, buf, nbytes, FDT_TOI, toilen, eslen, max_sblen, sbn, fec_enc_id,
						fec_inst_id);

		if(sent == -1) {
            free(bs);
            return -1;
		}
		else if(sent == -2) {
            free(bs);
            return -2;
		}

		bytes_left -= nbytes;
        sentbytes += sent;
        sbn++;
	}

	free(bs);

//	printf("FDT Instance added to tx_queue.\n");
	fflush(stdout);
	
	return 0;
}

/*
 * This function creates FDT Instance string buffer from file structure(s).
 *
 * Params:	file_t *file: Pointer to first file to be defined in an FDT Instance,
 *			int nb_of_files: Number of files to be defined in an FDT Instance,
 *			fdt_t *fdt:	Pointer to Complete FDT to be splitted to FDT Instances,
 *			int *s_id: Pointer to Session identifier,
 *			ULONGLONG/unsigned long long *fdt_inst_len: Pointer to length of created FDT Instance.
 *
 * Return:	char*: Pointer to created FDT Instance string buffer.
 *
 */

char *create_fdt_instance(file_t *file, int nb_of_files, fdt_t *fdt, int *s_id,
#ifdef WIN32
						  ULONGLONG *fdt_inst_len
#else
						  unsigned long long *fdt_inst_len
#endif
						  ) {

	file_t *tmp_file;
	char *fdt_inst_payload = NULL;
	char tmp_line[MAX_PATH];
	int i;

#ifdef WIN32
	ULONGLONG size = 0;
	ULONGLONG position = 0;
#else
	unsigned long long size = 0;
	unsigned long long position = 0;
#endif

	alc_session_t *s;

	s = get_alc_session(*s_id);

	tmp_file = file;

	memset(tmp_line, 0, MAX_PATH);
	sprintf(tmp_line, "<?xml version=\"1.0\"?>\n");
	size = strlen(tmp_line);

	/* Allocate memory for fdt_inst_payload */
     if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
		return NULL;
    }
	
	memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
	position += strlen(tmp_line);

	memset(tmp_line, 0, MAX_PATH);

#ifdef WIN32
	sprintf(tmp_line, "<FDT-Instance Expires=\"%I64u\"", fdt->expires);
#else
	sprintf(tmp_line, "<FDT-Instance Expires=\"%llu\"", fdt->expires);
#endif
	size += strlen(tmp_line);

	/* Reallocate memory for fdt_inst_payload */
     if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
        return NULL;
    }

	memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
	position += strlen(tmp_line);

#ifdef FDT_INST_FEC_OTI_COMMON
	if(s->use_fec_oti_ext_hdr == 0) {

		/**** FEC-OTI parameters ****/

		memset(tmp_line, 0, MAX_PATH);
		sprintf(tmp_line, "\n\tFEC-OTI-FEC-Encoding-ID=\"%u\"", fdt->fec_encoding_id);
		size += strlen(tmp_line);

		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

		memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);

		if(fdt->fec_encoding_id > 127) {
			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\tFEC-OTI-FEC-Instance-ID=\"%u\"", fdt->fec_instance_id);
			size += strlen(tmp_line);

			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
		}
		
		memset(tmp_line, 0, MAX_PATH);
		sprintf(tmp_line, "\n\tFEC-OTI-Maximum-Source-Block-Length=\"%u\"",
				fdt->max_source_block_length);
		size += strlen(tmp_line);

		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

		memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);
		
		memset(tmp_line, 0, MAX_PATH);
		sprintf(tmp_line, "\n\tFEC-OTI-Encoding-Symbol-Length=\"%u\"",
				fdt->encoding_symbol_length);
		size += strlen(tmp_line);

		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

		memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);

		if(fdt->fec_encoding_id == SB_SYS_FEC_ENC_ID) {
			
			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\tFEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"",
					fdt->max_nb_of_encoding_symbols);
			size += strlen(tmp_line);

			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
		}
	}
#endif
		
	memset(tmp_line, 0, MAX_PATH);
    sprintf(tmp_line, ">\n");
	size += strlen(tmp_line);

	/* Reallocate memory for fdt_inst_payload */
	if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
		return NULL;
	}

	memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
	position += strlen(tmp_line);
	
	for(i = 0; i < nb_of_files; i++) {

		if(tmp_file == NULL) {
			break;
		}

		memset(tmp_line, 0, MAX_PATH);

#ifdef WIN32
		sprintf(tmp_line, "\t<File TOI=\"%I64u\"", tmp_file->toi);
#else
		sprintf(tmp_line, "\t<File TOI=\"%llu\"", tmp_file->toi);
#endif
		size += strlen(tmp_line);
	
		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

		memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);


		memset(tmp_line, 0, MAX_PATH);
		sprintf(tmp_line, "\n\t\tContent-Location=\"%s\"", tmp_file->location);
		size += strlen(tmp_line);
	
		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

		memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);

        memset(tmp_line, 0, MAX_PATH);
#ifdef WIN32
        sprintf(tmp_line, "\n\t\tContent-Length=\"%I64u\"", tmp_file->file_len);
#else
		sprintf(tmp_line, "\n\t\tContent-Length=\"%llu\"", tmp_file->file_len);
#endif
		size += strlen(tmp_line);

		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

        memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);

		if(tmp_file->type != NULL) {
			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\t\tContent-Type=\"%s\"", tmp_file->type);
			size += strlen(tmp_line);
	
			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;

			}

        	memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
		}

        if(tmp_file->md5 != NULL) {
            memset(tmp_line, 0, MAX_PATH);
            sprintf(tmp_line, "\n\t\tContent-MD5=\"%s\"", tmp_file->md5);
			size += strlen(tmp_line);
	
			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
        }

#ifdef USE_ZLIB
		if(s->encode_content == ENCODE_FILES) {

			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\t\tContent-Encoding=\"%s\"", tmp_file->encoding);
			size += strlen(tmp_line);

			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);

			memset(tmp_line, 0, MAX_PATH);
#ifdef WIN32
			sprintf(tmp_line, "\n\t\tTransfer-Length=\"%I64u\"", tmp_file->toi_len);
#else
			sprintf(tmp_line, "\n\t\tTransfer-Length=\"%llu\"", tmp_file->toi_len);
#endif
			size += strlen(tmp_line);

			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else	
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
		}
#endif

		if(s->use_fec_oti_ext_hdr == 0) {
			/**** FEC-OTI parameters ****/

#ifdef FDT_INST_FEC_OTI_FILE

#ifndef FDT_SCHEMA_FLUTEv07
	
			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\t\tFEC-OTI-FEC-Encoding-ID=\"%u\"", tmp_file->fec_encoding_id);
			size += strlen(tmp_line);
	
			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
#endif

			if(tmp_file->fec_encoding_id > 127) {
		
				memset(tmp_line, 0, MAX_PATH);
				sprintf(tmp_line, "\n\t\tFEC-OTI-FEC-Instance-ID=\"%u\"", tmp_file->fec_instance_id);
				size += strlen(tmp_line);
	
				/* Reallocate memory for fdt_inst_payload */
				if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
					return NULL;
				}

				memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
				position += strlen(tmp_line);
			}
		
			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\t\tFEC-OTI-Maximum-Source-Block-Length=\"%u\"",
					tmp_file->max_source_block_length);
			size += strlen(tmp_line);
	
			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
		
			memset(tmp_line, 0, MAX_PATH);
			sprintf(tmp_line, "\n\t\tFEC-OTI-Encoding-Symbol-Length=\"%u\"",
					tmp_file->encoding_symbol_length);
			size += strlen(tmp_line);
	
			/* Reallocate memory for fdt_inst_payload */
			if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
				printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
				return NULL;
			}

			memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
			position += strlen(tmp_line);
	
			
			if(tmp_file->fec_encoding_id == SB_SYS_FEC_ENC_ID) {
				memset(tmp_line, 0, MAX_PATH);
				sprintf(tmp_line, "\n\t\tFEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"",
						tmp_file->max_nb_of_encoding_symbols);
				size += strlen(tmp_line);
	
				/* Reallocate memory for fdt_inst_payload */
				if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
					return NULL;
				}

				memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
				position += strlen(tmp_line);
			}

#elif defined(FDT_INST_FEC_OTI_COMMON)
			
			if(tmp_file->fec_encoding_id != fdt->fec_encoding_id) {
				memset(tmp_line, 0, MAX_PATH);
				sprintf(tmp_line, "\n\t\tFEC-OTI-FEC-Encoding-ID=\"%u\"", tmp_file->fec_encoding_id);
				size += strlen(tmp_line);
		
				/* Reallocate memory for fdt_inst_payload */
				if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
					return NULL;
				}

				memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
				position += strlen(tmp_line);
			}
			
			if(tmp_file->fec_encoding_id > 127) {

				if(tmp_file->fec_instance_id != fdt->fec_instance_id) {
					memset(tmp_line, 0, MAX_PATH);
					sprintf(tmp_line, "\n\t\tFEC-OTI-FEC-Instance-ID=\"%u\"", tmp_file->fec_instance_id);
					size += strlen(tmp_line);
		
					/* Reallocate memory for fdt_inst_payload */
					if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
						printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
						printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
						return NULL;
					}

					memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
					position += strlen(tmp_line);
				}
			}

			if(tmp_file->max_source_block_length != fdt->max_source_block_length) {
				memset(tmp_line, 0, MAX_PATH);
				sprintf(tmp_line, "\n\t\tFEC-OTI-Maximum-Source-Block-Length=\"%u\"",
						tmp_file->max_source_block_length);
				size += strlen(tmp_line);
		
				/* Reallocate memory for fdt_inst_payload */
				if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
					return NULL;
				}

				memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
				position += strlen(tmp_line);
			}
			if(tmp_file->encoding_symbol_length != fdt->encoding_symbol_length) {
				memset(tmp_line, 0, MAX_PATH);
				sprintf(tmp_line, "\n\t\tFEC-OTI-Encoding-Symbol-Length=\"%u\"",
						tmp_file->encoding_symbol_length);
				size += strlen(tmp_line);
		
				/* Reallocate memory for fdt_inst_payload */
				if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
					printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
					return NULL;
				}

				memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
				position += strlen(tmp_line);
			}
        
			if(tmp_file->fec_encoding_id == SB_SYS_FEC_ENC_ID) {
				if(tmp_file->max_nb_of_encoding_symbols != fdt->max_nb_of_encoding_symbols) {
					memset(tmp_line, 0, MAX_PATH);
					sprintf(tmp_line, "\n\t\tFEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"",
							tmp_file->max_nb_of_encoding_symbols);
					size += strlen(tmp_line);
			
					/* Reallocate memory for fdt_inst_payload */
					if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
						printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
						printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
						return NULL;
					}

					memcpy((fdt_inst_payload + (unsigned int)position), tmp_line, strlen(tmp_line));
					position += strlen(tmp_line);
				}
			}
#endif
		}
		
		memset(tmp_line, 0, MAX_PATH);
        sprintf(tmp_line, "/>\n");
		size += strlen(tmp_line);
	
		/* Reallocate memory for fdt_inst_payload */
		if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
			printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
			return NULL;
		}

		memcpy((fdt_inst_payload + position), tmp_line, strlen(tmp_line));
		position += strlen(tmp_line);

		if(tmp_file->next == NULL) {
			break;
		}

		tmp_file = tmp_file->next;
	}
 
	memset(tmp_line, 0, MAX_PATH);
    sprintf(tmp_line, "</FDT-Instance>\n");
	size += strlen(tmp_line);

	size++;
	
	/* Reallocate memory for fdt_inst_payload */
	if(!(fdt_inst_payload = (char*)realloc(fdt_inst_payload, ((unsigned int)size * sizeof(char))))) {
#ifdef WIN32
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %I64u!\n", size);
#else
		printf("Could not (re)alloc memory for fdt_inst_payload, size: %llu!\n", size);
#endif
		return NULL;
	}

    memcpy((fdt_inst_payload + position), tmp_line, strlen(tmp_line));
	position += strlen(tmp_line);
	
	*(fdt_inst_payload + position) = '\0';

	*fdt_inst_len = size - 1;

	return fdt_inst_payload;
}

/*
 * This function sends files defined in FDT file and FDT Instances.
 *
 * Params:	char *fdt_file: Pointer to buffer containing FDT filepath,
 *			int s_id: Session identifier,
 *			int ch_id[MAX_CHANNELS_IN_SESSION]: Channel identifier(s),
 *			int nb_channel: Number of channels,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			int fls_btw_fdt_inst: Files between FDT Instances,
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			int is_last_loop: Is last carousell (1 = yes, 0 = no).
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 * Not used in flute anymore.
 */

int fdtbasedsend(char *fdt_file, int s_id, int ch_id[MAX_CHANNELS_IN_SESSION], int nb_channel,
				 unsigned short eslen, unsigned int max_sblen, int fls_btw_fdt_inst,
				 unsigned char fec_enc_id, unsigned short fec_inst_id, int is_last_loop) {

	char *buf = NULL;
	char *fdt_inst_buf = NULL;

	int sent = 0;
	int nbytes = 0;

	FILE *fp;
	struct stat	file_stats;

	fdt_t *fdt;
	file_t *file;

	/*char *tmp;
	char *filename;
	char path[MAX_PATH];*/

	unsigned int fdt_length = 0;       
	static time_t fdt_mtime = 0;					/* Time of last data modification */
	static int nb_of_instances_in_last_round = 0;

	int nb_files_sent = 0;
	int j;

	unsigned short tmp_eslen = eslen;
	unsigned int tmp_max_sblen = max_sblen;
	unsigned char tmp_fec_enc_id = fec_enc_id;
	unsigned short tmp_fec_inst_id = fec_inst_id;

#ifdef WIN32
	ULONGLONG fdt_inst_len;
#else
	unsigned long long fdt_inst_len;
#endif

	if(stat(fdt_file, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", fdt_file);
		fflush(stdout);
		return -1;
	}
	
	fdt_length = file_stats.st_size;
	
	/* If someone has modified the fdt file, then we don't reset
	   the fdt_instance_id to zero in caroussel beginning. */
	
	if(fdt_mtime != file_stats.st_mtime) {
		fdt_mtime = file_stats.st_mtime;
	}
	else {
		set_fdt_instance_id(s_id, (get_fdt_instance_id(s_id)-nb_of_instances_in_last_round));
	}

	nb_of_instances_in_last_round = 0;

	/* Allocate memory for buf, to read fdt file to it */
	if(!(buf = (char*)calloc((fdt_length + 1), sizeof(char)))) {
		printf("Could not alloc memory for fdt buffer!\n");
		return -1;
	}

	if((fp = fopen(fdt_file, "rb")) == NULL) {
		printf("Error: unable to open tx_file %s\n", fdt_file);
		fflush(stdout);
		free(buf);
		return -1;
	}

	nbytes = fread(buf, 1, fdt_length, fp);

	if(nbytes <= 0) {
		printf("fread error\n");
		fflush(stdout);
		fclose(fp);
		free(buf);
		return -1;
	}

	fdt = decode_fdt_payload(buf);
	free(buf);
	fclose(fp);

	if(fdt == NULL) {
		return -1;
	}

	file = fdt->file_list;
	
	while(file != NULL) { /* loop for sending all files in fdt */

		if(!(nb_files_sent%fls_btw_fdt_inst)) { /* check is it time to send fdt instance */

			fdt_inst_buf = create_fdt_instance(file, fls_btw_fdt_inst, fdt, &s_id, &fdt_inst_len);

			for(j = 0; j < nb_channel; j++) {
				
				/*sent = send_file(fdt_file, s_id, ch_id[j], FDT_TOI, eslen, max_sblen);*/
			
				sent = send_fdt_instance(fdt_inst_buf, strlen(fdt_inst_buf), s_id, ch_id[j], eslen,
										max_sblen, fec_enc_id, fec_inst_id);

				nb_of_instances_in_last_round++;				

				if(sent == -1) {
					/*printf("send_file() failed\n");
					fflush(stdout);*/
	                free(fdt_inst_buf);
        	        FreeFDT(fdt);
					return -1;
				}
				else if(sent == -2) {
					/*printf("send_file() stopped\n");	
					fflush(stdout);*/
	                free(fdt_inst_buf);
        	        FreeFDT(fdt);
					return -2;
				}
			}

			set_fdt_instance_id(s_id, (get_fdt_instance_id(s_id) + 1));
			free(fdt_inst_buf);

			/* Sleep after fdt_instance so that a receiver can parse and process it */

#ifdef WIN32
			Sleep(500);
#else
			usleep(500000);
#endif /* WIN32 */
		}

		/* tmp is for parsing file->location */

		/*if(!(tmp = (char*)calloc((strlen(file->location) + 1), sizeof(char)))) {
			printf("Could not alloc memory for tmp (file location)!\n");
			return -1;
		}
		memcpy(tmp, file->location, strlen(file->location));
										
		filename = strtok(tmp, "/");
		filename = strtok(NULL, "\n");

		strcpy(path, filename);*/

		for(j = 0; j < nb_channel; j++) { /* send file to all channels */

			if(((file->next == NULL) && (is_last_loop))) {	
				set_session_a_flag(s_id);
			}

			if(((file->encoding_symbol_length != 0) && (file->max_source_block_length != 0))) {
			
				tmp_eslen = file->encoding_symbol_length;
				tmp_max_sblen = file->max_source_block_length;
			}
			
			if(file->fec_encoding_id != -1) {
				if(file->fec_encoding_id != tmp_fec_enc_id) {
					tmp_fec_enc_id = (unsigned char)file->fec_encoding_id;
					tmp_fec_inst_id = (unsigned short)file->fec_instance_id;
				}
			}

			/*sent = send_file(path, s_id, ch_id[j], file->toi, tmp_eslen, tmp_max_sblen);*/

			sent = send_file(file->location, s_id, ch_id[j], file->toi, tmp_eslen, tmp_max_sblen,
							tmp_fec_enc_id, tmp_fec_inst_id);

			if(sent == -1) {
				/*printf("\nsend_file() failed\n");	
				fflush(stdout);*/
				/*free(tmp);*/
				FreeFDT(fdt);
				return -1;
			}
			else if(sent == -2) {
				/*printf("\nsend_file() stopped\n");	
				fflush(stdout);*/
                                /*free(tmp);*/
                                FreeFDT(fdt);
				return -2;
			}
		}
						
		/*free(tmp);*/
		file = file->next;
		nb_files_sent++;
		
		/* Sleep between files so that receiver can decode them */
#ifdef WIN32
		Sleep(500);
#else
		usleep(500000);
#endif /* WIN32 */

	}

	FreeFDT(fdt);
	return 0;
}

/*
 * This function sends files defined in FDT file and FDT Instances.
 *
 * Params:	char *fdt_file: Pointer to buffer containing FDT filepath,
 *			char *base_dir: Pointer to buffer containing base directory for file
 *							or directory to be parsed to the fdt,
 *			char *file_path: Pointer to buffer containing path of file/directory to be sent,
 *			int *s_id: Pointer to Session identifier,
 *			int *ch_id: Pointer to Channel identifier,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			int files_in_fdt_inst: Files defined in FDT Instance (Maximum number),
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			int use_fec_oti_ext_hdr: Use EXT_FTI header [0 = no, 1 = yes],
 *			int encode_content: Encode content (1 = yes, 0 = no),
 *			int is_last_loop: Is last carousell (1 = yes, 0 = no).
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */

int fdtbasedsend2(char *fdt_file, char *base_dir, char *file_path, int *s_id, int *ch_id, unsigned short eslen,
				  unsigned int max_sblen, int files_in_fdt_inst, unsigned char fec_enc_id,
				  unsigned short fec_inst_id, int use_fec_oti_ext_hdr,
#ifdef USE_ZLIB
				  int encode_content, int is_last_loop) {
#else
				  int is_last_loop) {
#endif

	char *buf = NULL;
	char *fdt_inst_buf = NULL;

#ifdef USE_ZLIB
	char *compr_fdt_inst_buf = NULL;

#ifdef WIN32
	ULONGLONG compr_fdt_inst_buf_len = 0;
#else
	unsigned long long compr_fdt_inst_buf_len = 0;
#endif

#endif

#ifdef WIN32
	int j;
#endif

	FILE *fp;
	struct stat file_stats;

	fdt_t *fdt;
	file_t *file;
	uri_t *uri;

	char path[MAX_PATH];

	
	static time_t fdt_mtime = 0;					/* Time of last FDT modification */
	static int nb_of_instances_in_last_round = 0;

	int nb_files_sent = 0;

	unsigned short tmp_eslen = eslen;
	unsigned int tmp_max_sblen = max_sblen;
	unsigned char tmp_fec_enc_id = fec_enc_id;
	unsigned short tmp_fec_inst_id = fec_inst_id;

	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
	ULONGLONG fdt_inst_len;
	ULONGLONG fdt_length = 0;
	ULONGLONG sent = 0;
	LONGLONG nbytes = 0;
#else
	unsigned long long curr_time;
	unsigned long long fdt_inst_len;
	unsigned long long fdt_length = 0;
	unsigned long long sent = 0;
	long long nbytes = 0;
#endif

	bool incomplete_fdt = false;

	time(&systime);
	curr_time = systime + 2208988800;

	if(stat(fdt_file, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", fdt_file);
		fflush(stdout);
		return -1;
	}

#ifdef WIN32
	fdt_length = (ULONGLONG)file_stats.st_size;
#else
	fdt_length = (unsigned long long)file_stats.st_size;
#endif

	/* If someone has modified the fdt file, then we don't reset
	   the fdt_instance_id to zero in caroussel beginning. */

	if(fdt_mtime != file_stats.st_mtime) {
		fdt_mtime = file_stats.st_mtime;
	}
	else {
		set_fdt_instance_id(*s_id, (get_fdt_instance_id(*s_id) - nb_of_instances_in_last_round));
	}

	nb_of_instances_in_last_round = 0;

	/* Allocate memory for buf, to read fdt file to it */
	if(!(buf = (char*)calloc((unsigned int)(fdt_length + 1), sizeof(char)))) {
		printf("Could not alloc memory for fdt buffer!\n");
		return -1;
	}
	
	if((fp = fopen(fdt_file, "rb")) == NULL) {
		printf("Error: unable to open tx_file %s\n", fdt_file);
		fflush(stdout);
		free(buf);
		return -1;
	}

	nbytes = fread(buf, 1, (unsigned int)fdt_length, fp);

	if(nbytes <= 0) {
		printf("fread error\n");
		fflush(stdout);
		fclose(fp);
		free(buf);
		return -1;
	}

	fdt = decode_fdt_payload(buf);
	free(buf);
	fclose(fp);

	if(fdt == NULL) {
		return -1;
	}

	if(fdt->expires < curr_time) {
		set_session_state(*s_id, SExiting);
		FreeFDT(fdt);
		return -2;
	}

	if (fdt->file_list == NULL) return 866181; /* FIXME defcon-1 */

	file = fdt->file_list;

	while(file != NULL) { /* loop for sending all files in fdt */

		if(use_fec_oti_ext_hdr == 0) {
			
			if(file->fec_encoding_id == -1) {
				incomplete_fdt = true;
			}
			else if(((file->fec_encoding_id > 127) && (file->fec_instance_id == -1))) {
				incomplete_fdt = true;
			}
			else if(file->max_source_block_length == 0) {
				incomplete_fdt = true;
			}
			else if(file->encoding_symbol_length == 0) {
				incomplete_fdt = true;
			}
			else if(((file->fec_encoding_id == SB_SYS_FEC_ENC_ID) &&
					(file->max_nb_of_encoding_symbols == 0))) {
				incomplete_fdt = true;
			}

			if(incomplete_fdt) {
#ifdef WIN32
				printf("FDT does not contain FEC-OTI information, TOI: %I64u\n", file->toi);
#else
				printf("FDT does not contain FEC-OTI information, TOI: %llu\n", file->toi);
#endif
				fflush(stdout);
				FreeFDT(fdt);
				return -1;
			}

			if(file->toi == 0) {
				incomplete_fdt = true;
			}
			else if(file->location == NULL) {
				incomplete_fdt = true;
			}

#ifdef USE_ZLIB
			if(((encode_content == 2) && (file->toi_len == 0))) {
				incomplete_fdt = true;
			}
			else if(((encode_content != 2) && (file->toi_len == 0))) {
				incomplete_fdt = true;
			}
#else	
			if(file->file_len == 0) {
				incomplete_fdt = true;
			}	
#endif
			
			if(incomplete_fdt) {
#ifdef WIN32
				printf("FDT does not contain enough File information, TOI: %I64u\n", file->toi);
#else
				printf("FDT does not contain enough File information, TOI: %llu\n", file->toi);
#endif
				fflush(stdout);
				FreeFDT(fdt);
				return -1;
			}
		}

		if(!(nb_files_sent%files_in_fdt_inst)) {

			if(fdt_inst_buf != NULL) {
				free(fdt_inst_buf);
			}

#ifdef USE_ZLIB
			if(encode_content) {
				if(compr_fdt_inst_buf != NULL) {
					free(compr_fdt_inst_buf);
				}
			}
#endif

			fdt_inst_buf = create_fdt_instance(file, files_in_fdt_inst, fdt, s_id, &fdt_inst_len);

			if(fdt_inst_buf == NULL) {
				FreeFDT(fdt);
				return -1;
			}

#ifdef USE_ZLIB
			if(encode_content) {

				compr_fdt_inst_buf = buffer_zlib_compress(fdt_inst_buf, fdt_inst_len,
														  &compr_fdt_inst_buf_len);

				if(compr_fdt_inst_buf == NULL) {
					free(fdt_inst_buf);
					FreeFDT(fdt);
					return -1;
				}	
			}
#endif

			set_fdt_instance_id(*s_id, (get_fdt_instance_id(*s_id) + 1));
			nb_of_instances_in_last_round++;
		}

#ifdef USE_ZLIB
		if(encode_content) {

			sent = send_fdt_instance(compr_fdt_inst_buf, compr_fdt_inst_buf_len, *s_id, *ch_id, eslen,
									max_sblen, fec_enc_id, fec_inst_id);			
		}
		else {
			sent = send_fdt_instance(fdt_inst_buf, fdt_inst_len, *s_id, *ch_id, eslen, max_sblen,
									fec_enc_id, fec_inst_id);	
		}
#else
		sent = send_fdt_instance(fdt_inst_buf, fdt_inst_len, *s_id, *ch_id, eslen, max_sblen,
								fec_enc_id, fec_inst_id);		
#endif
		
		if(sent == -1) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif
			free(fdt_inst_buf);
        		FreeFDT(fdt);
			return -1;
		}
		else if(sent == -2) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif
			free(fdt_inst_buf);
        		FreeFDT(fdt);
			return -2;
		}

#ifdef WIN32
		Sleep(500);
#else
		usleep(500000);
#endif

		uri = parse_uri(file->location, strlen(file->location));

		memset(path, 0, MAX_PATH);

		if(!(strcmp(base_dir, "") == 0)) {
			strcpy(path, base_dir);
			strcat(path, "/");
		}
		strcat(path, uri->path);

#ifdef USE_ZLIB
		if(encode_content == ENCODE_FILES) {
			strcat(path, GZ_SUFFIX);	
		}
#endif

		if(((file->next == NULL) && (is_last_loop))) {
			set_session_a_flag(*s_id);
		}

		if(((file->encoding_symbol_length != 0) && (file->max_source_block_length != 0))) {
		
			tmp_eslen = file->encoding_symbol_length;
			tmp_max_sblen = file->max_source_block_length;
		}

		if(file->fec_encoding_id != -1) {
			if(file->fec_encoding_id != tmp_fec_enc_id) {
				tmp_fec_enc_id = (unsigned char)file->fec_encoding_id;
				tmp_fec_inst_id = (unsigned short)file->fec_instance_id;
			}
		}

#ifdef WIN32
		for(j = 0; j < (int)strlen(path); j++) {
			if(*(path + j) == '/') {
				*(path + j) = '\\';
			}
		}
#endif

		sent = send_file(path, *s_id, *ch_id, file->toi, tmp_eslen, tmp_max_sblen, tmp_fec_enc_id,
						tmp_fec_inst_id);

		free_uri(uri);

		if(sent == -1) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif

			free(fdt_inst_buf);
			FreeFDT(fdt);
			return -1;
		}
		else if(sent == -2) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif

			free(fdt_inst_buf);
			FreeFDT(fdt);
			return -2;
		}
						
		file = file->next;
		nb_files_sent++;
		
		/* Sleep between files so that receiver can decode them */

#ifdef WIN32
		Sleep(500);
#else
		usleep(500000);
#endif

	}

#ifdef USE_ZLIB
	if(encode_content) {
		free(compr_fdt_inst_buf);
	}
#endif

	free(fdt_inst_buf);	
	FreeFDT(fdt);
	
	return 0;
}

/*
 * This function sends files defined in FDT file and FDT Instances.
 *
 * Params:	char *fdt_file: Pointer to buffer containing FDT filepath,
 *			char *base_dir: Pointer to buffer containing base directory for file
 *							or directory to be parsed to the fdt,
 *			char *file_path: Pointer to buffer containing path of file/directory to be sent,
 *			int *s_id: Pointer to Session identifier,
 *			int nb_channel: Number of channels,
 *			unsigned short eslen: Encoding Symbol Length,
 *			unsigned int max_sblen: Maximum-Size Source Block Length,
 *			int files_in_fdt_inst: Files defined in FDT Instance (Maximum number),
 *			unsigned char fec_enc_id: FEC Encoding ID,
 *			unsigned short fec_inst_id: FEC Instance ID,
 *			int use_fec_oti_ext_hdr: Use EXT_FTI header [0 = no, 1 = yes],
 *			int encode_content: Encode content (1 = yes, 0 = no).
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */

int fdtbasedsend3(char *fdt_file, char *base_dir, char *file_path, int *s_id, int nb_channel, unsigned short eslen,
				  unsigned int max_sblen, int files_in_fdt_inst, unsigned char fec_enc_id,
				  unsigned short fec_inst_id, int use_fec_oti_ext_hdr
#ifdef USE_ZLIB
				  , int encode_content) {
#else
				  ) {
#endif

	char *buf = NULL;
	char *fdt_inst_buf = NULL;

#ifdef USE_ZLIB
	char *compr_fdt_inst_buf = NULL;

#ifdef WIN32
	ULONGLONG compr_fdt_inst_buf_len = 0;
#else
	unsigned long long compr_fdt_inst_buf_len = 0;
#endif

#endif

#ifdef WIN32
	int j;
#endif

	FILE *fp;
	struct stat file_stats;

	fdt_t *fdt;
	file_t *file;
	uri_t *uri;

	char path[MAX_PATH];

	static time_t fdt_mtime = 0;					/* Time of last FDT modification */
	static int nb_of_instances_in_last_round = 0;
	int nb_files_sent = 0;

	unsigned short tmp_eslen = eslen;
	unsigned int tmp_max_sblen = max_sblen;
	unsigned char tmp_fec_enc_id = fec_enc_id;
	unsigned short tmp_fec_inst_id = fec_inst_id;

	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
	ULONGLONG fdt_inst_len;
	ULONGLONG fdt_length = 0;
	ULONGLONG sent = 0;
	LONGLONG nbytes = 0;
#else
	unsigned long long curr_time;
	unsigned long long fdt_inst_len;
	unsigned long long fdt_length = 0;
	unsigned long long sent = 0;
	long long nbytes = 0;
#endif

	bool incomplete_fdt = false;

	time(&systime);
	curr_time = systime + 2208988800;

	if(stat(fdt_file, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", fdt_file);
		fflush(stdout);
		return -1;
	}

	fdt_length = file_stats.st_size;
	
	/* If someone has modified the fdt file, then we don't reset
	   the fdt_instance_id to zero in caroussel beginning. */

	if(fdt_mtime != file_stats.st_mtime) {
		fdt_mtime = file_stats.st_mtime;
	}
	else {
		set_fdt_instance_id(*s_id, (get_fdt_instance_id(*s_id) - nb_of_instances_in_last_round));
	}

	nb_of_instances_in_last_round = 0;

	/* Allocate memory for buf, to read fdt file to it */
	if(!(buf = (char*)calloc((unsigned int)(fdt_length+1), sizeof(char)))) {
		printf("Could not alloc memory for fdt buffer!\n");
		fflush(stdout);
		return  -1;
	}

	if((fp = fopen(fdt_file, "rb")) == NULL) {
		printf("Error: unable to open tx_file %s\n", fdt_file);
		fflush(stdout);
		free(buf);
		return -1;
	}

	nbytes = fread(buf, 1, (unsigned int)fdt_length, fp);

	if(nbytes <= 0) {
		printf("fread error\n");
		fflush(stdout);
		fclose(fp);
		free(buf);
		return -1;
	}

	fdt = decode_fdt_payload(buf);
	free(buf);
	fclose(fp);

	if(fdt == NULL) {
		return -1;
	}

	if(fdt->expires < curr_time) {
		set_session_state(*s_id, SExiting);
		FreeFDT(fdt);
		return -2;
	}

	if (fdt->file_list == NULL) return 866181; /* FIXME defcon-1 */

	file = fdt->file_list;

	while(file != NULL) { /* loop for sending all files in fdt */

		if(use_fec_oti_ext_hdr == 0) {
			
			if(file->fec_encoding_id == -1) {
				incomplete_fdt = true;
			}
			else if(((file->fec_encoding_id > 127) && (file->fec_instance_id == -1))) {
				incomplete_fdt = true;
			}
			else if(file->max_source_block_length == 0) {
				incomplete_fdt = true;
			}
			else if(file->encoding_symbol_length == 0) {
				incomplete_fdt = true;
			}
			else if(((file->fec_encoding_id == SB_SYS_FEC_ENC_ID) &&
					(file->max_nb_of_encoding_symbols == 0))) {
				incomplete_fdt = true;
			}

			if(incomplete_fdt) {
#ifdef WIN32
				printf("FDT does not contain FEC-OTI information, TOI: %I64u\n", file->toi);
#else
				printf("FDT does not contain FEC-OTI information, TOI: %llu\n", file->toi);
#endif
				fflush(stdout);
				FreeFDT(fdt);
				return -1;
			}

			if(file->toi == 0) {
				incomplete_fdt = true;
			}
			else if(file->location == NULL) {
				incomplete_fdt = true;
			}

#ifdef USE_ZLIB
			if(((encode_content == 2) && (file->toi_len == 0))) {
				incomplete_fdt = true;
			}
			else if(((encode_content != 2) && (file->toi_len == 0))) {
				incomplete_fdt = true;
			}
#else	
			if(file->file_len == 0) {
				incomplete_fdt = true;
			}	
#endif
			
			if(incomplete_fdt) {
#ifdef WIN32
				printf("FDT does not contain enough File information, TOI: %I64u\n", file->toi);
#else
				printf("FDT does not contain enough File information, TOI: %llu\n", file->toi);
#endif
				fflush(stdout);
				FreeFDT(fdt);
				return -1;
			}
		}

		if(!(nb_files_sent%files_in_fdt_inst)) {

			if(fdt_inst_buf != NULL) {
				free(fdt_inst_buf);
			}

#ifdef USE_ZLIB
			if(encode_content) {
				if(compr_fdt_inst_buf != NULL) {
					free(compr_fdt_inst_buf);
				}
			}
#endif

			fdt_inst_buf = create_fdt_instance(file, files_in_fdt_inst, fdt, s_id, &fdt_inst_len);

			if(fdt_inst_buf == NULL) {
				FreeFDT(fdt);
				return -1;
			}

#ifdef USE_ZLIB
			if(encode_content) {

				compr_fdt_inst_buf = buffer_zlib_compress(fdt_inst_buf, fdt_inst_len,
														  &compr_fdt_inst_buf_len);

				if(compr_fdt_inst_buf == NULL) {
					free(fdt_inst_buf);
					FreeFDT(fdt);
					return -1;
				}	
			}
#endif
				
			set_fdt_instance_id(*s_id, (get_fdt_instance_id(*s_id) + 1));
			nb_of_instances_in_last_round++;
		}

#ifdef USE_ZLIB
		if(encode_content) {
			sent = send_fdt_instance2(compr_fdt_inst_buf, compr_fdt_inst_buf_len, *s_id, eslen, max_sblen,
									  fec_enc_id, fec_inst_id);			
		}
		else {
			sent = send_fdt_instance2(fdt_inst_buf, fdt_inst_len, *s_id, eslen, max_sblen, fec_enc_id,
										fec_inst_id);	
		}
#else
		sent = send_fdt_instance2(fdt_inst_buf, fdt_inst_len, *s_id, eslen, max_sblen, fec_enc_id,
									fec_inst_id);		
#endif
		
		if(sent == -1) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif

			free(fdt_inst_buf);
        	FreeFDT(fdt);
			return -1;
		}
		else if(sent == -2) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif

			free(fdt_inst_buf);
        	FreeFDT(fdt);
			return -2;
		}

		uri = parse_uri(file->location, strlen(file->location));

		memset(path, 0, MAX_PATH);

		if(!(strcmp(base_dir, "") == 0)) {
			strcpy(path, base_dir);
			strcat(path, "/");
		}
		strcat(path, uri->path);

#ifdef USE_ZLIB
		if(encode_content == ENCODE_FILES) {
			strcat(path, GZ_SUFFIX);	
		}
#endif

		/**** TODO: add support ****/
		/*if(((file->encoding_symbol_length != 0) && (file->max_source_block_length != 0))) {
			tmp_eslen = file->encoding_symbol_length;
			tmp_max_sblen = file->max_source_block_length;
		}*/

		if(file->fec_encoding_id != -1) {
			if(file->fec_encoding_id != tmp_fec_enc_id) {
				tmp_fec_enc_id = (unsigned char)file->fec_encoding_id;
				tmp_fec_inst_id = (unsigned short)file->fec_instance_id;
			}
		}

#ifdef WIN32
		for(j = 0; j < (int)strlen(path); j++) {
			if(*(path + j) == '/') {
				*(path + j) = '\\';
			}
		}
#endif

		sent = send_file2(path, *s_id, file->toi, tmp_eslen, tmp_max_sblen, tmp_fec_enc_id,
							tmp_fec_inst_id);

		free_uri(uri);

		if(sent == -1) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif

			free(fdt_inst_buf);
			FreeFDT(fdt);
			return -1;
		}
		else if(sent == -2) {

#ifdef USE_ZLIB
			if(encode_content) {
				free(compr_fdt_inst_buf);
			}
#endif

			free(fdt_inst_buf);
			FreeFDT(fdt);
			return -2;
		}

		file = file->next;
		nb_files_sent++;
	}

#ifdef USE_ZLIB
	if(encode_content) {
		free(compr_fdt_inst_buf);
	}
#endif

	free(fdt_inst_buf);
	FreeFDT(fdt);

	return 0;
}

#ifdef USE_ZLIB

/*
 * This function removes tempororary ~gz files in sender side.
 *
 * Params:	char *fdt_file: Pointer to buffer containing fdt file name,
 *			char *base_dir: Pointer to buffer containing base directory for file
 *							or directory to be parsed to the fdt,
 *			char *file_path: Pointer to buffer containing file or directory parsed to the fdt,			
 *
 * Return:	int: 0 in success, -1 otherwise
 *
 */

int remove_gz_files(char *fdt_file, char *base_dir, char *file_path) {
	
	uri_t *uri;
	char path[MAX_PATH];
	file_t *file;
	fdt_t *fdt;
	char *buf = NULL;
	struct stat	file_stats;
	unsigned int fdt_length = 0; 
    FILE *fp;
	int nbytes;

#ifdef WIN32
	int j;
#endif
	
	if(stat(fdt_file, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", fdt_file);
		fflush(stdout);
		return -1;
	}

	fdt_length = file_stats.st_size;

	/* Allocate memory for buf, to read fdt file to it */
	if(!(buf = (char*)calloc((fdt_length+1), sizeof(char)))) {
		printf("Could not alloc memory for fdt buffer!\n");
		fflush(stdout);
		return  -1;
	}

	if((fp = fopen(fdt_file, "rb")) == NULL) {
		printf("Error: unable to open fdt_file %s\n", fdt_file);
		fflush(stdout);
		free(buf);
		return -1;
	}

	nbytes = fread(buf, 1, fdt_length, fp);

	if(nbytes <= 0) {
		printf("fread error\n");
		fflush(stdout);
		fclose(fp);
		free(buf);
		return -1;
	}

	fdt = decode_fdt_payload(buf);
	free(buf);
	fclose(fp);

	if(fdt == NULL) {
		return -1;
	}
	
	file = fdt->file_list;

	while(file != NULL) {

		uri = parse_uri(file->location, strlen(file->location));

		memset(path, 0, MAX_PATH);

		if(!(strcmp(base_dir, "") == 0)) {
			strcpy(path, base_dir);
			strcat(path, "/");
		}
		strcat(path, uri->path);
		strcat(path, GZ_SUFFIX);

#ifdef WIN32
		for(j = 0; j < (int)strlen(path); j++) {
			if(path[j] == '/') {
				path[j] = '\\';
			}
		}
#endif
		remove(path);
		file = file->next;
	}

	return 0;
}

#endif

