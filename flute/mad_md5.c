/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.13 $ */
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

#ifdef USE_OPENSSL

#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "mad_md5.h"

/*
 * This function calculates MD5 for file.
 *
 * Params:	char *filename: Pointer to buffer containg file name.
 *
 * Return:	char*: Pointer to buffer containing file's MD5, 
 *			NULL: In error cases.
 */

char* file_md5(char *filename) {
				
	/*char *buf;*/
	struct stat file_stats;

	int nbytes;
	FILE *fp;
	char *md5 = NULL;

	char b64_digest[MD5_DIGEST_LENGTH*2];
	BIO *bio, *b64, *mem;

	unsigned char md5_digest[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;				
	int i;
	char zBuf[10240]; /*10240*/

	memset(md5_digest, 0, MD5_DIGEST_LENGTH);
	memset(b64_digest, 0, MD5_DIGEST_LENGTH*2);

	MD5_Init(&ctx);

	fp = fopen(filename, "rb");

	if(stat(filename, &file_stats) == -1) {
		printf("Error: %s is not valid file name\n", filename);
		fflush(stdout);
        return NULL;
	}

	/*if(!(buf = (char*)calloc((file_stats.st_size + 1), sizeof(char)))) {
		printf("Could not alloc memory for buf!\n");
		fflush(stdout);
        return NULL;
	} 

	nbytes = fread(buf, 1, file_stats.st_size, fp);*/

	for(;;) {
		nbytes = fread(zBuf, 1, sizeof(zBuf), fp);
	
		if(nbytes <= 0) {
			break;
		}
		
		MD5_Update(&ctx, (unsigned char*)zBuf, (unsigned)nbytes);
    }

	/*if(ferror(fp)) {
		printf("fread error, file: %s\n", filename);
		fflush(stdout);
		free(buf);
		fclose(fp);
		return NULL;
	}*/

	MD5_Final(md5_digest, &ctx); 

	b64 = BIO_new(BIO_f_base64());
	mem = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, mem);
	BIO_write(bio, md5_digest, MD5_DIGEST_LENGTH);
	BIO_flush(bio);
	BIO_gets(mem, b64_digest, MD5_DIGEST_LENGTH*2);
	BIO_free_all(bio);

	for(i = 0; i < MD5_DIGEST_LENGTH*2; i++) {
		if(b64_digest[i] <= ' ') {
			b64_digest[i] = '\0';
        }
	}

	if(!(md5 = (char*)calloc((strlen(b64_digest) + 1), sizeof(char)))) {
		printf("Could not alloc memory for md5 buffer!\n");
		fflush(stdout);
        return NULL;
	} 

	memcpy(md5, b64_digest, strlen(b64_digest));
	
	fclose(fp);

	/*md5 = buffer_md5(buf, nbytes);

	free(buf);*/

	return md5;
}

/*
 * This function calculates MD5 for buffer.
 *
 * Params:	char *buffer: Pointer to buffer containg data,
 *			ULONGLONG/unsigned long long length: Buffer length.
 *
 * Return:	char*: Pointer to buffer containing data's MD5, 
 *			NULL: In error cases.
 */

char* buffer_md5(char *buffer, 
#ifdef WIN32
				 ULONGLONG length
#else
				 unsigned long long length
#endif
				 ) {
	
	char b64_digest[MD5_DIGEST_LENGTH*2];
	BIO *bio, *b64, *mem;

	unsigned char md5_digest[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;				
	int i;
	char *md5 = NULL;

	memset(md5_digest, 0, MD5_DIGEST_LENGTH);
	memset(b64_digest, 0, MD5_DIGEST_LENGTH*2);

	MD5_Init(&ctx);

	MD5_Update(&ctx, buffer, (unsigned int)length);
	MD5_Final(md5_digest, &ctx); 

	b64 = BIO_new(BIO_f_base64());
	mem = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, mem);
	BIO_write(bio, md5_digest, MD5_DIGEST_LENGTH);
	BIO_flush(bio);
	BIO_gets(mem, b64_digest, MD5_DIGEST_LENGTH*2);
	BIO_free_all(bio);

	for(i = 0; i < MD5_DIGEST_LENGTH*2; i++) {
		if(b64_digest[i] <= ' ') {
			b64_digest[i] = '\0';
        }
	}

	if(!(md5 = (char*)calloc((strlen(b64_digest) + 1), sizeof(char)))) {
		printf("Could not alloc memory for md5 buffer!\n");
		fflush(stdout);
        return NULL;
	} 

	memcpy(md5, b64_digest, strlen(b64_digest));

	return md5;
}

#endif
