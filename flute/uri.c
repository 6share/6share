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

#include "../alclib/inc.h"

#include <stdlib.h>
#include "uri.h"

/*
 * This function allocates memory for uri_t structure.
 *
 * Params:	void	
 *
 * Return:	uri_t*: Pointer to allocated uri_t structure,
 *			NULL: In error cases.
 *
 */

uri_t* alloc_uri_struct(void) {
	
	uri_t *uri = NULL;

	if(!(uri = (uri_t*)calloc(1, sizeof(uri_t)))) {
		printf("Could not alloc memory for uri structure!\n");
	}

	return uri;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

uri_t* parse_uri(char* uri_string, int len) {

	uri_t* uri = NULL;
	char* tmp_uri = NULL;
	char* uri_pointer = NULL;

	if(!(tmp_uri = (char*)calloc((strlen(uri_string) + 1), sizeof(char)))) {
		printf("Could not alloc memory for tmp (uri string)!\n");
		return NULL;
	}

	memcpy(tmp_uri, uri_string, strlen(uri_string));

/* fix up escaping */
        char *write= tmp_uri, *read= tmp_uri;
	char code[3];
	while (*read) {
          switch (*read) {
	    case '%':
		code[0] = read[1];
		code[1] = read[2];
		code[2] = 0;
		unsigned char c = strtol(code, NULL, 16);
		*write = c;
		read += 2; /* skip forward */
		break;
	    case '+':
		*write = ' ';
		break;
	    default:
		*write = *read;
		break;
	  }
          *read++;
	  *write++;
	}
	*write= '\0';

	if(!(uri = (uri_t*)calloc(1, sizeof(uri_t)))) {
		printf("Could not alloc memory for uri structure!\n");
		free(tmp_uri);
		return NULL;
	}

	uri_pointer = parse_scheme(uri, tmp_uri);

	if(strcmp(uri_pointer, uri_string) == 0) {
	
		uri_pointer = parse_path(uri, uri_pointer);
		
		if(uri_pointer != NULL) {
			uri_pointer = parse_frag(uri, uri_pointer);
		}
	}
	else {
		uri_pointer = parse_authority(uri, uri_pointer);
		uri_pointer = parse_path(uri, uri_pointer);

		if(uri_pointer != NULL) {
			uri_pointer = parse_frag(uri, uri_pointer);
		}
	}

	free(tmp_uri);
	return uri;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* uri_string(uri_t *uri) {
	
	char *uri_str;
	int len = 0;

	if(uri->scheme != NULL) {
		len += strlen(uri->scheme);
		len += 3; /* '://' */
	}

	if(strcmp(uri->scheme, "file") == 0) {
		if(uri->host != NULL) {
			len += strlen(uri->host);
		}
		if(uri->port != NULL) {
			len += strlen(uri->port);
			len += 1; /* ':' */
		}
		if(uri->path != NULL) {
			len += 1; /* '/' */
			len += strlen(uri->path);
		}
		/* TODO: add more */
	}
	if(strcmp(uri->scheme, "ftp") == 0) {
		/* TODO */
	}
	if(strcmp(uri->scheme, "http") == 0) {
		/* TODO */
	}

	if(!(uri_str = (char*)calloc((len + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri string!\n");
		fflush(stdout);
		return NULL;
	}

	if(uri->scheme != NULL) {
		strcat(uri_str, uri->scheme);
		strcat(uri_str, "://");
	}

	if(strcmp(uri->scheme, "file") == 0) {
		if(uri->host != NULL) {
			strcat(uri_str, uri->host);	
		}
		if(uri->port != NULL) {
			strcat(uri_str, ":");
			strcat(uri_str, uri->port);
		}
		if(uri->path != NULL) {
			strcat(uri_str, "/");
			strcat(uri_str, uri->path);
		}
		/* TODO: add more */
	}
	if(strcmp(uri->scheme, "ftp") == 0) {
		/* TODO */
	}	
	if(strcmp(uri->scheme, "http") == 0) {
		/* TODO */	
	}

    return uri_str;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void free_uri(uri_t* uri) {

	if(uri->scheme != NULL) {
		free(uri->scheme);
	}
	if(uri->host != NULL) {
		free(uri->host);
	}
	if(uri->port != NULL) {
		free(uri->port);
	}
	if(uri->path != NULL) {
		free(uri->path);
	}
	if(uri->params != NULL) {
		free(uri->params);
	}
	if(uri->query != NULL) {
		free(uri->query);
	}
	if(uri->frag != NULL) {
		free(uri->frag);
	}
	if(uri->user != NULL) {
		free(uri->user);
	}
	if(uri->passwd != NULL) {
		free(uri->passwd);
	}

	free(uri);
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_scheme(uri_t* uri, char* uri_pointer) {

	char *new_uri_pointer = NULL;
	char *scheme = NULL;
	int scheme_length = 0;

	unsigned char* start = (unsigned char*)uri_pointer;
	unsigned char* end;

	while(*start && isspace((int)*start)) {
		start++;
	}

	end = start;
  
	while(*end && (isalnum((int)*end) || *end == '+' || *end == '-' || *end == '.')) {
		/**end = tolower((int)*end);*/
		end++;
	}
	
	if(*end != '\0' && end > start && *end == ':') {
		scheme = start;
		scheme_length = end - start;
		scheme[scheme_length] = '\0';
	}
	else {
		new_uri_pointer = uri_pointer;
	}

	if(scheme_length != 0) {

		end = scheme;
  
        	while(*end) {
                	*end = tolower((int)*end);
                	end++;
        	}

		set_uri_scheme(uri, scheme);
		new_uri_pointer = scheme + scheme_length + 1;		
	}

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_scheme(uri_t* uri) {
	return uri->scheme;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_scheme(uri_t* uri, char* scheme) {

	if(uri->scheme != NULL) {
		free(uri->scheme);
	}

	if(!(uri->scheme = (char*)calloc((strlen(scheme) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->scheme!\n");
		fflush(stdout);
	}
		
	memcpy(uri->scheme, scheme, strlen(scheme));
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_authority(uri_t* uri, char* uri_pointer) {

	char *new_uri_pointer = NULL;
	char *userinfo = NULL;
	char *hostport = NULL;
	/*char *port = NULL;*/

	unsigned char* start = (unsigned char*)uri_pointer;
	unsigned char* end;

	if(start[0] == '/' && start[1] == '/' && start[2] == '/') {
		/* Authority part not present */
		new_uri_pointer = start + 3;
	}
	else if(start[0] == '/' && start[1] == '/') {
		/* Authority part present, Server-Based Naming Authority supposed */
			
		start += 2;
		userinfo = start;
		end = start;
			
		while(*end && *end != '/') {
			end++;
		}

		*end = '\0';

		new_uri_pointer = end + 1;
			
		hostport = parse_userinfo(uri, userinfo);
		new_uri_pointer = parse_hostport(uri, hostport);

	}

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_hostport(uri_t* uri, char* uri_pointer) {

	char *new_uri_pointer = NULL;
	/*char *host = NULL;
	char *port = NULL;*/

	unsigned char* start = (unsigned char*)uri_pointer;
	unsigned char* end = start + strlen(uri_pointer);
	
	unsigned char* host_start = start;
	unsigned char* host_end;

	if((host_end = strchr(host_start, ':'))) {
		*host_end = '\0';
		set_uri_host(uri, host_start);
		set_uri_port(uri, (host_end + 1));
	}
	else {
		set_uri_host(uri, start);
	}

	new_uri_pointer = end + 1;	

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_host(uri_t* uri) {
	return uri->host;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_host(uri_t* uri, char* host) {
	
	if(uri->host != NULL) {
		free(uri->host);
	}

	if(!(uri->host = (char*)calloc((strlen(host) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->host!\n");
		fflush(stdout);
	}
		
	memcpy(uri->host, host, strlen(host));
}


/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_port(uri_t* uri) {
	return uri->port;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_port(uri_t* uri, char* port) {
	
	if(uri->port != NULL) {
		free(uri->port);
	}

	if(!(uri->port = (char*)calloc((strlen(port) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->port!\n");
		fflush(stdout);
	}
		
	memcpy(uri->port, port, strlen(port));
}

/*
char* uri_parse_query(uri_t* object, char* p, int flags)
{
  if(!(flags & URI_SCHEME_GENERIC_PARSE_SKIP_QUERY)) {
    object->query = strchr(p, '?');
    if(object->query) {
      *object->query = '\0';
      object->query++;
    }
  }

  return p;
}

char* uri_parse_params(uri_t* object, char* p, int flags)
{
  object->params = strchr(p, ';');
  if(object->params) {
    *object->params = '\0';
    object->params++;
  }

  return p;
}*/

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_host_and_path(uri_t* uri) {
	
	char *tmp = NULL;
	int len = 0;

	if(uri->host != NULL) {
		len += strlen(uri->host);
		len += 1;
	}
	
	len += strlen(uri->path);
	
	if(!(tmp = (char*)calloc((len + 1), sizeof(char)))) {
		printf("Could not alloc memory for tmp (get_uri_host_and_path)!\n");
		fflush(stdout);
		return NULL;
	}
	
#if 0
	if(uri->host != NULL) {
		strcat(tmp, uri->host);
		strcat(tmp, "/");
	}
#endif
	
	char *slash = strrchr(uri->path, '/');
	if (slash && slash[1] != 0) {
		strcat(tmp, slash);
	} else {
		strcat(tmp, uri->path);
	}

	return tmp;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_path(uri_t* uri, char* uri_pointer) {

	char *new_uri_pointer = NULL;
	/*char *user = NULL;
	char *passwd = NULL;*/

	unsigned char* start = (unsigned char*)uri_pointer;
	/*unsigned char* end = start + strlen(uri_pointer);*/
	
	unsigned char* path_start = start;
	unsigned char* path_end;

	if((path_end = strchr(start, '#'))) {
		*path_end = '\0';
		
		set_uri_path(uri, path_start);
		new_uri_pointer = path_end + 1;
	}
	else {
		set_uri_path(uri, start);
		return NULL;
	}

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_path(uri_t* uri) {
	return uri->path;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_path(uri_t* uri, char* path) {
	
	if(uri->path != NULL) {
		free(uri->path);
	}

	if(!(uri->path = (char*)calloc((strlen(path) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->path!\n");
		fflush(stdout);
	}
		
	memcpy(uri->path, path, strlen(path));
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_params(uri_t* uri, char* uri_pointer) {

	char *new_uri_pointer = NULL;
	char *scheme = NULL;
	int scheme_length = 0;

	unsigned char* start = (unsigned char*)uri_pointer;
	unsigned char* end;

	while(*start && isspace((int)*start)) {
		start++;
	}

	end = start;
  
	while(*end && (isalnum((int)*end) || *end == '+' || *end == '-' || *end == '.')) {
		end++;
	}
	
	if(*end != '\0' && end > start && *end == ':') {
		scheme = start;
		scheme_length = end - start;
		scheme[scheme_length] = '\0';
	}
	else {
		return NULL;
	}

	if(scheme_length != 0) {
		
		set_uri_scheme(uri, scheme);
		new_uri_pointer = scheme + scheme_length + 1;		
	}

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_params(uri_t* uri) {
	return uri->params;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_params(uri_t* uri, char* params) {
	
	if(uri->params != NULL) {
		free(uri->params);
	}

	if(!(uri->params = (char*)calloc((strlen(params) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->params!\n");
		fflush(stdout);
	}
		
	memcpy(uri->params, params, strlen(params));
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_query(uri_t* uri, char* uri_pointer) {

	char *new_uri_pointer = NULL;
	char *scheme = NULL;
	int scheme_length = 0;

	unsigned char* start = (unsigned char*)uri_pointer;
	unsigned char* end;

	while(*start && isspace((int)*start)) {
		start++;
	}

	end = start;
  
	while(*end && (isalnum((int)*end) || *end == '+' || *end == '-' || *end == '.')) {
		end++;
	}
	
	if(*end != '\0' && end > start && *end == ':') {
		scheme = start;
		scheme_length = end - start;
		scheme[scheme_length] = '\0';
	}
	else {
		return NULL;
	}

	if(scheme_length != 0) {
		
		set_uri_scheme(uri, scheme);
		new_uri_pointer = scheme + scheme_length + 1;		
	}

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_query(uri_t* uri) {
	return uri->query;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_query(uri_t* uri, char* query) {
	
	if(uri->query != NULL) {
		free(uri->query);
	}

	if(!(uri->query = (char*)calloc((strlen(query) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->query!\n");
		fflush(stdout);
	}
		
	memcpy(uri->query, query, strlen(query));
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_frag(uri_t* uri, char* uri_pointer) {
	
	set_uri_frag(uri, uri_pointer);
	
	return NULL;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_frag(uri_t* uri) {
	return uri->frag;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_frag(uri_t* uri, char* frag) {
	
	if(uri->frag != NULL) {
		free(uri->frag);
	}

	if(!(uri->frag = (char*)calloc((strlen(frag) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->frag!\n");
		fflush(stdout);
	}
		
	memcpy(uri->frag, frag, strlen(frag));
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* parse_userinfo(uri_t *uri, char *uri_pointer) {

	char *new_uri_pointer = NULL;
	/*char *user = NULL;
	char *passwd = NULL;*/

	unsigned char* start = (unsigned char*)uri_pointer;
	unsigned char* end;
	
	unsigned char* user_start = start;
	unsigned char* user_end;

	if((end = strchr(start, '@'))) {
		*end = '\0';
			
		if((user_end = strchr(user_start, ':'))) {
			*user_end = '\0';

			set_uri_user(uri, user_start);
			set_uri_passwd(uri, (user_end + 1));
		}
		else {
			set_uri_user(uri, start);
		}

		new_uri_pointer = end + 1;	
	}
	else {
		new_uri_pointer = uri_pointer;	
	}

	return new_uri_pointer;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_user(uri_t* uri) {
	return uri->user;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_user(uri_t* uri, char* user) {
	
	if(uri->user != NULL) {
		free(uri->user);
	}

	if(!(uri->user = (char*)calloc((strlen(user) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->user!\n");
		fflush(stdout);
	}
		
	memcpy(uri->user, user, strlen(user));
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

char* get_uri_passwd(uri_t* uri) {
	return uri->passwd;
}

/*
 * This function.
 *
 * Params:	
 *
 * Return:
 *
 */

void set_uri_passwd(uri_t* uri, char* passwd) {
	
	if(uri->passwd != NULL) {
		free(uri->passwd);
	}

	if(!(uri->passwd = (char*)calloc((strlen(passwd) + 1), sizeof(char)))) {
		printf("Could not alloc memory for uri->passwd!\n");
		fflush(stdout);
	}
		
	memcpy(uri->passwd, passwd, strlen(passwd));
}
