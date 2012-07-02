/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.5 $ */
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

/*
 * This function returns DNS name for host.
 *
 * Params:	void
 *
 * Return:	char*: Pointer to buffer containing host's  DNS name, 
 *			NULL: In error cases.
 */

char* getdnsname() {

	char *hostname = NULL;
	char host[MAX_PATH];
	struct hostent *hptr;

#ifdef WIN32
    int d;
    WSADATA ws;

    d = WSAStartup(0x0202, &ws);
#endif

	memset(host, 0, MAX_PATH);
    gethostname(host, MAX_PATH);

    if((hptr = gethostbyname(host)) == NULL) {
		
		if(!(hostname = (char*)calloc((strlen(host) + 1), sizeof(char)))) {
			printf("Could not alloc memory for hostname!\n");
			fflush(stdout);
			return NULL;
		}

		memcpy(hostname, host, strlen(host));
		return hostname;

    } else {
		memset(host, 0, MAX_PATH);
        memcpy(host, hptr->h_name, strlen(hptr->h_name));
	}


	if(!(hostname = (char*)calloc((strlen(host) + 1), sizeof(char)))) {
		printf("Could not alloc memory for hostname!\n");
		fflush(stdout);
		return NULL;
	}

	memcpy(hostname, host, strlen(host));
    
    return hostname;
}
