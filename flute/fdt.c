/* $Author: peltotal $ $Date: 2004/06/22 06:36:21 $ $Revision: 1.35 $ */
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

#include <expat.h>

#include "../alclib/inc.h"
#include "fdt.h"

#ifdef USE_ZLIB
#include "mad_zlib.h"
#endif

/* Global variables */

fdt_t *fdt;
file_t *file;
file_t *prev;
bool is_first_toi;

/* Private functions */

int copy_file_info(file_t *src, file_t *dest);
int update_fdt(fdt_t *fdt_db, fdt_t *instance);
void PrintFDT(fdt_t *fdt, int s_id);

/*
 * This function parses FDT XML document to fdt structure.
 *
 * Params:	void *userData: Not used,
 *			const char *name: Pointer to buffer containing element name,
 *			const char **atts: Pointer to buffer containing element's attributes.
 *
 * Return:	static void
 *
 */

static void startElement(void *userData, const char *name, const char **atts) {

#ifndef WIN32
	char *ep;
#endif

	int errno;

	while(*atts != NULL) {
	  
		if(!strcmp(name, "File")) {
			
			if(file == NULL) {

				if(!(file = (file_t*)calloc(1, sizeof(file_t)))) {
					printf("Could not alloc memory for mad_fdt file!\n");
					return;
				}

				/* initialise file parameters */
				file->prev = NULL;
				file->next = NULL;
				file->toi = 0;
				file->is_downloaded = 0;
				file->expires = fdt->expires;
				file->toi_len = 0;
				file->file_len = 0;
				file->location = NULL;
				file->type = NULL;
				file->md5 = NULL;
				file->encoding = NULL;

				file->fec_encoding_id = fdt->fec_encoding_id;
				file->fec_instance_id = fdt->fec_instance_id;
				file->max_source_block_length = fdt->max_source_block_length;
				file->encoding_symbol_length = fdt->encoding_symbol_length;
				file->max_nb_of_encoding_symbols = fdt->max_nb_of_encoding_symbols;
			}

			if(!strcmp(*atts, "TOI")) {
	      
#ifdef WIN32    
				file->toi = _atoi64(*(++atts));

				if(file->toi > 0xFFFFFFFFFFFFFFFF) {
					printf("TOI too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
				}
#else               
				file->toi = strtoull(*(++atts), &ep, 10);

                if(*(atts) == '\0' || *ep != '\0') {
                    printf("Content-Length not a number\n");
                    fflush(stdout);
                    exit(-1);
                }

                if(errno == ERANGE && file->toi == 0xFFFFFFFFFFFFFFFF) {
                    printf("TOI too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }
#endif
		
				if(is_first_toi) {
					fdt->file_list = file;
					is_first_toi = false;
				}
				else {
					prev->next = file;
					file->prev = prev;
				}
	      
				prev = file;
			}
			else if(!strcmp(*atts, "Content-Location")) {

				atts++;
      
				if(!(file->location = (char*)calloc((strlen(*atts) + 1), sizeof(char)))) {
					printf("Could not alloc memory for file->location!\n");
					return;
				}
			  
				memcpy(file->location, *atts, strlen(*atts));
			}
			else if(!strcmp(*atts, "Content-Length")) {

#ifdef WIN32     
				file->file_len = _atoi64(*(++atts));
				
				if(file->file_len > 0xFFFFFFFFFFFFFFFF) {
                    printf("Content-Length too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }
#else               
				/*file->file_len = atoll(*(++atts));*/

				file->file_len = strtoull(*(++atts), &ep, 10);

                if(*(atts) == '\0' || *ep != '\0') {
                    printf("Content-Length not a number\n");
                    fflush(stdout);
                    exit(-1);
                }

                if(errno == ERANGE && file->file_len == 0xFFFFFFFFFFFFFFFF) {
                    printf("Content-Length too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }	
#endif
				if(file->toi_len == 0) {
					file->toi_len = file->file_len;
				}
			}
			else if(!strcmp(*atts, "Transfer-Length")) {

#ifdef WIN32			  
				file->toi_len = _atoi64(*(++atts));

				if(file->toi_len > 0xFFFFFFFFFFFFFFFF) {
                    printf("Transfer-Length too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }
#else
				/*file->toi_len = atoll(*(++atts));*/

				file->toi_len = strtoull(*(++atts), &ep, 10);

                if(*(atts) == '\0' || *ep != '\0') {
                    printf("Transfer-Length not a number\n");
                    fflush(stdout);
                    exit(-1);
                }

                if(errno == ERANGE && file->toi_len == 0xFFFFFFFFFFFFFFFF) {
                    printf("Transfer-Length too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }
#endif 
				if(file->toi_len > 0xFFFFFFFFFFFF) {
#ifdef WIN32
					printf("Transfer-Length too big (max=%I64u)\n", 0xFFFFFFFFFFFF);
#else
					printf("Transfer-Length too big (max=%llu)\n", 0xFFFFFFFFFFFF);
#endif
                    fflush(stdout);
                    exit(-1);
                }
			}
			else if(!strcmp(*atts, "Content-Type")) {
			  
				atts++;

				if(!(file->type = (char*)calloc((strlen(*atts) + 1), sizeof(char)))) {
					printf("Could not alloc memory for file->type!\n");
					return;
				}

				memcpy(file->type, *atts, strlen(*atts));
			}
			else if(!strcmp(*atts, "Content-Encoding")) {
			  
				atts++;
			  
				if(!(file->encoding = (char*)calloc((strlen(*atts) + 1), sizeof(char)))) {
					printf("Could not alloc memory for file->encoding!\n");
					return;
				}
			  
				memcpy(file->encoding, *atts, strlen(*atts));
			}
			else if(!strcmp(*atts, "Content-MD5")) {
			  
				atts++;

				if(!(file->md5 = (char*)calloc((strlen(*atts) + 1), sizeof(char)))) {
					printf("Could not alloc memory for file->md5!\n");
					return;
				}

				memcpy(file->md5, *atts, strlen(*atts));
			}
			else if(!strcmp(*atts, "FEC-OTI-FEC-Encoding-ID")) {
                    file->fec_encoding_id = (unsigned char)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-FEC-Instance-ID")) {
                    file->fec_instance_id = (unsigned short)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-Maximum-Source-Block-Length")) {
                    file->max_source_block_length = (unsigned int)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-Encoding-Symbol-Length")) {
                    file->encoding_symbol_length = (unsigned short)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-Max-Number-of-Encoding-Symbols")) {
                    file->max_nb_of_encoding_symbols = (unsigned short)atoi(*(++atts));
            }
			else {			  
				atts++;
			}

			atts++;
		}
		else if(!strcmp(name, "FDT-Instance")) {

			if(!strcmp(*atts, "Expires")) {  
#ifdef WIN32
				fdt->expires = _atoi64(*(++atts));

				if(fdt->expires > 0xFFFFFFFFFFFFFFFF) {
                    printf("Expires too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }
#else
				/*fdt->expires = atoll(*(++atts));*/

                fdt->expires = strtoull(*(++atts), &ep, 10);

                if(*(atts) == '\0' || *ep != '\0') {
                    printf("Expires not a number\n");
                        fflush(stdout);
                        exit(-1);
                }

                if(errno == ERANGE && fdt->expires == 0xFFFFFFFFFFFFFFFF) {
                    printf("Expires too big for unsigned long long (64 bits)\n");
                    fflush(stdout);
                    exit(-1);
                }
#endif
			}
			else if(!strcmp(*atts, "Complete")) {
				if(!strcmp("true", *atts)) {
					fdt->is_complete = true;
				}
				else {
					fdt->is_complete = false;
				}
			}
			else if(!strcmp(*atts, "FEC-OTI-FEC-Encoding-ID")) {
				fdt->fec_encoding_id = (unsigned char)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-FEC-Instance-ID")) {
				fdt->fec_instance_id = (unsigned short)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-Maximum-Source-Block-Length")) {
				fdt->max_source_block_length = (unsigned int)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-Encoding-Symbol-Length")) {
				fdt->encoding_symbol_length = (unsigned short)atoi(*(++atts));
            }
            else if(!strcmp(*atts, "FEC-OTI-Max-Number-of-Encoding-Symbols")) {
				fdt->max_nb_of_encoding_symbols = (unsigned short)atoi(*(++atts));
            }
			else {
				atts++;
			}
			atts++;
		}
		else {
			atts += 2;
		}
	}

	file = NULL;
}

/*
 * This function decodes FDT XML document to fdt structure by using Expat XML library.
 *
 * Params:	char *fdt_payload: Pointer to buffer containing FDT XML document.
 *
 * Return:	fdt_t*: Pointer to fdt structure.
 *
 */

fdt_t* decode_fdt_payload(char *fdt_payload) {

	XML_Parser parser = XML_ParserCreate(NULL);
	size_t len;
 	
	len = strlen(fdt_payload);
	fdt = NULL;
	
	if(!(fdt = (fdt_t*)calloc(1, sizeof(fdt_t)))) {
		printf("Could not alloc memory for fdt!\n");
		XML_ParserFree(parser);
		return NULL;
	}

	/* initialise fdt parameters */
	fdt->expires = 0;
	fdt->is_complete = false;
	fdt->fec_encoding_id = -1;
	fdt->fec_instance_id = -1;
	fdt->max_source_block_length = 0;
	fdt->encoding_symbol_length = 0;
	fdt->max_nb_of_encoding_symbols = 0;

	file = NULL;
	prev = NULL;
	is_first_toi = true;

	XML_SetStartElementHandler(parser, startElement);

	if(XML_Parse(parser, fdt_payload, len, 1) == XML_STATUS_ERROR) {
		fprintf(stderr, "%s at line %d\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser));
		XML_ParserFree(parser);
		return NULL;
	}

	XML_ParserFree(parser);

	return fdt;
}

/*  
 * This function receives and decodes FDT Instance.
 * 
 * Params:	void *s: Pointer to alc session	
 *
 * Return:	void
 *
 */

void* fdt_thread(void *a) {

	fdt_th_args_t *args;
	char *buf = NULL;

#ifdef WIN32
	ULONGLONG buflen;
#else
	unsigned long long buflen;
#endif

	int updated;
		
	fdt_t *fdt_instance;
	time_t systime;
	file_t *file;
	file_t *next_file;
	int retval;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

#ifdef USE_ZLIB
	char *uncompr_buf = NULL;

#ifdef WIN32
	ULONGLONG uncompr_buflen;
#else
	unsigned long long uncompr_buflen;
#endif

	unsigned char cont_enc_algo = 0;
#endif

	args = (fdt_th_args_t *)a;

	buflen = 0;

#ifdef USE_ZLIB
	uncompr_buflen = 0;
#endif

	while(get_session_state(args->s_id) == SActive) {

		time(&systime);
		curr_time = systime + 2208988800;

		/* Get initial fdt */
		if(args->fdt == NULL) {

#ifdef USE_ZLIB
			buf = fdt_recv(args->s_id, &buflen, &retval, &cont_enc_algo);
#else
			buf = fdt_recv(args->s_id, &buflen, &retval);
#endif

			if(buf == NULL) {

				if(retval == -1) {
					continue;
				}

				/* end thread */

				/*printf("FDT thread exit\n");
				fflush(stdout);*/
#ifdef WIN32
				ExitThread(0);
#else
				pthread_exit(0);
#endif
			}

#ifdef USE_ZLIB
			if(cont_enc_algo == ZLIB) {
				uncompr_buf = buffer_zlib_uncompress(buf, buflen, &uncompr_buflen);

				if(uncompr_buf == NULL) {
					free(buf);
					continue;
				}
				fdt_instance = decode_fdt_payload(uncompr_buf);
				free(uncompr_buf);
			}
			else {
				fdt_instance = decode_fdt_payload(buf);
			}
#else 
			fdt_instance = decode_fdt_payload(buf);
#endif
			
			if(fdt_instance == NULL) {
				free(buf);
				continue;
			}

			if(fdt_instance->expires < curr_time) {


				if(args->accept_expired_fdt_inst == 0) {
					printf("\nExpired FDT Instance received, discarding\n\n");
					fflush(stdout);
					free(buf);
					FreeFDT(fdt_instance);
					continue;
				}
				else {
					printf("\nExpired FDT Instance received, using it anyway\n\n");
					fflush(stdout);
				}
			}

			args->fdt = fdt_instance;

			printf("\nFDT Instance received\n");
			printf("FDT updated, new file description(s) added\n\n");
			fflush(stdout);
			PrintFDT(fdt_instance, args->s_id);
			free(buf);

			if(args->rx_automatic == 1) {
				next_file = args->fdt->file_list;

				while(next_file != NULL) {
					file = next_file;

					if(file->is_downloaded == 0) {

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
								next_file = file->next;
								continue;
							}
						}

						retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
													   file->encoding_symbol_length,
													   file->max_source_block_length,
													   file->fec_instance_id,
													   file->fec_encoding_id,
													   file->max_nb_of_encoding_symbols, cont_enc_algo);
#else
						retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
													   file->encoding_symbol_length,
													   file->max_source_block_length,
													   file->fec_instance_id,
													   file->fec_encoding_id,
													   file->max_nb_of_encoding_symbols);
#endif
						if(retval < 0) {
							/* Memory error */
						}
						else {
							file->is_downloaded = 1;
						}
					}	
				
					next_file = file->next;
				}
			}
			else if(args->filetoken != NULL) {
				next_file = args->fdt->file_list;

				while(next_file != NULL) {
					file = next_file;

					if(strstr(file->location, args->filetoken) != NULL) {

						if(file->is_downloaded == 0) {

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
									next_file = file->next;
									continue;
								}
							}

							retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
														   file->encoding_symbol_length,
														   file->max_source_block_length,
														   file->fec_instance_id,
														   file->fec_encoding_id,
														   file->max_nb_of_encoding_symbols, cont_enc_algo);
#else
							retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
														   file->encoding_symbol_length,
														   file->max_source_block_length,
														   file->fec_instance_id,
														   file->fec_encoding_id,
														   file->max_nb_of_encoding_symbols);
#endif
							if(retval < 0) {
								/* Memory error */
							}
							else {
								file->is_downloaded = 1;
							}
						}
					}	
				
					next_file = file->next;
				}
			}
		}
		else { /* Receive new FDT Instance when it comes */
			updated = 0;
			/*printf("Updating File Delivery Table...\n");
			fflush(stdout);*/

#ifdef USE_ZLIB
			buf = fdt_recv(args->s_id, &buflen, &retval, &cont_enc_algo);
#else
			buf = fdt_recv(args->s_id, &buflen, &retval);
#endif

			if(buf == NULL) {

				if(retval == -1) {
					continue;
				}
					
				/* end thread */
					
				/*printf("FDT thread exit\n");
				fflush(stdout);*/
#ifdef WIN32
				ExitThread(0);
#else
				pthread_exit(0);
#endif
			}

#ifdef USE_ZLIB
			if(cont_enc_algo == ZLIB) {
				uncompr_buf = buffer_zlib_uncompress(buf, buflen, &uncompr_buflen);

				if(uncompr_buf == NULL) {
					free(buf);
					continue;
				}
				fdt_instance = decode_fdt_payload(uncompr_buf);
				free(uncompr_buf);
			}
			else {
				fdt_instance = decode_fdt_payload(buf);
			}
#else 
			fdt_instance = decode_fdt_payload(buf);
#endif

			if(fdt_instance == NULL) {
				free(buf);
				continue;
			}


			if(fdt_instance->expires < curr_time) {
printf("now %lld : expires %lld\n", curr_time, fdt_instance->expires);
				if(args->accept_expired_fdt_inst == 0) {
					printf("\nExpired FDT Instance received, discarding\n\n");
					fflush(stdout);
					FreeFDT(fdt_instance);
					free(buf);
					continue;
				}
				else {
					printf("\nExpired FDT Instance received, using it anyway\n\n");
					fflush(stdout);
				}
			}

//			printf("\nFDT Instance received\n");
			fflush(stdout);
			free(buf);

			updated = update_fdt(args->fdt, fdt_instance);
			
			if(updated < 0) {
				continue;
			}
			else if(updated == 0) {
//				printf("\n");
			}
			else if(updated == 1) {
				printf("FDT updated, file description(s) complemented\n\n");
				fflush(stdout);
			}
			else if(updated == 2) {
				printf("FDT updated, new file description(s) added\n\n");
				fflush(stdout);
				PrintFDT(fdt_instance, args->s_id);

				if(args->rx_automatic == 1) {
					next_file = args->fdt->file_list;

					while(next_file != NULL) {
						file = next_file;

						if(file->is_downloaded == 0) {

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
								next_file = file->next;
								continue;
							}
						}

						retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
													   file->encoding_symbol_length,
													   file->max_source_block_length,
													   file->fec_instance_id,
													   file->fec_encoding_id,
													   file->max_nb_of_encoding_symbols,
													   cont_enc_algo);
#else
						retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
													   file->encoding_symbol_length,
													   file->max_source_block_length,
													   file->fec_instance_id,
													   file->fec_encoding_id,
													   file->max_nb_of_encoding_symbols);
#endif

							if(retval < 0) {
								/* Memory error */
							}
							else {
								file->is_downloaded = 1;
							}
						}	
					
						next_file = file->next;
					}
				}
				else if(args->filetoken != NULL) {

					next_file = args->fdt->file_list;

					while(next_file != NULL) {
						file = next_file;

						if(strstr(file->location, args->filetoken) != NULL) {

							if(file->is_downloaded == 0) {

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
										next_file = file->next;
										continue;
									}
								}

								retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
															   file->encoding_symbol_length,
															   file->max_source_block_length,
															   file->fec_instance_id,
															   file->fec_encoding_id,
															   file->max_nb_of_encoding_symbols, cont_enc_algo);
#else
								retval = set_wanted_object(args->s_id, file->toi, file->toi_len,
															   file->encoding_symbol_length,
															   file->max_source_block_length,
															   file->fec_instance_id,
															   file->fec_encoding_id,
															   file->max_nb_of_encoding_symbols);
#endif
								if(retval < 0) {
									/* Memory error */
								}
								else {
									file->is_downloaded = 1;
								}
							}
						}	
					
						next_file = file->next;
					}
				}
			}

            FreeFDT(fdt_instance);
		}
	}
	
	/*printf("FDT thread exit\n");
	fflush(stdout);*/

	/* end thread */
#ifdef WIN32
	ExitThread(0);
#else
	pthread_exit(0);
#endif /* WIN32 */
}

/*
 * This function frees FDT structure
 *
 * Params:	fdt_t *fdt: Pointer to FDT structure to be freed.
 *
 * Return:	void
 *
 */

void FreeFDT(fdt_t *fdt) {

	file_t *next_file;
	file_t *file;

	/**** Free FDT struct ****/

	next_file = fdt->file_list;

	while(next_file != NULL) {
		file = next_file;
							
		if(file->encoding != NULL) {
			free(file->encoding);
		}
							
		if(file->location != NULL) {
			free(file->location);
		}
								
		if(file->md5 != NULL) {
			free(file->md5);
		}

		if(file->type != NULL) {
			free(file->type);
		}
					
		next_file = file->next;
		free(file);
	}

	free(fdt);
}

/*
 * This function prints FDT information
 *
 * Params:      fdt_t *fdt: Pointer to FDT structure to be freed,
 *				int s_id: Session Identifier.
 *
 * Return:      void
 *
 */

void PrintFDT(fdt_t *fdt, int s_id) {

	file_t *next_file;
	file_t *file;
	char encoding[5] = "null"; 
	char *enc = encoding;
    
	next_file = fdt->file_list;
    
	while(next_file != NULL) {	
		file = next_file;
		
		if(file->encoding != NULL) {
			enc = file->encoding;
		}
		
#ifdef WIN32
		printf("URI: %s (TOI=%I64u)\n",  file->location, file->toi);
#else
		printf("URI: %s (TOI=%llu)\n",  file->location, file->toi);
#endif
		fflush(stdout);

		next_file = file->next;
	}
}

	char *encoding;

/* FEC OTI */

	short fec_encoding_id;
	int fec_instance_id;
	unsigned int max_source_block_length;
	unsigned short encoding_symbol_length;
	unsigned short max_nb_of_encoding_symbols;

/*
 * This function frees file structure
 *
 * Params:	file_t *file: Pointer to file structure to be freed.
 *
 * Return:	void
 *
 */

void free_file(file_t *file) {

	if(file->encoding != NULL) {
		free(file->encoding);
	}
							
	if(file->location != NULL) {
		free(file->location);
	}
								
	if(file->md5 != NULL) {
		free(file->md5);
	}

	if(file->type != NULL) {
		free(file->type);
	}
					
	free(file);
}


/*
 * This function copies file description from source to destination. 
 *
 * Params:	file_t *src: Pointer to source file structure,
 *			file_t *dest: Pointer to destination file structure.
 *
 * Return:	int: 1 if file description is updated, 0 if not, and -1 otherwise
 *
 */

int copy_file_info(file_t *src, file_t *dest) {

	int updated = 0;

	/* Copy only if particular field is not present in destination, so file description can be only
       complemented not modified */
	
	if(src->toi != 0) {
		if(dest->toi == 0) {
			dest->toi = src->toi;
			updated = 1;
		}
	}

	if(src->expires != 0) {
		if(dest->expires == 0) {
			dest->expires = src->expires;
			updated = 1;
		}
	}

	if(src->toi_len != 0) {
		if(dest->toi_len == 0) {
			dest->toi_len = src->toi_len;
			updated = 1;
		}
	}

	if(src->file_len != 0) {
		if(dest->file_len == 0) {
			dest->file_len = src->file_len;
			updated = 1;
		}
	}

	if(src->fec_instance_id != 0) {
		if(dest->fec_instance_id == 0) {
			dest->fec_instance_id = src->fec_instance_id;
			updated = 1;
		}
	}

	if(src->max_source_block_length != 0) {
		if(dest->max_source_block_length == 0) {
			dest->max_source_block_length = src->max_source_block_length;
			updated = 1;
		}
	}

	if(src->encoding_symbol_length != 0) {
		if(dest->encoding_symbol_length == 0) {
			dest->encoding_symbol_length = src->encoding_symbol_length;
			updated = 1;
		}
	}

	if(src->max_nb_of_encoding_symbols != 0) {
		if(dest->max_nb_of_encoding_symbols == 0) {
			dest->max_nb_of_encoding_symbols = src->max_nb_of_encoding_symbols;
			updated = 1;
		}
	}

	if(src->location != NULL) {
		
		if(dest->location == NULL) {

			if(!(dest->location  = (char*)calloc((strlen(src->location) + 1), sizeof(char)))) {
				printf("Could not alloc memory for file->location!\n");
				return -1;
			}

			memcpy(dest->location, src->location, strlen(src->location));
			updated = 1;
		}
	}

	if(src->type != NULL) {

		if(dest->type == NULL) {

			if(!(dest->type  = (char*)calloc((strlen(src->type) + 1), sizeof(char)))) {
				printf("Could not alloc memory for file->type!\n");
				return -1;
			}
					  
			memcpy(dest->type, src->type, strlen(src->type));
			updated = 1;
		}
	}

	if(src->md5 != NULL) {

		if(dest->md5 == NULL) {

			if(!(dest->md5 = (char*)calloc((strlen(src->md5) + 1), sizeof(char)))) {
				printf("Could not alloc memory for file->md5!\n");
				return -1;
			}
					  
			memcpy(dest->md5, src->md5, strlen(src->md5));
			updated = 1;
		}
	}

	if(src->encoding != NULL) {

		if(dest->encoding == NULL) {

			if(!(dest->encoding = (char*)calloc((strlen(src->encoding) + 1), sizeof(char)))) {
				printf("Could not alloc memory for file->encoding!\n");
				return -1;
			}
					  
			memcpy(dest->encoding, src->encoding, strlen(src->encoding));
			updated = 1;
		}
	}

	return updated;
}

/*
 * This function updates FDT database.
 *
 * Params:	fdt_t *fdt_db: Pointer to FDT database,
 *			fdt_t *instance: Pointer to received FDT Instance.
 *
 * Return:	int: 1 (existing file description is complemented or 2 (new file description entity) if 
 *				 FDT database is updated, 0 if not, and -1 otherwise
 *
 */

int update_fdt(fdt_t *fdt_db, fdt_t *instance) {

	file_t *tmp_file;
	file_t *fdt_file;
	file_t *new_file;
	int retval = 0;
	int updated = 0;

	tmp_file = instance->file_list;

	while(tmp_file != NULL) {

                /* Test Print */

	/*
	#ifdef WIN32
                printf("TOI:%I64u\n", tmp_file->toi);
	#else
				printf("TOI:%llu\n", tmp_file->toi);
	#endif
                printf("Content-Location:%s\n", tmp_file->location);
                printf("Content-Length:%u\n", tmp_file->file_len);
                printf("Transfer-Length:%u\n", tmp_file->toi_len);
                printf("Content-Type:%s\n", tmp_file->type);
                printf("Content-Encoding:%s\n", tmp_file->encoding);
                printf("Content-MD5:%s\n", tmp_file->md5);
                printf("FEC-OTI-FEC-Instance-ID:%u\n", tmp_file->fec_instance_id);
                printf("FEC-OTI-Maximum-Source-Block-Length:%u\n", tmp_file->max_source_block_length);
                printf("FEC-OTI-Encoding-Symbol-Length:%u\n", tmp_file->encoding_symbol_length);
                printf("FEC-OTI-Max-Number-of-Encoding-Symbols:%u\n", tmp_file->max_nb_of_encoding_symbols);
	*/

		fdt_file = fdt_db->file_list;

		for(;; fdt_file = fdt_file->next) {

			if(tmp_file->toi == fdt_file->toi) {

				retval = copy_file_info(tmp_file, fdt_file);

				if(retval < 0) {
					return -1;
				}
				else if(retval == 1) {
					updated = 1;
				}

				break;
			}
			else if(fdt_file->next != NULL) {
				continue;
			}
			else {

				if(!(new_file = (file_t*)calloc(1, sizeof(file_t)))) {
					printf("Could not alloc memory for mad_fdt file!\n");
					return -1;
				}
						
				retval = copy_file_info(tmp_file, new_file);

				if(retval < 0) {
					return -1;
				}
				else if(retval == 1) {
					updated = 2;
				}

				new_file->next = fdt_file->next;
				new_file->prev = fdt_file;
				fdt_file->next = new_file;
				
				break;
			}
		}

		tmp_file = tmp_file->next;
	}
	return updated;
}
