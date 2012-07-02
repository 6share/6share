/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.15 $ */
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

#ifndef _SENDER_H_
#define _SENDER_H_

/**** Typedef ****/

typedef struct send_file_th_args{
	char *tx_file;
	int s_id;
	int ch_id;
	unsigned int toi;
	unsigned int sblen;
	int sentbytes;
} send_file_th_args_t;

/**** Functions ****/

int send_file(char *tx_file, int s_id, int ch_id,
#ifdef WIN32
	      ULONGLONG toi,
#else
	      unsigned long long toi,
#endif
	      unsigned short eslen, unsigned int max_sblen, unsigned char fec_enc_id,
		  unsigned short fec_inst_id);

int fdtbasedsend(char *fdt_file, int s_id, int ch_id[MAX_CHANNELS_IN_SESSION], int nb_channel,
	         unsigned short eslen, unsigned int max_sblen, int fls_btw_fdt_inst,
			 unsigned char fec_enc_id, unsigned short fec_inst_id, int is_last_loop);
int fdtbasedsend2(char *fdt_file, char *base_dir, char *file_path, int *s_id, int *ch_id,
				  unsigned short eslen, unsigned int max_sblen, int files_in_fdt_inst,
				  unsigned char fec_enc_id, unsigned short fec_inst_id,
				  int use_fec_oti_ext_hdr,
#ifdef USE_ZLIB
		  int encode_content, int is_last_loop);
#else
		  int is_last_loop);
#endif

int fdtbasedsend3(char *fdt_file, char *base_dir, char *file_path, int *s_id, int nb_channel,
				  unsigned short eslen, unsigned int max_sblen, int files_in_fdt_inst,
				  unsigned char fec_enc_id, unsigned short fec_inst_id,
				  int use_fec_oti_ext_hdr
#ifdef USE_ZLIB
	          , int encode_content);
#else
	          );
#endif

#ifdef USE_ZLIB
int remove_gz_files(char *fdt_file, char *base_dir, char *file_path);
#endif

#endif /* _SENDER_H_ */
