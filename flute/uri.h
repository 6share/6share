/* $Author: peltotal $ $Date: 2004/04/27 06:54:40 $ $Revision: 1.3 $ */
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

#ifndef _URI_H_
#define _URI_H_

/**** Typedef ****/

typedef struct uri {

	char* scheme;	/* URI scheme (file, http, ftp...) */
	char* host;		/* hostname part (www.foo.com) */
	char* port;		/* port part if any (www.foo.com:8080 => 8080) */
	char* path;		/* path portion without params and query */
	char* params;	/* params part (/foo;dir/bar => foo) */
	char* query;	/* query part (/foo?bar=val => bar=val) */
	char* frag;		/* frag part (/foo#part => part) */
	char* user;		/* user part (http://user:pass@www.foo.com => user) */
	char* passwd;	/* user part (http://user:pass@www.foo.com => pass) */

} uri_t;


/**** Functions ****/

uri_t* alloc_uri_struct();
uri_t* parse_uri(char* uri_string, int len);
char* uri_string(uri_t *uri);
void free_uri(uri_t* uri);

char* parse_scheme(uri_t *uri, char *uri_pointer);
char* get_uri_scheme(uri_t* uri);
void set_uri_scheme(uri_t* uri, char* scheme);

char* parse_authority(uri_t *uri, char *uri_pointer);
char* parse_hostport(uri_t *uri, char *uri_pointer);

char* get_uri_host_and_path(uri_t* uri);

char* get_uri_host(uri_t* uri);
void set_uri_host(uri_t* uri, char* host);

char* get_uri_port(uri_t* uri);
void set_uri_port(uri_t* uri, char* port);

char* parse_path(uri_t *uri, char *uri_pointer);
char* get_uri_path(uri_t* uri);
void set_uri_path(uri_t* uri, char* path);

char* parse_params(uri_t *uri, char *uri_pointer);
char* get_uri_params(uri_t* uri);
void set_uri_params(uri_t* uri, char* params);

char* parse_query(uri_t *uri, char *uri_pointer);
char* get_uri_query(uri_t* uri);
void set_uri_query(uri_t* uri, char* query);

char* parse_frag(uri_t *uri, char *uri_pointer);
char* get_uri_frag(uri_t* uri);
void set_uri_frag(uri_t* uri, char* frag);

char* parse_userinfo(uri_t *uri, char *uri_pointer);

char* get_uri_user(uri_t* uri);
void set_uri_user(uri_t* uri, char* user);

char* get_uri_passwd(uri_t* uri);
void set_uri_passwd(uri_t* uri, char* passwd);

#endif







