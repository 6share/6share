/* $Author: peltotal $ $Date: 2004/10/15 07:50:01 $ $Revision: 1.38 $ */
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

#include "receiver.h"
#include "uri.h"

#ifdef USE_ZLIB
#include "mad_zlib.h"
#endif

#ifdef USE_OPENSSL
#include "mad_md5.h"
#endif

/*
 * This function receives file identified with TOI.
 *
 * Params:	int s_id: Session identifier,
 *			char *filepath: Pointer to files path string, used when creating file and storing data to it,
 *			ULONGLONG/unsigned long long toi: Transport Object Identifier for desired file,
 *			char *md5: MD5 checksum for the file,
 *			int big_file_mode: Receive big files,
 *			char *encoding: Content-Encoding algorithm for file,
 *			ULONGLONG\unsigned long long file_len: Length of file.
 *
 * Return:	int: Different values (1, 0, -1, -2, -3, -4) depending on how function ends
 *
 */

int recvfile(int s_id, char *filepath,
#ifdef WIN32
	     ULONGLONG toi,
#else
	     unsigned long long toi,
#endif
	     char *md5, int big_file_mode

#ifdef USE_ZLIB
			 , char *encoding
#ifdef WIN32	
			 , ULONGLONG file_len
#else

			 , unsigned long long file_len
#endif

#endif
			 ) {
	
#ifdef USE_ZLIB
	char *uncompr_buf = NULL;

#ifdef WIN32
	ULONGLONG uncompr_buflen = 0;
#else
	unsigned long long uncompr_buflen = 0;
#endif

	unsigned char fdt_cont_enc_algo = 0;
	struct stat	file_stats;
#endif

#ifdef WIN32
	ULONGLONG recvbytes = 0;
#else
	unsigned long long recvbytes = 0;
#endif
	
	char *buf = NULL;
	int fd;

	char *ptr;
	int point; 
	int ch = '/';
	int i = 0;
	int j;

	char *tmp = NULL;
	char fullpath[MAX_PATH];
	char filename[MAX_PATH];

	char *tmp_file_name;
	int retcode = 0;
	int retval;

#ifdef USE_OPENSSL
	char *md5_digest = NULL;
#endif

	char* session_basedir = get_session_basedir(s_id);

#ifdef WIN32
	printf("Receiving file: %s (TOI=%I64u)\n\n", filepath, toi);
#else
	printf("Receiving file: %s (TOI=%llu)\n\n", filepath, toi);
#endif
	fflush(stdout);

	for(j = 0; j < (int)strlen(filepath); j++) {
		if(*(filepath + j) == '\\') {
			*(filepath + j) = '/';
		}
	}

	if(toi == FDT_TOI) {
#ifdef USE_ZLIB
		buf = fdt_recv(s_id, &recvbytes, &retcode, &fdt_cont_enc_algo);

		if(buf == NULL) {
			return retcode;
		}

		if(fdt_cont_enc_algo == ZLIB) {
			uncompr_buf = buffer_zlib_uncompress(buf, recvbytes, &uncompr_buflen);

			if(uncompr_buf == NULL) {
				free(buf);
				return -1;
			}

			free(buf);
		}
#else
		buf = fdt_recv(s_id, &recvbytes, &retcode);

		if(buf == NULL) {
			return retcode;
		}
#endif

	}
	else {	
		if(big_file_mode) {

			tmp_file_name = alc_recv3(s_id, &toi, &retcode);

			if(tmp_file_name == NULL) {
				return retcode;
			}

#ifdef USE_OPENSSL
			if(md5 != NULL) {

				md5_digest = file_md5(tmp_file_name);

				if(md5_digest == NULL) {
					printf("MD5 check failed!\n\n");
					fflush(stdout);
					remove(tmp_file_name);
					free(tmp_file_name);
					return -4;
				}
				else{
					if(strcmp(md5, md5_digest) != 0) {
						printf("MD5 check failed!\n\n");
						fflush(stdout);
						remove(tmp_file_name);
						free(tmp_file_name);
						free(md5_digest);
						return -4;
					}

					free(md5_digest);
				}
			}
#endif

#ifdef USE_ZLIB

			if(encoding != NULL) {

				if(strcmp(encoding, "gzip") == 0) {
					
					retcode = file_gzip_uncompress(tmp_file_name);

					if(retcode == -1) {
						free(tmp_file_name);
						return -1;
					}

					*(tmp_file_name + (strlen(tmp_file_name) - GZ_SUFFIX_LEN)) = '\0';

					
					if(file_len != 0) {

						if(stat(tmp_file_name, &file_stats) == -1) {
							printf("Error: %s is not valid file name\n", tmp_file_name);
							fflush(stdout);
							free(tmp_file_name);
							return -1;
						}

						if(file_stats.st_size != file_len) {
							printf("Error: uncompression failed\n");
							remove(tmp_file_name);
							free(tmp_file_name);
							fflush(stdout);
							return -1;
						}
					}
				}
			}
#endif
		}
		else {
			buf = alc_recv(s_id, toi, &recvbytes, &retcode);

			if(buf == NULL) {
                /*printf("recvfile() buf NULL\n");*/
                return retcode;
        	}

#ifdef USE_OPENSSL
			if(md5 != NULL) {
				md5_digest = buffer_md5(buf, recvbytes);

				if(md5_digest == NULL) {
					printf("MD5 check failed!\n\n");
					fflush(stdout);

					free(buf);
					return -4;
				}
				else{
					if(strcmp(md5, md5_digest) != 0) {
						printf("MD5 check failed!\n\n");
						fflush(stdout);

						free(buf);
						free(md5_digest);

						return -4;
					}

					free(md5_digest);
				}
			}
#endif

#ifdef USE_ZLIB
			if(encoding != NULL) {

				if(strcmp(encoding, "gzip") == 0) {
					
					uncompr_buf = buffer_gzip_uncompress(buf, recvbytes, file_len);

					if(uncompr_buf == NULL) {
						free(buf);
						return -1;
					}

					free(buf);
				}
			}
#endif
		}
	}

	if(!(tmp = (char*)calloc((strlen(filepath) + 1), sizeof(char)))) {
		printf("Could not alloc memory for tmp (filepath)!\n");
		fflush(stdout);
		
		if(big_file_mode) {
			free(tmp_file_name);		
		}
		else if(toi == FDT_TOI) {
#ifdef USE_ZLIB
			if(fdt_cont_enc_algo == ZLIB) {
				free(uncompr_buf);
			}
			else {
				free(buf);
			}
#else
			free(buf);
#endif
		}
		else {

#ifdef USE_ZLIB
			if(encoding != NULL) {
				if(strcmp(encoding, "gzip") == 0) {
					free(uncompr_buf);
				}
			}
			else {
				free(buf);
			}
#else
			free(buf);
#endif
		}

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
	      
					if(big_file_mode) {
                        free(tmp_file_name);
                	}
					else if(toi == FDT_TOI) {
#ifdef USE_ZLIB
						if(fdt_cont_enc_algo == ZLIB) {
							free(uncompr_buf);
						}
						else {
							free(buf);
						}
#else
						free(buf);
#endif
					}
                	else {
#ifdef USE_ZLIB
						if(encoding != NULL) {
							if(strcmp(encoding, "gzip") == 0) {
								free(uncompr_buf);
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

	if(((!big_file_mode) || (toi == FDT_TOI))) {
														
#ifdef WIN32
		if((fd = open((const char*)fullpath, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC , _S_IWRITE)) < 0) {
#else
		if((fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC , S_IRWXU)) < 0) {
#endif
			printf("Error: unable to open file %s\n", fullpath);
			fflush(stdout);

			free(tmp);

#ifdef USE_ZLIB
			if(toi == FDT_TOI) {
				if(fdt_cont_enc_algo == ZLIB) {
					free(uncompr_buf);
				}
				else {
					free(buf);
				}
			}
			else if(encoding != NULL) {
				if(strcmp(encoding, "gzip") == 0) {
					free(uncompr_buf);
				}
				else {
					free(buf);
				}
			}
#else
			free(buf);
#endif
			return -1;
		}

#ifdef USE_ZLIB
		if(toi == FDT_TOI) {
			if(fdt_cont_enc_algo == ZLIB) {
				write(fd, uncompr_buf, (unsigned int)uncompr_buflen);
				free(uncompr_buf);
			}
			else {
				write(fd, buf, (unsigned int)recvbytes);
				free(buf);
			}
		}
		else if(encoding != NULL) {
			if(strcmp(encoding, "gzip") == 0) {
				write(fd, uncompr_buf, (unsigned int)file_len);
				free(uncompr_buf);
			}
		}
		else {
			write(fd, buf, (unsigned int)recvbytes);
			free(buf);
		}
#else
		write(fd, buf, (unsigned int)recvbytes);
		free(buf);
#endif

		free(tmp);
		close(fd);

#ifdef WIN32
		printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
		printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif
		fflush(stdout);
	}
	
	if(((big_file_mode) && (toi != FDT_TOI))) {
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
                free(tmp);

#ifdef WIN32
                printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
                printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif
		fflush(stdout);
	}

	return 1;
}

/*
 * This function receives files defined in an FDT file using recvfile() function.
 *
 * Params:	fdt_t *fdt: Pointer to FDT structure,
 *			int s_id: Session identifier,
 *			int openfile: Open file automatically (1 = yes, 0 = no) [only in Windows]
 *
 * Return:	int: Different values (1, 0, -1, -2, -3) depending on how function ends
 *
 * Not used in flute anymore
 */

int fdtbasedrecv(fdt_t *fdt, int s_id, int openfile) {

	file_t *file;
	file_t *next_file;
	char *filepath;
	int retval;
	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

	int printed = 0;

	time(&systime);
	curr_time = systime + 2208988800;

	next_file = fdt->file_list;

	while(next_file != NULL) {
		file = next_file;

		if(file->expires < curr_time) {

			if(file->next != NULL) {
				file->next->prev = file->prev;
			}
			if(file->prev != NULL) {
				file->prev->next = file->next;
			}
			if(file == fdt->file_list) {
				fdt->file_list = file->next;
			}

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

			continue;
		}
		
		next_file = file->next;
	}

	file = fdt->file_list;

	/*while(file != NULL) {*/

	while(1) {

		if(get_session_state(s_id) == SExiting) {
			return -2;
		}
		else if(get_session_state(s_id) == STxStopped) {
			return -3;
		}

		if(file->is_downloaded == 1) {

			if(!printed) {
				printf("4 All files received, waiting for new files\n");
				fflush(stdout);
				printed = 1;
			}

			if(file->next != NULL) {
				file = file->next;
				printed = 0;
				continue;
			}
			else {
#ifdef WIN32
				Sleep(1000);
#else
				sleep(1);
#endif
				continue;
			}
		}

		if(!(filepath = (char*)calloc((strlen(file->location) + 1), sizeof(char)))) {
			printf("Could not alloc memory for tmp (file location)!\n");
			return -1;
		}

		memcpy(filepath, file->location, strlen(file->location));
														
		retval = recvfile(s_id, filepath, file->toi, file->md5, 0
#ifdef USE_ZLIB
						  , file->encoding, file->file_len
#endif
						  );
				
		if(retval <= 0) {
			free(filepath);
			return retval;
		}

		file->is_downloaded = 1;

#ifdef WIN32
		if(openfile) {
			ShellExecute(NULL, "Open", filepath, NULL, NULL, SW_SHOWNORMAL); 
		}
#endif
		free(filepath);
	
		if(get_session_state(s_id) == STxStopped) {
			return -3;
		}

		if(file->next != NULL) {
			file = file->next;
		}
	}

	return 1;
}


/*
 * This function receives files defined in an FDT file using alc_recv2() function.
 *
 * Params:	fdt_t *fdt: Pointer to FDT structure,
 *			int s_id: Session identifier,
 *			int openfile: Open file automatically (1 = yes, 0 = no) [only in Windows],
 *			int accept_expired_files: Accept Expired files.
 *
 * Return:	int: Different values (1, 0, -1, -2, -3) depending on how function ends
 *
 */

int fdtbasedrecv2(fdt_t *fdt, int s_id, int openfile, int accept_expired_files) {

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
	int retcode;

	char *ptr;
	int point; 
	int ch = '/';
	int i;

	char *tmp = NULL;
	char fullpath[MAX_PATH];
	char filename[MAX_PATH];
	char *filepath = NULL;
	uri_t *uri = NULL;

	bool is_all_files_received;
	bool is_printed = false;

#ifdef USE_ZLIB
	char *uncomprBuf = NULL;
#endif

#ifdef USE_OPENSSL
	char *md5 = NULL;
#endif

	char* session_basedir = get_session_basedir(s_id);

	while(1) {

		if(get_session_state(s_id) == SExiting) {
			return -2;
		}
		else if(get_session_state(s_id) == STxStopped) {
			return -3;
		}

		is_all_files_received = true;

		time(&systime);
		curr_time = systime + 2208988800;

		next_file = fdt->file_list;

		while(next_file != NULL) {
			file = next_file;

			if(((file->expires < curr_time) && (accept_expired_files == 0))) {

				if(file->next != NULL) {
					file->next->prev = file->prev;
				}
				if(file->prev != NULL) {
					file->prev->next = file->next;
				}
				if(file == fdt->file_list) {
					fdt->file_list = file->next;
				}

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

				continue;
			}

			if(file->is_downloaded != 2) {

				is_all_files_received = false;
				is_printed = false;
			}	
			
			next_file = file->next;
		}

		i = 0;
		toilen = 0;
		toi = 0;

		if(is_all_files_received) {
			
			if(!is_printed) {
				printf("8 All files received, waiting for new files\n\n");
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

		buf = alc_recv2(s_id, &toi, &toilen, &retcode);

		if(buf == NULL) {
			/*printf("alc_recv2() recvfile ret <= 0\n");
			fflush(stdout);*/
			return retcode;
		}

		next_file = fdt->file_list;

		/* Find correct file structure, to get the file->location for file creation purpose */

		while(next_file != NULL) {
			file = next_file;

			if(file->toi == toi) {
				file->is_downloaded = 2;
				break;
			}

			next_file = file->next;
		}

#ifdef USE_OPENSSL
		md5 = buffer_md5(buf, toilen);

		if(md5 == NULL) {
#ifdef WIN32
			printf("MD5 check failed (TOI=%I64u)!\n\n", file->toi);
#else
			printf("MD5 check failed (TOI=%llu)!\n\n", file->toi);
#endif
			fflush(stdout);

			free(buf);

			file->is_downloaded = 1;

#ifdef USE_ZLIB
			if(file->encoding == NULL) {
				set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
								 file->max_source_block_length,
								file->fec_instance_id,
								file->fec_encoding_id,
								file->max_nb_of_encoding_symbols, 0);
			}
			else {
				set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
								 file->max_source_block_length,
								file->fec_instance_id,
								file->fec_encoding_id,
								file->max_nb_of_encoding_symbols, GZIP);
			}
#else
			set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
								 file->max_source_block_length,
								file->fec_instance_id,
								file->fec_encoding_id,
								file->max_nb_of_encoding_symbols);
#endif

			continue;
		}
		else{
			if(strcmp(md5, file->md5) != 0) {
#ifdef WIN32
				printf("MD5 check failed (TOI=%I64u)!\n\n", file->toi);
#else
				printf("MD5 check failed (TOI=%llu)!\n\n", file->toi);
#endif
				fflush(stdout);

				free(buf);
				free(md5);
				file->is_downloaded = 1;
#ifdef USE_ZLIB
				if(file->encoding == NULL) {
					set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									file->fec_instance_id, file->fec_encoding_id,
									file->max_nb_of_encoding_symbols, 0);
				}
				else {
					set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									file->fec_instance_id, file->fec_encoding_id,
									file->max_nb_of_encoding_symbols, GZIP);
				}
#else
				set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
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
	
		uri = parse_uri(file->location, strlen(file->location));
	
		filepath = get_uri_host_and_path(uri);

		if(!(tmp = (char*)calloc((strlen(filepath) + 1), sizeof(char)))) {
			printf("Could not alloc memory for tmp (file location)!\n");

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
			free_uri(uri);
			free(filepath);
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
    	if(openfile) {
			ShellExecute(NULL, "Open", fullpath, NULL, NULL, SW_SHOWNORMAL);
    	}
#endif

		free(filepath);

		if(get_session_state(s_id) == STxStopped) {
			return -3;
		}

#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}
     
	return 1;
}

/*
 * This function receives files defined in an FDT file using alc_recv3() function.
 *
 * Params:	fdt_t *fdt: Pointer to FDT structure,
 *			int s_id: Session identifier,
 *			int openfile: Open file automatically (1 = yes, 0 = no) [only in Windows],
 *			int accept_expired_files: Accept Expired files.
 *
 * Return:	int: Different values (1, 0, -1, -2, -3) depending on how function ends
 *
 */

int fdtbasedrecv3(fdt_t *fdt, int s_id, int openfile, int accept_expired_files) {
  	
	file_t *file;
  	file_t *next_file;
	uri_t *uri;
  
  	time_t systime;

#ifdef WIN32
	ULONGLONG curr_time;
#else
	unsigned long long curr_time;
#endif

#ifdef WIN32  
  	ULONGLONG toi;
#else
	unsigned long long toi;
#endif
  
  	char *ptr;
 	int point; 
  	int ch = '/';
  	int i;
  
  	char *tmp = NULL;
  
	char fullpath[MAX_PATH];
  	char filename[MAX_PATH];

  	char *filepath = NULL;
  
  	bool is_all_files_received;
  	bool is_printed = false;
  
  	int retcode;
	int retval;

  	char *tmp_filename;

#ifdef USE_OPENSSL
	char *md5= NULL;
#endif

#ifdef USE_ZLIB
	/*unsigned short cont_enc_algo;*/
	struct stat	file_stats;
#endif

	char* session_basedir = get_session_basedir(s_id);

  	while(1) {
    
    	if(get_session_state(s_id) == SExiting) {
      		return -2;
    	}
    	else if(get_session_state(s_id) == STxStopped) {
      		return -3;
    	}

    	is_all_files_received = true;

    	time(&systime);
		curr_time = systime + 2208988800;

    	next_file = fdt->file_list;

    	while(next_file != NULL) {
      		file = next_file;
  
      		if(((file->expires < curr_time) && (accept_expired_files == 0))) {

				if(file->next != NULL) {
	  				file->next->prev = file->prev;
				}

				if(file->prev != NULL) {
	  				file->prev->next = file->next;
				}
	
				if(file == fdt->file_list) {
	  				fdt->file_list = file->next;
				}
	
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
	
				continue;
      		}
  
      		if(file->is_downloaded != 2) {

				am_new_file(file->location, file->toi);
				is_all_files_received = false;
				is_printed = false;
      		}	
  
      		next_file = file->next;
    	}
    
    	i = 0;
   		retcode = 0;
    	toi = 0;

    	if(is_all_files_received) {
  
      		if(!is_printed) {
				am_status("Waiting for new files...");
//				printf("All files received, waiting for new files\n\n");
//				fflush(stdout);
				is_printed = true;
      		}
  
#ifdef WIN32
      		Sleep(1);
#else
      		usleep(1000);
#endif
      		continue;
    	}

	am_status("Receiving...");
    	tmp_filename = alc_recv3(s_id, &toi, &retcode);

    	if(tmp_filename == NULL) {
      		return retcode;
    	}
    
    	next_file = fdt->file_list;

    	/* Find correct file structure, to get the file->location for file creation purpose */

   		while(next_file != NULL) {
      		file = next_file;
  
      		if(file->toi == toi) {
				file->is_downloaded = 2;
				break;
      		}
  
      		next_file = file->next;
    	}

#ifdef USE_OPENSSL
		if(file->md5 != NULL) {

			md5 = file_md5(tmp_filename);

			if(md5 == NULL) {

#ifdef WIN32
				printf("MD5 check failed (TOI=%I64u)!\n\n", file->toi);
#else
				printf("MD5 check failed (TOI=%llu)!\n\n", file->toi);
#endif
				fflush(stdout);
				remove(tmp_filename);
				free(tmp_filename);
				file->is_downloaded = 1;

#ifdef USE_ZLIB
				if(file->encoding == NULL) {
					set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									 file->fec_instance_id, file->fec_encoding_id,
									 file->max_nb_of_encoding_symbols, 0);
				}
				else {
					set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									file->fec_instance_id, file->fec_encoding_id,
									file->max_nb_of_encoding_symbols, GZIP);
				}
#else
				set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
									 file->max_source_block_length,
									file->fec_instance_id,
									file->fec_encoding_id,
									file->max_nb_of_encoding_symbols);
#endif
				continue;
			}
			else{
				if(strcmp(md5, file->md5) != 0) {

#ifdef WIN32
					printf("MD5 check failed (TOI=%I64u)!\n\n", file->toi);
#else
					printf("MD5 check failed (TOI=%llu)!\n\n", file->toi);
#endif
					fflush(stdout);
					/*remove(tmp_filename);*/
					free(tmp_filename);
					free(md5);
					file->is_downloaded = 1;

					return -1;
#ifdef USE_ZLIB
					if(file->encoding == NULL) {
						set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										 file->max_source_block_length,
										file->fec_instance_id, file->fec_encoding_id,
										file->max_nb_of_encoding_symbols, 0);
					}
					else {
						set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										 file->max_source_block_length,
										file->fec_instance_id, file->fec_encoding_id,
										file->max_nb_of_encoding_symbols, GZIP);
					}
#else
					set_wanted_object(s_id, file->toi, file->toi_len, file->encoding_symbol_length,
										 file->max_source_block_length,
										file->fec_instance_id,
										file->fec_encoding_id,
										file->max_nb_of_encoding_symbols);
#endif
					continue;
				}

				free(md5);
			}
		}
#endif

#ifdef USE_ZLIB

		if(file->encoding != NULL) {

			if(strcmp(file->encoding, "gzip") == 0) {
				
				retcode = file_gzip_uncompress(tmp_filename);
			
				if(retcode == -1) {
					free(tmp_filename);
					return -1;
				}

				*(tmp_filename + (strlen(tmp_filename) - GZ_SUFFIX_LEN)) = '\0';

				if(stat(tmp_filename, &file_stats) == -1) {
					printf("Error: %s is not valid file name\n", tmp_filename);
					fflush(stdout);
					free(tmp_filename);
					return -1;
				}

				if(file_stats.st_size != file->file_len) {
					printf("Error: uncompression failed, file-size not ok.\n");
					fflush(stdout);
					remove(tmp_filename);
					free(tmp_filename);
					return -1;
				}
			}
		}
#endif
		am_progress(100.0, file->toi);

		uri = parse_uri(file->location, strlen(file->location));
	
		filepath = get_uri_host_and_path(uri);

    	if(!(tmp = (char*)calloc((strlen(filepath) + 1), sizeof(char)))) {
      		printf("Could not alloc memory for tmp-filepath!\n");
      		fflush(stdout);
			free(tmp_filename);
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
						free(tmp_filename);
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

		if(rename(tmp_filename, fullpath) < 0) {

			if(errno == EEXIST) {

				retval = remove(fullpath);

				if(retval == -1) {
				
					printf("errno: %i\n", errno);
					fflush(stdout);
				}

				if(rename(tmp_filename, fullpath) < 0) {
					printf("rename() error1: %s\n", tmp_filename);
					fflush(stdout);
				}
			}
			else {
				printf("rename() error2: %s\n", tmp_filename);
				fflush(stdout);
			}
		}
		am_file_done(fullpath, file->toi);

			
        free(tmp_filename);
        free(tmp);

#ifdef WIN32
		printf("File: %s received (TOI=%I64u)\n\n", filepath, toi);
#else
		printf("File: %s received (TOI=%llu)\n\n", filepath, toi);
#endif
		fflush(stdout);
		free_uri(uri);
                
#ifdef WIN32
    	if(openfile) {
			
      		ShellExecute(NULL, "Open", fullpath, NULL, NULL, SW_SHOWNORMAL);
    	}
#endif
    	free(filepath);

    	if(get_session_state(s_id) == STxStopped) {
      		return -3;
    	}
    
#ifdef WIN32
    	Sleep(1);
#else
    	usleep(1000);
#endif
  	}

  	return 1;
}
