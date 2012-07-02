/* $Author: peltotal $ $Date: 2004/10/06 07:05:00 $ $Revision: 1.26 $ */
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

#include "uri.h"
#include "mad_md5.h"
#include "getdnsname.h"
#include "mad_zlib.h"
#include "fdt_gen.h"

/* Global variable */
#ifdef WIN32
static ULONGLONG toi = 1;
#else
static unsigned long long toi = 1;
#endif

/* Local functions */

static int checkpath(const char *path);
static int encode_directory(char *directory, char *base_dir, FILE *fp, int *s_id);
static int encode_file(char *file, char *base_dir, FILE *fp, int *s_id);

/* FIXME describe this function */
int generate_fdt_from_list(file_info_t *list, int *s_id, char *fdt_file_name)
{

	FILE *fp;
	alc_session_t *s = NULL;
	file_info_t *info;

#ifdef FDT_INST_FEC_OTI_COMMON
	div_t div_max_n;
	int max_n;
#endif

	if((fp = fopen(fdt_file_name, "wb")) == NULL) {
		printf("Error: unable to create fdtfile %s\n", fdt_file_name);
		fflush(stdout);
		return -1;
	}

	s = get_alc_session(*s_id);

#ifdef FDT_INST_FEC_OTI_COMMON
	div_max_n = div((s->def_max_sblen * (100 + s->def_fec_ratio)), 100);
	max_n = div_max_n.quot;
#endif

	fprintf(fp, "<?xml version=\"1.0\"?>\n");
	fprintf(fp, "<FDT-Instance ");

#ifdef WIN32
	if(s->stoptime == 0) {
		/* session is not bounded, set 64bits max value */
		fprintf(fp, "Expires=\"%I64u\"", 0xFFFFFFFFFFFFFFFF);
	}
	else {
		fprintf(fp, "Expires=\"%I64u\"", s->stoptime);
	}
#else
	if(s->stoptime == 0) {
		/* session is not bounded, set 64bits max value */
		fprintf(fp, "Expires=\"%llu\"", 0xFFFFFFFFFFFFFFFF);
	}
	else {
		fprintf(fp, "Expires=\"%llu\"", s->stoptime);
	}
#endif

#ifdef FDT_INST_FEC_OTI_COMMON
	if(s->use_fec_oti_ext_hdr == 0) {

		fprintf(fp, "\n\t");
		fprintf(fp, "FEC-OTI-FEC-Encoding-ID=\"%u\"", s->def_fec_enc_id);

		if(s->def_fec_enc_id >= 128) {
			fprintf(fp, "\n\t");
			fprintf(fp, "FEC-OTI-FEC-Instance-ID=\"%u\"", s->def_fec_inst_id);
		}

		fprintf(fp, "\n\t");
		fprintf(fp, "FEC-OTI-Maximum-Source-Block-Length=\"%u\"", s->def_max_sblen);
		fprintf(fp, "\n\t");
		fprintf(fp, "FEC-OTI-Encoding-Symbol-Length=\"%u\"", s->def_eslen);

		if(s->def_fec_enc_id == SB_SYS_FEC_ENC_ID) {
			fprintf(fp, "\n\t");
			fprintf(fp, "FEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"", max_n);	
		}
	}
#endif

	fprintf(fp, ">\n");

	for (info = list; info != NULL; info= info->next) {
		char fullpath[1024] = "/";
		struct stat file_stats;
		toi = info->toi;

		strcat(fullpath, info->location);
		if(stat(fullpath, &file_stats) == -1) {
			printf("Error: %s is not valid file name\n", fullpath);
			continue;
		}		
		if(file_stats.st_mode & S_IFREG) {
			encode_file(info->location, "/", fp, s_id);
		}
	}

	fprintf(fp, "</FDT-Instance>\n");
	fclose(fp);

//	printf("File: %s created\n", fdt_file_name);

	return 0;
}

/*
 * This function generates fdt file.
 *
 * Params:	char *file_path: Pointer to buffer containing file or directory to be parsed to the fdt,
 *			char *base_dir: Pointer to buffer containing base directory for file or directory to be
 *							parsed to the fdt,
 *			int *s_id: Pointer to session identifier,
 *			char *fdt_file_name: Pointer to buffer containing file name to fdt.
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */

int generate_fdt(char *file_path, char *base_dir, int *s_id, char *fdt_file_name) {

	int result;
	FILE *fp;
	struct stat file_stats;
	alc_session_t *s = NULL;
	char fullpath[MAX_PATH + MAX_LENGTH];

#ifdef FDT_INST_FEC_OTI_COMMON
	div_t div_max_n;
	int max_n;
#endif

	memset(fullpath, 0, (MAX_PATH + MAX_LENGTH));
	
	if(!(strcmp(base_dir, "") == 0)) {
		strcpy(fullpath, base_dir);

#ifdef WIN32
		strcat(fullpath, "\\");
#else
		strcat(fullpath, "/");
#endif
	}
	
	strcat(fullpath, file_path);

	if(stat(fullpath, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", fullpath);
		fflush(stdout);
		return -1;
	}		

	if((fp = fopen(fdt_file_name, "wb")) == NULL) {
		printf("Error: unable to create fdtfile %s\n", fdt_file_name);
		fflush(stdout);
		return -1;
	}

	s = get_alc_session(*s_id);

#ifdef FDT_INST_FEC_OTI_COMMON
	div_max_n = div((s->def_max_sblen * (100 + s->def_fec_ratio)), 100);
	max_n = div_max_n.quot;
#endif

	fprintf(fp, "<?xml version=\"1.0\"?>\n");
	fprintf(fp, "<FDT-Instance ");

#ifdef WIN32
	if(s->stoptime == 0) {
		/* session is not bounded, set 64bits max value */
		fprintf(fp, "Expires=\"%I64u\"", 0xFFFFFFFFFFFFFFFF);
	}
	else {
		fprintf(fp, "Expires=\"%I64u\"", s->stoptime);
	}
#else
	if(s->stoptime == 0) {
		/* session is not bounded, set 64bits max value */
		fprintf(fp, "Expires=\"%llu\"", 0xFFFFFFFFFFFFFFFF);
	}
	else {
		fprintf(fp, "Expires=\"%llu\"", s->stoptime);
	}
#endif

#ifdef FDT_INST_FEC_OTI_COMMON
	if(s->use_fec_oti_ext_hdr == 0) {

		fprintf(fp, "\n\t");
		fprintf(fp, "FEC-OTI-FEC-Encoding-ID=\"%u\"", s->def_fec_enc_id);

		if(s->def_fec_enc_id >= 128) {
			fprintf(fp, "\n\t");
			fprintf(fp, "FEC-OTI-FEC-Instance-ID=\"%u\"", s->def_fec_inst_id);
		}

		fprintf(fp, "\n\t");
		fprintf(fp, "FEC-OTI-Maximum-Source-Block-Length=\"%u\"", s->def_max_sblen);
		fprintf(fp, "\n\t");
		fprintf(fp, "FEC-OTI-Encoding-Symbol-Length=\"%u\"", s->def_eslen);

		if(s->def_fec_enc_id == SB_SYS_FEC_ENC_ID) {
			fprintf(fp, "\n\t");
			fprintf(fp, "FEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"", max_n);	
		}
	}
#endif

	fprintf(fp, ">\n");

	if(file_stats.st_mode & S_IFDIR) {
		result = encode_directory(file_path, base_dir, fp, s_id);
	}
	else {
		result = encode_file(file_path, base_dir, fp, s_id);
	}

	if(result < 0) {
		fclose(fp);
		remove(fdt_file_name);
		return -1;
	}
	else {
		fprintf(fp, "</FDT-Instance>\n");

		printf("File: %s created\n", fdt_file_name);
	}

	fclose(fp);

	return 0;
}

/*
 * This function encodes directory to fdt.
 *
 * Params:	char *directory: Pointer to buffer containing directory to be encoded,
 *			char *base_dir: Pointer to buffer containing base directory for file
 *							or directory to be parsed to the fdt,
 *			FILE *fp: File pointer,
 *			int *s_id: Pointer to session identifier.
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */

int encode_directory(char *directory, char *base_dir, FILE *fp, int *s_id) {

	int result;
	char fullname[MAX_PATH + MAX_LENGTH];
	char fullpath[MAX_PATH + MAX_LENGTH];
	struct stat file_stats;

	uri_t *uri = NULL;
	char *hostname = NULL;
	alc_session_t *s = NULL;
	char *user = NULL;

#ifdef USE_ZLIB
	int retcode;
	char enc_fullpath[MAX_PATH + MAX_LENGTH];
	struct stat enc_file_stats;
#endif

#ifdef USE_OPENSSL
	char *md5 = NULL;
#endif

#ifdef WIN32
	int i;
	char findfile[MAX_PATH + 3];
	HANDLE dirptr = NULL;
	WIN32_FIND_DATA entry;
#else
	struct dirent *entry;
	DIR *dirptr = NULL;
	char findfile[MAX_PATH];
#endif

#ifdef FDT_INST_FEC_OTI_FILE
	div_t div_max_n;
	int max_n;
#endif

	char *uri_str = NULL;

	hostname = getdnsname();

	s = get_alc_session(*s_id);

#ifdef FDT_INST_FEC_OTI_FILE
	div_max_n = div((s->def_max_sblen * (100 + s->def_fec_ratio)), 100);
	max_n = div_max_n.quot;
#endif

#ifdef WIN32
	user = getenv("USERNAME");

	memset(findfile, 0, (MAX_PATH + 3));

	if(!(strcmp(base_dir, "") == 0)) {
		strcpy(findfile, base_dir);
		strcat(findfile, "\\");
	}

	strcat(findfile, directory);
	strcat(findfile, "\\*");
	
	dirptr = FindFirstFile(findfile, &entry);

	if(dirptr == INVALID_HANDLE_VALUE) {
		printf("Error: %s is not valid directory name\n", directory);
		fflush(stdout);
		free(hostname);
		return -1;
	}

	if(!checkpath(entry.cFileName)) {

		memset(fullname, 0 , (MAX_PATH + MAX_LENGTH));
		strcpy(fullname, directory);

		if(fullname[strlen(fullname) - 1] != '\\') {
			strcat(fullname, "\\");
		}

		strcat(fullname, entry.cFileName);

		for(i = 0; i < (int)strlen(fullname); i++) {
			if(fullname[i] == '\\') {
				fullname[i] = '/';
			}
		}

		if(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			result = encode_directory(fullname, base_dir, fp, s_id);

			if(result < 0) {
				free(hostname);
				return -1;
			}
		}
		else {

			memset(fullpath, 0, MAX_PATH);

			if(!(strcmp(base_dir, "") == 0)) {
				strcpy(fullpath, base_dir);
				strcat(fullpath, "\\");
			}

			strcat(fullpath, fullname);

			if(stat(fullpath, &file_stats) == -1) {
				printf("Error: %s is not valid file name\n", fullpath);
				fflush(stdout);
				free(hostname);
				return -1;
			}		

			if(file_stats.st_size == 0) {
				printf("Error: file %s size = 0\n", fullpath);
				fflush(stdout);
				free(hostname);
				return -1;
			}
			
			uri = alloc_uri_struct();

			set_uri_scheme(uri, "file");

			if(user != NULL) {
				set_uri_user(uri, user);
			}

			if(hostname != NULL) {
				set_uri_host(uri, hostname);
			}

			set_uri_path(uri, fullname);

			fprintf(fp, "\t<File TOI=\"%I64u\"", toi);
			
			uri_str = uri_string(uri);

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-Location=\"%s\"", uri_str);

			free(uri_str);
			uri_str = NULL;

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-Length=\"%lu\"", file_stats.st_size);

#ifdef USE_ZLIB
			if(s->encode_content == ENCODE_FILES) {


				retcode = file_gzip_compress(fullpath, "wb");

				if(retcode == 0) {
					
					memset(enc_fullpath, 0 , (MAX_PATH + MAX_LENGTH));
					strcpy(enc_fullpath, fullpath);
					strcat(enc_fullpath, GZ_SUFFIX);

					if(stat(enc_fullpath, &enc_file_stats) == -1) {
						printf("Error: %s is not valid file name\n", enc_fullpath);
						fflush(stdout);
						free(hostname);
						free_uri(uri);
						return -1;
					}
					
					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-Encoding=\"%s\"", "gzip");
				
#ifdef USE_OPENSSL
					md5 = file_md5(enc_fullpath);

					if(md5 == NULL) {
						free(hostname);
						free_uri(uri);
						return -1;
					}

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Transfer-Length=\"%lu\"", enc_file_stats.st_size);
				}
			}
			else {

#ifdef USE_OPENSSL
				md5 = file_md5(fullpath);

				if(md5 == NULL) {
					free(hostname);
					free_uri(uri);
					return -1;
				}

				fprintf(fp, "\n\t\t");
				fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif
			}
#else

#ifdef USE_OPENSSL
			md5 = file_md5(fullpath);

			if(md5 == NULL) {
				free(hostname);
				free_uri(uri);
				return -1;
			}

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif

#endif

#ifdef FDT_INST_FEC_OTI_FILE
			if(s->use_fec_oti_ext_hdr == 0) {

#ifndef FDT_SCHEMA_FLUTEv07
				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-FEC-Encoding-ID=\"%u\"", s->def_fec_enc_id);
#endif

				if(s->def_fec_enc_id >= 128) {
					fprintf(fp, "\n\t\t");
					fprintf(fp, "FEC-OTI-FEC-Instance-ID=\"%u\"", s->def_fec_inst_id);
				}

				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-Maximum-Source-Block-Length=\"%u\"", s->def_max_sblen);
				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-Encoding-Symbol-Length=\"%u\"", s->def_eslen);

				if(s->def_fec_enc_id == SB_SYS_FEC_ENC_ID) {
					fprintf(fp, "\n\t\t");
					fprintf(fp, "FEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"", max_n);	
				}
			}
#endif

			fprintf(fp, "/>\n");
			
			toi++;
			free_uri(uri);

#ifdef USE_OPENSSL
			free(md5);
			md5 = NULL;
#endif
		}
	}

	while(FindNextFile(dirptr, &entry)) {

		if(checkpath(entry.cFileName)) {
			continue;
		}

		memset(fullname, 0 , MAX_PATH);
		strcpy(fullname, directory);

		if(fullname[strlen(fullname) - 1] != '\\') {
			strcat(fullname, "\\");
		}

		strcat(fullname, entry.cFileName);

		for(i = 0; i < (int)strlen(fullname); i++) {
			if(fullname[i] == '\\') {
				fullname[i] = '/';
			}
		}

		if(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			result = encode_directory(fullname, base_dir, fp, s_id);

			if(result < 0) {
				continue;
			}
		}
		else {
	
			memset(fullpath, 0, MAX_PATH);

			if(!(strcmp(base_dir, "") == 0)) {
				strcpy(fullpath, base_dir);
				strcat(fullpath, "\\");
			}

			strcat(fullpath, fullname);

			if(stat(fullpath, &file_stats) == -1) {
				printf("Error: %s is not valid file name\n", fullpath);
				fflush(stdout);
				continue;
			}

			if(file_stats.st_size == 0) {
				printf("Error: file %s size = 0\n", fullpath);
				fflush(stdout);
				continue;
			}

			uri = alloc_uri_struct();

			set_uri_scheme(uri, "file");

			if(user != NULL) {
				set_uri_user(uri, user);
			}
			
			if(hostname != NULL) {
				set_uri_host(uri, hostname);
			}

			set_uri_path(uri, fullname);

            fprintf(fp, "\t<File TOI=\"%I64u\"", toi);

			uri_str = uri_string(uri);

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-Location=\"%s\"", uri_str);

			free(uri_str);
			uri_str = NULL;

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-Length=\"%lu\"", file_stats.st_size);

#ifdef USE_ZLIB
			if(s->encode_content == ENCODE_FILES) {

				retcode = file_gzip_compress(fullpath, "wb");
			
				if(retcode == 0) {

					memset(enc_fullpath, 0 , MAX_PATH);
					strcpy(enc_fullpath, fullpath);
					strcat(enc_fullpath, GZ_SUFFIX);

					if(stat(enc_fullpath, &enc_file_stats) == -1) {
						printf("Error: %s is not valid file name\n", enc_fullpath);
						fflush(stdout);
						free_uri(uri);
						continue;
					}
					
					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-Encoding=\"%s\"", "gzip");

#ifdef USE_OPENSSL
					md5 = file_md5(enc_fullpath);

					if(md5 == NULL) {
						free(hostname);
						free_uri(uri);
						continue;
					}

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Transfer-Length=\"%lu\"", enc_file_stats.st_size);
				}
			}
			else {

#ifdef USE_OPENSSL
				md5 = file_md5(fullpath);

				if(md5 == NULL) {
					free(hostname);
					free_uri(uri);
					continue;
				}

				fprintf(fp, "\n\t\t");
				fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif
			}
#else

#ifdef USE_OPENSSL
			md5 = file_md5(fullpath);

			if(md5 == NULL) {
				free(hostname);
				free_uri(uri);
				continue;
			}

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif

#endif
			
#ifdef FDT_INST_FEC_OTI_FILE
			if(s->use_fec_oti_ext_hdr == 0) {

#ifndef FDT_SCHEMA_FLUTEv07
				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-FEC-Encoding-ID=\"%u\"", s->def_fec_enc_id);
#endif

				if(s->def_fec_enc_id >= 128) {
					fprintf(fp, "\n\t\t");
					fprintf(fp, "FEC-OTI-FEC-Instance-ID=\"%u\"", s->def_fec_inst_id);
				}

				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-Maximum-Source-Block-Length=\"%u\"", s->def_max_sblen);
				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-Encoding-Symbol-Length=\"%u\"", s->def_eslen);
				
				if(s->def_fec_enc_id == SB_SYS_FEC_ENC_ID) {
					fprintf(fp, "\n\t\t");
					fprintf(fp, "FEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"", max_n);	
				}
			}
#endif

			fprintf(fp, "/>\n");
			
			toi++;
			free_uri(uri);

#ifdef USE_OPENSSL
			free(md5);
			md5 = NULL;
#endif
		}
	}
	FindClose(dirptr);

#else
	user = getenv("USER");	

	memset(findfile, 0, MAX_PATH);

	if(!(strcmp(base_dir, "") == 0)) {
		strcpy(findfile, base_dir);
		strcat(findfile, "/");
	}

	strcat(findfile, directory);

	dirptr = opendir(findfile);

	if(dirptr == NULL) {
		printf("%s is not valid directory name\n", findfile);
		fflush(stdout);
		free(hostname);
		return -1;
	}

	entry = readdir(dirptr);

	while(entry != NULL) {
		
		if(checkpath(entry->d_name)) {
			entry = readdir(dirptr);
			continue;
		}

		memset(fullname, 0 , MAX_PATH);
		strcpy(fullname, directory);
		
		if(fullname[strlen(fullname) - 1] != '/') {
			strcat(fullname, "/");
		}

		strcat(fullname, entry->d_name);

		memset(fullpath, 0, MAX_PATH);

		if(!(strcmp(base_dir, "") == 0)) {
			strcpy(fullpath, base_dir);
			strcat(fullpath, "/");
		}

		strcat(fullpath, fullname);

		if(stat(fullpath, &file_stats) == -1) {
			printf("Error: %s is not valid file name\n", fullpath);
			fflush(stdout);
			entry = readdir(dirptr);
			continue;
		}

		if(S_ISDIR(file_stats.st_mode)) {
			result = encode_directory(fullname, base_dir, fp, s_id);

			if(result < 0) {
				entry = readdir(dirptr);
				continue;
			}
		}
		else {	
			if(file_stats.st_size == 0) {
				printf("Error: file %s size = 0\n", fullpath);
				fflush(stdout);
				entry = readdir(dirptr);
				continue;
			}

			uri = alloc_uri_struct();

			set_uri_scheme(uri, "file");

			if(user != NULL) {
				set_uri_user(uri, user);
			}
			
			if(hostname != NULL) {
				set_uri_host(uri, hostname);
			}

			set_uri_path(uri, fullname);

            fprintf(fp, "\t<File TOI=\"%llu\"", toi);

			uri_str = uri_string(uri);

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-Location=\"%s\"", uri_str);

			free(uri_str);
			uri_str = NULL;

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-Length=\"%lu\"", file_stats.st_size);

#ifdef USE_ZLIB
			if(s->encode_content == ENCODE_FILES) {
                                                                                                                                              
				retcode = file_gzip_compress(fullpath, "wb");
                                                                                                                                          
				if(retcode == 0) {
					
					memset(enc_fullpath, 0 , MAX_PATH);
					strcpy(enc_fullpath, fullpath);
					strcat(enc_fullpath, GZ_SUFFIX);
                                                                                                                                      
					if(stat(enc_fullpath, &enc_file_stats) == -1) {
						printf("Error: %s is not valid file name\n", enc_fullpath);
						fflush(stdout);
						entry = readdir(dirptr);
						free_uri(uri);
						continue;
					}

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-Encoding=\"%s\"", "gzip");
					
#ifdef USE_OPENSSL
					md5 = file_md5(enc_fullpath);

					if(md5 == NULL) {
						free_uri(uri);
						continue;
					}

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif
                
					fprintf(fp, "\n\t\t");
					fprintf(fp, "Transfer-Length=\"%lu\"", enc_file_stats.st_size);
				}
			}
			else {

#ifdef USE_OPENSSL
				md5 = file_md5(fullpath);

				if(md5 == NULL) {
					free_uri(uri);
					continue;
				}

				fprintf(fp, "\n\t\t");
				fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif
			}
#else

#ifdef USE_OPENSSL
			md5 = file_md5(fullpath);

			if(md5 == NULL) {
				free_uri(uri);
				continue;
			}

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif

#endif
			
#ifdef FDT_INST_FEC_OTI_FILE
			if(s->use_fec_oti_ext_hdr == 0) {

#ifndef FDT_SCHEMA_FLUTEv07
				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-FEC-Encoding-ID=\"%u\"", s->def_fec_enc_id);
#endif

				if(s->def_fec_enc_id >= 128) {
					fprintf(fp, "\n\t\t");
					fprintf(fp, "FEC-OTI-FEC-Instance-ID=\"%u\"", s->def_fec_inst_id);
				}

				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-Maximum-Source-Block-Length=\"%u\"", s->def_max_sblen);
				fprintf(fp, "\n\t\t");
				fprintf(fp, "FEC-OTI-Encoding-Symbol-Length=\"%u\"", s->def_eslen);

				if(s->def_fec_enc_id == SB_SYS_FEC_ENC_ID) {
					fprintf(fp, "\n\t\t");
					fprintf(fp, "FEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"", max_n);	
				}
			}
#endif

			fprintf(fp, "/>\n");
			
			toi++;
			free_uri(uri);

#ifdef USE_OPENSSL
			free(md5);
			md5 = NULL;
#endif
		}
		entry = readdir(dirptr);
	}
	closedir(dirptr);

#endif
	
	if(hostname != NULL) {
		free(hostname);
	}

	if(toi == 1) {
		return -1;
	}

	return 0;
}

/*
 * This function encodes file to fdt.
 *
 * Params:	char *file: Pointer to buffer containing file to be encoded,
 *			char *base_dir: Pointer to buffer containing base directory for file
 *							or directory to be parsed to the fdt,
 *			FILE *fp: File pointer,
 *			int *s_id: Pointer to session identifier.
 *
 * Return:	int: 0 in success, -1 otherwise.
 *
 */

int encode_file(char *file, char *base_dir, FILE *fp, int *s_id) {

	char fullname[MAX_PATH + MAX_LENGTH];
	char fullpath[MAX_PATH + MAX_LENGTH];
	struct stat file_stats;
	int i;

	uri_t *uri = NULL;
	char *hostname = NULL;
	alc_session_t *s = get_alc_session(*s_id);
	char *user = NULL;
	char *uri_str = NULL;	

#ifdef USE_ZLIB
    int retcode;
    char enc_fullpath[MAX_PATH + MAX_LENGTH];
    struct stat enc_file_stats;
#endif

#ifdef USE_OPENSSL
	char *md5 = NULL;
#endif

#ifdef FDT_INST_FEC_OTI_FILE
	div_t div_max_n;
	int max_n;

	div_max_n = div((s->def_max_sblen * (100 + s->def_fec_ratio)), 100);
	max_n = div_max_n.quot;
#endif

#ifdef WIN32
	user = getenv("USERNAME");
#else
	user = getenv("USER");
#endif

	memset(fullpath, 0, (MAX_PATH + MAX_LENGTH));

	if(!(strcmp(base_dir, "") == 0)) {
		strcpy(fullpath, base_dir);

#ifdef WIN32
		strcat(fullpath, "\\");
#else
		strcat(fullpath, "/");
#endif
	}

	strcat(fullpath, file);

	if(stat(fullpath, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", fullpath);
		fflush(stdout);
		return -1;
	}

	if(file_stats.st_size == 0) {
		printf("Error: file %s size = 0\n", fullpath);
		fflush(stdout);
		return -1;
	}

	hostname = getdnsname();	

	memset(fullname, 0, (MAX_PATH + MAX_LENGTH));
	strcpy(fullname, file);

	for(i = 0; i < (int)strlen(fullname); i++) {
		if(fullname[i] == '\\') {
			fullname[i] = '/';
		}
	}

	uri = alloc_uri_struct();

	set_uri_scheme(uri, "file");

	if(user != NULL) {
		set_uri_user(uri, user);
	}	
	
	if(hostname != NULL) {
		set_uri_host(uri, hostname);
	}

	set_uri_path(uri, fullname);

#ifdef WIN32
	fprintf(fp, "\t<File TOI=\"%I64u\"", toi);
#else
	fprintf(fp, "\t<File TOI=\"%llu\"", toi);
#endif

	uri_str = uri_string(uri);
	
	fprintf(fp, "\n\t\t");
	fprintf(fp, "Content-Location=\"%s\"", uri_str);
	
	free(uri_str);
	
	fprintf(fp, "\n\t\t");
	fprintf(fp, "Content-Length=\"%lu\"", file_stats.st_size);


#ifdef USE_ZLIB
       	
	if(s->encode_content == ENCODE_FILES) {
                
		retcode = file_gzip_compress(fullpath, "wb");
                                                                                                                                                              
        	if(retcode == 0) {
                	fprintf(fp, "\n\t\t");
                	fprintf(fp, "Content-Encoding=\"%s\"", "gzip");
                                                                                                                                                              
               		memset(enc_fullpath, 0 , MAX_PATH);
                	strcpy(enc_fullpath, fullpath);
                	strcat(enc_fullpath, GZ_SUFFIX);
                                                                                                                                                              
                	if(stat(enc_fullpath, &enc_file_stats) == -1) {
                        printf("Error: %s is not valid file name\n", enc_fullpath);
                        fflush(stdout);
                        free(hostname);
						free_uri(uri);
                        return -1;
                    }

#ifdef USE_OPENSSL
					md5 = file_md5(enc_fullpath);

					if(md5 == NULL) {
						free(hostname);
						free_uri(uri);
						return -1;
					}

					fprintf(fp, "\n\t\t");
					fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif
                                                                                                                                                              
                    fprintf(fp, "\n\t\t");
					fprintf(fp, "Transfer-Length=\"%lu\"", enc_file_stats.st_size);

          	}
      	}
		else {

#ifdef USE_OPENSSL
			md5 = file_md5(fullpath);

			if(md5 == NULL) {
				free(hostname);
				free_uri(uri);
				return -1;
			}

			fprintf(fp, "\n\t\t");
			fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif
		}
#else

#ifdef USE_OPENSSL
		md5 = file_md5(fullpath);

		if(md5 == NULL) {
			free(hostname);
			free_uri(uri);
			return -1;
		}

		fprintf(fp, "\n\t\t");
		fprintf(fp, "Content-MD5=\"%s\"", md5);
#endif

#endif
	
#ifdef FDT_INST_FEC_OTI_FILE
	if(s->use_fec_oti_ext_hdr == 0) {
		
#ifndef FDT_SCHEMA_FLUTEv07
		fprintf(fp, "\n\t\t");
		fprintf(fp, "FEC-OTI-FEC-Encoding-ID=\"%u\"", s->def_fec_enc_id);
#endif

		if(s->def_fec_enc_id >= 128) {
			fprintf(fp, "\n\t\t");
			fprintf(fp, "FEC-OTI-FEC-Instance-ID=\"%u\"", s->def_fec_inst_id);
		}

		fprintf(fp, "\n\t\t");
		fprintf(fp, "FEC-OTI-Maximum-Source-Block-Length=\"%u\"", s->def_max_sblen);
		fprintf(fp, "\n\t\t");
		fprintf(fp, "FEC-OTI-Encoding-Symbol-Length=\"%u\"", s->def_eslen);

		if(s->def_fec_enc_id == SB_SYS_FEC_ENC_ID) {
			fprintf(fp, "\n\t\t");
			fprintf(fp, "FEC-OTI-Max-Number-of-Encoding-Symbols=\"%u\"", max_n);	
		}
	}
#endif

	fprintf(fp, "/>\n");
	free_uri(uri);

#ifdef USE_OPENSSL
	free(md5);
#endif
	
	if(hostname != NULL) {
		free(hostname);
	}

	return 0;
}

/*
 * This function checks if path is empty, or . or ..
 *
 * Params: const char *path: Pointer to buffer containg path.
 *
 * Return:	int: 1 if path is empty or . or .., 0  otherwise.
 *
 */

int checkpath(const char *path) {
	int ret = 0;

	if(strcmp(path, "") == 0) {
		ret = 1;
	}
	else if(strcmp(path, ".") == 0) {
		ret = 1;
	}
	else if(strcmp(path, "..") == 0) {
		ret = 1;
	}

	return ret;
}
