/* $Author: peltotal $ $Date: 2004/04/30 06:24:09 $ $Revision: 1.12 $ */
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

#ifndef _FDT_H_
#define _FDT_H_

/**** Typedefs ****/

typedef struct file {
	
	struct file *prev;
	struct file *next;

	int is_downloaded; /* 0 = not, 1 = downloading, 2 = downloaded */

#ifdef WIN32
	ULONGLONG toi;
	ULONGLONG expires;
	ULONGLONG toi_len;	
	ULONGLONG file_len;
#else
	unsigned long long toi;
	unsigned long long expires;
	unsigned long long toi_len;
	unsigned long long file_len;
#endif

	char *location;
	char *type;
	char *md5;
	char *encoding;

/* FEC OTI */

	short fec_encoding_id;
	int fec_instance_id;
	unsigned int max_source_block_length;
	unsigned short encoding_symbol_length;
	unsigned short max_nb_of_encoding_symbols;

} file_t;

typedef struct fdt {
#ifdef WIN32
	ULONGLONG expires;
#else
	unsigned long long expires;  /* fdt expiration time in NTP-time */
#endif
	bool is_complete;
	file_t *file_list;

/* FEC OTI */

	short fec_encoding_id;
	int fec_instance_id;
	unsigned int max_source_block_length;
	unsigned short encoding_symbol_length;
	unsigned short max_nb_of_encoding_symbols;
} fdt_t;

typedef struct fdt_th_args{
	int s_id;
	fdt_t *fdt;
	int rx_automatic;				/* Download files defined in fdt automatically */
	char *filetoken;				/* wild card filetoken */
	int accept_expired_fdt_inst;	/* Accept expired FDT Instances in receiver */
} fdt_th_args_t;

/**** Functions ****/

fdt_t* decode_fdt_payload(char *fdt_payload);
void* fdt_thread(void *a);
void FreeFDT(fdt_t *fdt);
void free_file(file_t *file);

#endif
