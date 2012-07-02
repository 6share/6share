/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.8 $ */
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

#ifndef _FDT_GEN_H_
#define _FDT_GEN_H_

/**** Typedef ****/

typedef struct file_info {
	struct file_info *next;
	struct file_info *prev;
	char *location;
	unsigned int toi;
	/* should not be here */
	void *data;
} file_info_t;

int generate_fdt(char *file_path, char *base_dir, int *s_id, char* fdt_file_name);

int generate_fdt_from_list(file_info_t *list, int *s_id, char *fdt_file_name);

#endif

