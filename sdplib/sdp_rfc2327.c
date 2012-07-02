/* $Author: peltotal $ $Date: 2004/09/20 09:20:50 $ $Revision: 1.4 $ */
/*
  The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
  Copyright (C) 2001,2002,2003  Aymeric MOIZARD jack@atosc.org
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>

#include <stdlib.h>



#include "sdp.h"

#include "const.h"

#include "port.h"



#define ERR_ERROR   -1		/* bad header */

#define ERR_DISCARD  0		/* wrong header */

#define WF           1		/* well formed header */





int

sdp_bandwidth_init (sdp_bandwidth_t ** b)

{

  *b = (sdp_bandwidth_t *) malloc (sizeof (sdp_bandwidth_t));

  if (*b == NULL)

    return -1;

  (*b)->b_bwtype = NULL;

  (*b)->b_bandwidth = NULL;

  return 0;

}



void

sdp_bandwidth_free (sdp_bandwidth_t * b)

{

  if (b == NULL)

    return;

  free (b->b_bwtype);

  free (b->b_bandwidth);

}



int

sdp_time_descr_init (sdp_time_descr_t ** td)

{

  *td = (sdp_time_descr_t *) malloc (sizeof (sdp_time_descr_t));

  if (*td == NULL)

    return -1;

  (*td)->t_start_time = NULL;

  (*td)->t_stop_time = NULL;

  (*td)->r_repeats = (list_t *) malloc (sizeof (list_t));

  list_init ((*td)->r_repeats);

  return 0;

}



void

sdp_time_descr_free (sdp_time_descr_t * td)

{

  if (td == NULL)

    return;

  free (td->t_start_time);

  free (td->t_stop_time);

  listofchar_free (td->r_repeats);

  free (td->r_repeats);

}



int

sdp_key_init (sdp_key_t ** key)

{

  *key = (sdp_key_t *) malloc (sizeof (sdp_key_t));

  if (*key == NULL)

    return -1;

  (*key)->k_keytype = NULL;

  (*key)->k_keydata = NULL;

  return 0;

}



void

sdp_key_free (sdp_key_t * key)

{

  if (key == NULL)

    return;

  free (key->k_keytype);

  free (key->k_keydata);

}



int

sdp_attribute_init (sdp_attribute_t ** attribute)

{

  *attribute = (sdp_attribute_t *) malloc (sizeof (sdp_attribute_t));

  if (*attribute == NULL)

    return -1;

  (*attribute)->a_att_field = NULL;

  (*attribute)->a_att_value = NULL;

  return 0;

}



void

sdp_attribute_free (sdp_attribute_t * attribute)

{

  if (attribute == NULL)

    return;

  free (attribute->a_att_field);

  free (attribute->a_att_value);

}



int

sdp_connection_init (sdp_connection_t ** connection)

{

  *connection = (sdp_connection_t *) malloc (sizeof (sdp_connection_t));

  if (*connection == NULL)

    return -1;

  (*connection)->c_nettype = NULL;

  (*connection)->c_addrtype = NULL;

  (*connection)->c_addr = NULL;

  (*connection)->c_addr_multicast_ttl = NULL;

  (*connection)->c_addr_multicast_int = NULL;

  return 0;

}



void

sdp_connection_free (sdp_connection_t * connection)

{

  if (connection == NULL)

    return;

  free (connection->c_nettype);

  free (connection->c_addrtype);

  free (connection->c_addr);

  free (connection->c_addr_multicast_ttl);

  free (connection->c_addr_multicast_int);

}



int

sdp_media_init (sdp_media_t ** media)

{

  *media = (sdp_media_t *) malloc (sizeof (sdp_media_t));

  if (*media == NULL)

    return -1;

  (*media)->m_media = NULL;

  (*media)->m_port = NULL;

  (*media)->m_number_of_port = NULL;

  (*media)->m_proto = NULL;

  (*media)->m_payloads = (list_t *) malloc (sizeof (list_t));

  list_init ((*media)->m_payloads);

  (*media)->i_info = NULL;

  (*media)->c_connections = (list_t *) malloc (sizeof (list_t));

  list_init ((*media)->c_connections);

  (*media)->b_bandwidths = (list_t *) malloc (sizeof (list_t));

  list_init ((*media)->b_bandwidths);

  (*media)->a_attributes = (list_t *) malloc (sizeof (list_t));

  list_init ((*media)->a_attributes);

  (*media)->k_key = NULL;

  return 0;

}



void

sdp_media_free (sdp_media_t * media)

{

  if (media == NULL)

    return;

  free (media->m_media);

  free (media->m_port);

  free (media->m_number_of_port);

  free (media->m_proto);

  listofchar_free (media->m_payloads);

  free (media->m_payloads);

  free (media->i_info);

  list_special_free (media->c_connections,

		     (void *(*)(void *)) &sdp_connection_free);

  free (media->c_connections);

  list_special_free (media->b_bandwidths,

		     (void *(*)(void *)) &sdp_bandwidth_free);

  free (media->b_bandwidths);

  list_special_free (media->a_attributes,

		     (void *(*)(void *)) &sdp_attribute_free);

  free (media->a_attributes);

  sdp_key_free (media->k_key);

  free (media->k_key);

}



/* to be changed to sdp_init(sdp_t **dest) */

int

sdp_init (sdp_t ** sdp)

{

  (*sdp) = (sdp_t *) malloc (sizeof (sdp_t));

  if (*sdp == NULL)

    return -1;



  (*sdp)->v_version = NULL;

  (*sdp)->o_username = NULL;

  (*sdp)->o_sess_id = NULL;

  (*sdp)->o_sess_version = NULL;

  (*sdp)->o_nettype = NULL;

  (*sdp)->o_addrtype = NULL;

  (*sdp)->o_addr = NULL;

  (*sdp)->s_name = NULL;

  (*sdp)->i_info = NULL;

  (*sdp)->u_uri = NULL;



  (*sdp)->e_emails = (list_t *) malloc (sizeof (list_t));

  if ((*sdp)->e_emails == NULL)

    return -1;

  list_init ((*sdp)->e_emails);



  (*sdp)->p_phones = (list_t *) malloc (sizeof (list_t));

  if ((*sdp)->p_phones == NULL)

    return -1;

  list_init ((*sdp)->p_phones);



  (*sdp)->c_connection = NULL;



  (*sdp)->b_bandwidths = (list_t *) malloc (sizeof (list_t));

  if ((*sdp)->b_bandwidths == NULL)

    return -1;

  list_init ((*sdp)->b_bandwidths);



  (*sdp)->t_descrs = (list_t *) malloc (sizeof (list_t));

  if ((*sdp)->t_descrs == NULL)

    return -1;

  list_init ((*sdp)->t_descrs);



  (*sdp)->z_adjustments = NULL;

  (*sdp)->k_key = NULL;



  (*sdp)->a_attributes = (list_t *) malloc (sizeof (list_t));

  if ((*sdp)->a_attributes == NULL)

    return -1;

  list_init ((*sdp)->a_attributes);



  (*sdp)->m_medias = (list_t *) malloc (sizeof (list_t));

  if ((*sdp)->m_medias == NULL)

    return -1;

  list_init ((*sdp)->m_medias);

  return 0;

}



int

sdp_parse_v (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "v" */

  if (equal[-1] != 'v')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/*v=\r ?? bad header */

  sdp->v_version = malloc (crlf - (equal + 1) + 1);

  sstrncpy (sdp->v_version, equal + 1, crlf - (equal + 1));



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}





int

sdp_parse_o (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *tmp;

  char *tmp_next;

  int i;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "o" */

  if (equal[-1] != 'o')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* o=\r ?? bad header */



  tmp = equal + 1;

  /* o=username sess-id sess-version nettype addrtype addr */



  /* useranme can contain any char (ascii) except "space" and CRLF */

  i = set_next_token (&(sdp->o_username), tmp, ' ', &tmp_next);

  if (i != 0)

    return -1;

  tmp = tmp_next;



  /* sess_id contains only numeric characters */

  i = set_next_token (&(sdp->o_sess_id), tmp, ' ', &tmp_next);

  if (i != 0)

    return -1;

  tmp = tmp_next;



  /* sess_id contains only numeric characters */

  i = set_next_token (&(sdp->o_sess_version), tmp, ' ', &tmp_next);

  if (i != 0)

    return -1;

  tmp = tmp_next;



  /* nettype is "IN" but will surely be extented!!! assume it's some alpha-char */

  i = set_next_token (&(sdp->o_nettype), tmp, ' ', &tmp_next);

  if (i != 0)

    return -1;

  tmp = tmp_next;



  /* addrtype  is "IP4" or "IP6" but will surely be extented!!! */

  i = set_next_token (&(sdp->o_addrtype), tmp, ' ', &tmp_next);

  if (i != 0)

    return -1;

  tmp = tmp_next;



  /* addr  is "IP4" or "IP6" but will surely be extented!!! */

  i = set_next_token (&(sdp->o_addr), tmp, '\r', &tmp_next);

  if (i != 0)

    {				/* could it be "\n" only??? rfc says to accept CR or LF instead of CRLF */

      i = set_next_token (&(sdp->o_addr), tmp, '\n', &tmp_next);

      if (i != 0)

	return -1;

    }



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_s (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "s" */

  if (equal[-1] != 's')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* o=\r ?? bad header */



  /* s=text */



  /* text is interpreted as ISO-10646 UTF8! */

  /* using ISO 8859-1 requires "a=charset:ISO-8859-1 */

  sdp->s_name = malloc (crlf - (equal + 1) + 1);

  sstrncpy (sdp->s_name, equal + 1, crlf - (equal + 1));



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_i (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  int i;

  char *i_info;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "i" */

  if (equal[-1] != 'i')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* o=\r ?? bad header */



  /* s=text */



  /* text is interpreted as ISO-10646 UTF8! */

  /* using ISO 8859-1 requires "a=charset:ISO-8859-1 */

  i_info = malloc (crlf - (equal + 1) + 1);

  sstrncpy (i_info, equal + 1, crlf - (equal + 1));



  /* add the bandwidth at the correct place:

     if there is no media line yet, then the "b=" is the

     global one.

   */

  i = list_size (sdp->m_medias);

  if (i == 0)

    sdp->i_info = i_info;

  else

    {

      sdp_media_t *last_sdp_media =

	(sdp_media_t *) list_get (sdp->m_medias, i - 1);

      last_sdp_media->i_info = i_info;

    }



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_u (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "u" */

  if (equal[-1] != 'u')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* u=\r ?? bad header */



  /* u=uri */

  /* we assume this is a URI */

  sdp->u_uri = malloc (crlf - (equal + 1) + 1);

  sstrncpy (sdp->u_uri, equal + 1, crlf - (equal + 1));



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_e (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *e_email;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "e" */

  if (equal[-1] != 'e')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* e=\r ?? bad header */



  /* e=email */

  /* we assume this is an EMAIL-ADDRESS */

  e_email = malloc (crlf - (equal + 1) + 1);

  sstrncpy (e_email, equal + 1, crlf - (equal + 1));



  list_add (sdp->e_emails, e_email, -1);



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_p (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *p_phone;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "p" */

  if (equal[-1] != 'p')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* p=\r ?? bad header */



  /* e=email */

  /* we assume this is an EMAIL-ADDRESS */

  p_phone = malloc (crlf - (equal + 1) + 1);

  sstrncpy (p_phone, equal + 1, crlf - (equal + 1));



  list_add (sdp->p_phones, p_phone, -1);



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}

/* Jani Peltotalo: int sdp_parce_c(sdp_t * sdp, char *buf, char **next)
   modified to support IPv6 multicast description without TTL-field, so
   in IPv4 multicast: ip/ttl[/integer], in IPv6 multicast: ip[/integer]]
   and in IPv4/IPv6 unicast: FQDN or ip */

int sdp_parse_c(sdp_t * sdp, char *buf, char **next) {

	char *equal;
	char *crlf;
	char *tmp;
	char *tmp_next;
	char *slash;

	sdp_connection_t *c_header;

	int i;

	*next = buf;
	equal = buf;

	while((*equal != '=') && (*equal != '\0')) {
		equal++;
	}

	if(*equal == '\0') {
		return ERR_ERROR;
	}

	/* check if header is "c" */

	if(equal[-1] != 'c') {
		return ERR_DISCARD;
	}

	crlf = equal + 1;

	while((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0')) {
		crlf++;
	}

	if(*crlf == '\0') {
		return ERR_ERROR;
	}
	
	if(crlf == equal + 1) {
		return ERR_ERROR;		/* c=\r ?? bad header */
	}
	
	tmp = equal + 1;

	i = sdp_connection_init(&c_header);

	if(i != 0) {
		return ERR_ERROR;
	}

	/* c=nettype addrtype (multicastaddr | addr) */

	/* nettype is "IN" and will be extended */

	i = set_next_token(&(c_header->c_nettype), tmp, ' ', &tmp_next);

	if(i != 0) {
		return -1;
	}

	tmp = tmp_next;

	/* nettype is "IP4" or "IP6" and will be extended */

	i = set_next_token(&(c_header->c_addrtype), tmp, ' ', &tmp_next);

	if(i != 0) {
		return -1;
	}

	tmp = tmp_next;

	/* in IPv4 multicast can be ip/ttl[/integer] */
	/* in IPv6 multicast can be ip[/integer]] */
	/* IPv4/IPv6 unicast is FQDN or ip */

	if(strcmp(c_header->c_addrtype, "IP4") == 0) {

		slash = strchr(tmp, '/');

		if(slash != NULL && slash < crlf) {
			/* it's a multicast address! */

			i = set_next_token(&(c_header->c_addr), tmp, '/', &tmp_next);

			if(i != 0) {
				return -1;
			}

			tmp = tmp_next;

			slash = strchr(slash + 1, '/');

			if(slash != NULL && slash < crlf) {
				/* optional integer is there! */

				i = set_next_token(&(c_header->c_addr_multicast_ttl),
									tmp, '/', &tmp_next);

				if(i != 0) {
					return -1;
				}

				tmp = tmp_next;

				i = set_next_token(&(c_header->c_addr_multicast_int),
									tmp, '\r', &tmp_next);

				if(i != 0) {
					i = set_next_token(&(c_header->c_addr_multicast_int),
										tmp, '\n', &tmp_next);

					if(i != 0) {
						sdp_connection_free(c_header);
						free (c_header);
						return -1;
					}
				}
			}
			else {
				i = set_next_token(&(c_header->c_addr_multicast_ttl),
									tmp, '\r', &tmp_next);

				if(i != 0) {
					i = set_next_token(&(c_header->c_addr_multicast_ttl),
									tmp, '\n', &tmp_next);

					if(i != 0) {
						sdp_connection_free(c_header);
						free(c_header);
						return -1;
					}
				}
			}
		}
		else {

			/* in this case, we have a unicast address */

			i = set_next_token(&(c_header->c_addr), tmp, '\r', &tmp_next);

			if(i != 0) {
				i = set_next_token(&(c_header->c_addr), tmp, '\n', &tmp_next);

				if(i != 0) {
					sdp_connection_free(c_header);
					free(c_header);
					return -1;
				}
			}
		}
	}
	else if(strcmp(c_header->c_addrtype, "IP6") == 0) {

		slash = strchr(tmp, '/');

		if(slash != NULL && slash < crlf) {
			/* optional integer is there! */

			i = set_next_token(&(c_header->c_addr), tmp, '/', &tmp_next);

			if(i != 0) {
				return -1;
			}

			tmp = tmp_next;

			i = set_next_token(&(c_header->c_addr_multicast_int),
									tmp, '\r', &tmp_next);

			if(i != 0) {
				i = set_next_token (&(c_header->c_addr_multicast_int),
									tmp, '\n', &tmp_next);

				if(i != 0) {
					sdp_connection_free(c_header);
					free (c_header);
					return -1;
				}
			}
		}
		else {
			i = set_next_token (&(c_header->c_addr), tmp, '\r', &tmp_next);

			if(i != 0) {
				i = set_next_token (&(c_header->c_addr), tmp, '\n', &tmp_next);

				if(i != 0) {
					sdp_connection_free(c_header);
					free(c_header);
					return -1;
				}
			}
		}
	}

	/*  add the connection at the correct place:
		if there is no media line yet, then the "c=" is the
		global one. */

	i = list_size (sdp->m_medias);

	if(i == 0) {
		sdp->c_connection = c_header;
	}
	else {
		sdp_media_t *last_sdp_media = (sdp_media_t *)list_get(sdp->m_medias, i - 1);
		list_add(last_sdp_media->c_connections, c_header, -1);
    }

	if(crlf[1] == '\n') {
		*next = crlf + 2;
	}
	else {
		*next = crlf + 1;
	}

	return WF;
}



int

sdp_parse_b (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *tmp;

  char *tmp_next;

  int i;

  sdp_bandwidth_t *b_header;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "b" */

  if (equal[-1] != 'b')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* b=\r ?? bad header */



  tmp = equal + 1;

  /* b = bwtype: bandwidth */

  i = sdp_bandwidth_init (&b_header);

  if (i != 0)

    return ERR_ERROR;



  /* bwtype is alpha-numeric */

  i = set_next_token (&(b_header->b_bwtype), tmp, ':', &tmp_next);

  if (i != 0)

    return -1;

  tmp = tmp_next;



  i = set_next_token (&(b_header->b_bandwidth), tmp, '\r', &tmp_next);

  if (i != 0)

    {

      i = set_next_token (&(b_header->b_bandwidth), tmp, '\n', &tmp_next);

      if (i != 0)

	{

	  sdp_bandwidth_free (b_header);

	  free (b_header);

	  return -1;

	}

    }



  /* add the bandwidth at the correct place:

     if there is no media line yet, then the "b=" is the

     global one.

   */

  i = list_size (sdp->m_medias);

  if (i == 0)

    list_add (sdp->b_bandwidths, b_header, -1);

  else

    {

      sdp_media_t *last_sdp_media =

	(sdp_media_t *) list_get (sdp->m_medias, i - 1);

      list_add (last_sdp_media->b_bandwidths, b_header, -1);

    }



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_t (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *tmp;

  char *tmp_next;

  int i;

  sdp_time_descr_t *t_header;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "t" */

  if (equal[-1] != 't')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* t=\r ?? bad header */



  tmp = equal + 1;

  /* t = start_time stop_time */

  i = sdp_time_descr_init (&t_header);

  if (i != 0)

    return ERR_ERROR;



  i = set_next_token (&(t_header->t_start_time), tmp, ' ', &tmp_next);

  if (i != 0)

    {

      sdp_time_descr_free (t_header);

      free (t_header);

      return -1;

    }

  tmp = tmp_next;



  i = set_next_token (&(t_header->t_stop_time), tmp, '\r', &tmp_next);

  if (i != 0)

    {

      i = set_next_token (&(t_header->t_stop_time), tmp, '\n', &tmp_next);

      if (i != 0)

	{

	  sdp_time_descr_free (t_header);

	  free (t_header);

	  return -1;

	}

    }



  /* add the new time_description header */

  list_add (sdp->t_descrs, t_header, -1);



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}





int

sdp_parse_r (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  int index;

  char *r_header;

  sdp_time_descr_t *t_descr;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "r" */

  if (equal[-1] != 'r')

    return ERR_DISCARD;



  index = list_size (sdp->t_descrs);

  if (index == 0)

    return ERR_ERROR;		/* r field can't come alone! */



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* r=\r ?? bad header */



  /* r=far too complexe and somewhat useless... I don't parse it! */

  r_header = malloc (crlf - (equal + 1) + 1);

  sstrncpy (r_header, equal + 1, crlf - (equal + 1));



  /* r field carry information for the last "t" field */

  t_descr = (sdp_time_descr_t *) list_get (sdp->t_descrs, index - 1);

  list_add (t_descr->r_repeats, r_header, -1);



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_z (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *z_header;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "z" */

  if (equal[-1] != 'z')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* z=\r ?? bad header */



  /* z=somewhat useless... I don't parse it! */

  z_header = malloc (crlf - (equal + 1) + 1);

  sstrncpy (z_header, equal + 1, crlf - (equal + 1));



  sdp->z_adjustments = z_header;



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_k (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  int i;

  char *colon;

  sdp_key_t *k_header;

  char *tmp;

  char *tmp_next;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "k" */

  if (equal[-1] != 'k')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* k=\r ?? bad header */



  tmp = equal + 1;



  i = sdp_key_init (&k_header);

  if (i != 0)

    return ERR_ERROR;

  /* k=key-type[:key-data] */



  /* is there any key-data? */

  colon = strchr (equal + 1, ':');

  if ((colon != NULL) && (colon < crlf))

    {

      /* att-field is alpha-numeric */

      i = set_next_token (&(k_header->k_keytype), tmp, ':', &tmp_next);

      if (i != 0)

	{

	  sdp_key_free (k_header);

	  free (k_header);

	  return -1;

	}

      tmp = tmp_next;



      i = set_next_token (&(k_header->k_keydata), tmp, '\r', &tmp_next);

      if (i != 0)

	{

	  i = set_next_token (&(k_header->k_keydata), tmp, '\n', &tmp_next);

	  if (i != 0)

	    {

	      sdp_key_free (k_header);

	      free (k_header);

	      return -1;

	    }

	}

    }

  else

    {

      i = set_next_token (&(k_header->k_keytype), tmp, '\r', &tmp_next);

      if (i != 0)

	{

	  i = set_next_token (&(k_header->k_keytype), tmp, '\n', &tmp_next);

	  if (i != 0)

	    {

	      sdp_key_free (k_header);

	      free (k_header);

	      return -1;

	    }

	}

    }



  /* add the key at the correct place:

     if there is no media line yet, then the "k=" is the

     global one.

   */

  i = list_size (sdp->m_medias);

  if (i == 0)

    sdp->k_key = k_header;

  else

    {

      sdp_media_t *last_sdp_media =

	(sdp_media_t *) list_get (sdp->m_medias, i - 1);

      last_sdp_media->k_key = k_header;

    }



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_a (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *tmp;

  char *tmp_next;

  int i;

  sdp_attribute_t *a_attribute;

  char *colon;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "a" */

  if (equal[-1] != 'a')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* a=\r ?? bad header */



  tmp = equal + 1;



  i = sdp_attribute_init (&a_attribute);

  if (i != 0)

    return ERR_ERROR;



  /* a=att-field[:att-value] */



  /* is there any att-value? */

  colon = strchr (equal + 1, ':');

  if ((colon != NULL) && (colon < crlf))

    {

      /* att-field is alpha-numeric */

      i = set_next_token (&(a_attribute->a_att_field), tmp, ':', &tmp_next);

      if (i != 0)

	{

	  sdp_attribute_free (a_attribute);

	  free (a_attribute);

	  return -1;

	}

      tmp = tmp_next;



      i = set_next_token (&(a_attribute->a_att_value), tmp, '\r', &tmp_next);

      if (i != 0)

	{

	  i =

	    set_next_token (&(a_attribute->a_att_value), tmp, '\n',

			    &tmp_next);

	  if (i != 0)

	    {

	      sdp_attribute_free (a_attribute);

	      free (a_attribute);

	      return -1;

	    }

	}

    }

  else

    {

      i = set_next_token (&(a_attribute->a_att_field), tmp, '\r', &tmp_next);

      if (i != 0)

	{

	  i =

	    set_next_token (&(a_attribute->a_att_field), tmp, '\n',

			    &tmp_next);

	  if (i != 0)

	    {

	      sdp_attribute_free (a_attribute);

	      free (a_attribute);

	      return -1;

	    }

	}

    }



  /* add the attribute at the correct place:

     if there is no media line yet, then the "a=" is the

     global one.

   */

  i = list_size (sdp->m_medias);

  if (i == 0)

    list_add (sdp->a_attributes, a_attribute, -1);

  else

    {

      sdp_media_t *last_sdp_media =

	(sdp_media_t *) list_get (sdp->m_medias, i - 1);

      list_add (last_sdp_media->a_attributes, a_attribute, -1);

    }



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}



int

sdp_parse_m (sdp_t * sdp, char *buf, char **next)

{

  char *equal;

  char *crlf;

  char *tmp;

  char *tmp_next;

  int i;

  sdp_media_t *m_header;

  char *slash;

  char *space;



  *next = buf;



  equal = buf;

  while ((*equal != '=') && (*equal != '\0'))

    equal++;

  if (*equal == '\0')

    return ERR_ERROR;



  /* check if header is "m" */

  if (equal[-1] != 'm')

    return ERR_DISCARD;



  crlf = equal + 1;



  while ((*crlf != '\r') && (*crlf != '\n') && (*crlf != '\0'))

    crlf++;

  if (*crlf == '\0')

    return ERR_ERROR;

  if (crlf == equal + 1)

    return ERR_ERROR;		/* a=\r ?? bad header */



  tmp = equal + 1;



  i = sdp_media_init (&m_header);

  if (i != 0)

    return ERR_ERROR;



  /* m=media port ["/"integer] proto *(payload_number) */



  /* media is "audio" "video" "application" "data" or other... */

  i = set_next_token (&(m_header->m_media), tmp, ' ', &tmp_next);

  if (i != 0)

    {

      sdp_media_free (m_header);

      free (m_header);

      return -1;

    }

  tmp = tmp_next;



  slash = strchr (tmp, '/');

  space = strchr (tmp, ' ');

  if (space == NULL)		/* not possible! */

    {

      sdp_media_free (m_header);

      free (m_header);

      return ERR_ERROR;

    }

  if ((slash != NULL) && (slash < space))

    {				/* a number of port is specified! */

      i = set_next_token (&(m_header->m_port), tmp, '/', &tmp_next);

      if (i != 0)

	{

	  sdp_media_free (m_header);

	  free (m_header);

	  return -1;

	}

      tmp = tmp_next;



      i = set_next_token (&(m_header->m_number_of_port), tmp, ' ', &tmp_next);

      if (i != 0)

	{

	  sdp_media_free (m_header);

	  free (m_header);

	  return -1;

	}

      tmp = tmp_next;

    }

  else

    {

      i = set_next_token (&(m_header->m_port), tmp, ' ', &tmp_next);

      if (i != 0)

	{

	  sdp_media_free (m_header);

	  free (m_header);

	  return -1;

	}

      tmp = tmp_next;

    }



  i = set_next_token (&(m_header->m_proto), tmp, ' ', &tmp_next);

  if (i != 0)

    {

      sdp_media_free (m_header);

      free (m_header);

      return -1;

    }

  tmp = tmp_next;



  {

    char *str;

    int more_space_before_crlf;



    space = strchr (tmp + 1, ' ');

    if (space == NULL)

      more_space_before_crlf = 1;

    else if ((space != NULL) && (space > crlf))

      more_space_before_crlf = 1;

    else

      more_space_before_crlf = 0;

    while (more_space_before_crlf == 0)

      {

	i = set_next_token (&str, tmp, ' ', &tmp_next);

	if (i != 0)

	  {

	    sdp_media_free (m_header);

	    free (m_header);

	    return -1;

	  }

	tmp = tmp_next;

	list_add (m_header->m_payloads, str, -1);



	space = strchr (tmp + 1, ' ');

	if (space == NULL)

	  more_space_before_crlf = 1;

	else if ((space != NULL) && (space > crlf))

	  more_space_before_crlf = 1;

	else

	  more_space_before_crlf = 0;

      }

    if (tmp_next < crlf)	// tmp_next is still less than clrf: no space

      {

	i = set_next_token (&str, tmp, '\r', &tmp_next);

	if (i != 0)

	  {

	    i = set_next_token (&str, tmp, '\n', &tmp_next);

	    if (i != 0)

	      {

		sdp_media_free (m_header);

		free (m_header);

		return -1;

	      }

	  }

	list_add (m_header->m_payloads, str, -1);

      }

  }



  list_add (sdp->m_medias, m_header, -1);



  if (crlf[1] == '\n')

    *next = crlf + 2;

  else

    *next = crlf + 1;

  return WF;

}





int

sdp_parse (sdp_t * sdp, const char *buf)

{



  /* In SDP, headers must be in the right order */

  /* This is a simple example

     v=0

     o=user1 53655765 2353687637 IN IP4 128.3.4.5

     s=Mbone Audio

     i=Discussion of Mbone Engineering Issues

     e=mbone@somewhere.com

     c=IN IP4 224.2.0.1/127

     t=0 0

     m=audio 3456 RTP/AVP 0

     a=rtpmap:0 PCMU/8000

   */



  char *next_buf;

  char *ptr;

  int i;



  ptr = (char *) buf;

  /* mandatory */

  i = sdp_parse_v (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  else if (0 == i)		/* header is not "v" */

    return -1;

  ptr = next_buf;



  /* adtech phone use the wrong ordering and place "s" before "o" */

  i = sdp_parse_s (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  /* else if (0==i) header is not "s" */

  /* else ADTECH PHONE DETECTED */



  ptr = next_buf;







  i = sdp_parse_o (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  else if (0 == i)		/* header is not "o" */

    return -1;

  ptr = next_buf;



  i = sdp_parse_s (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  else if (0 == i)		/* header is not "s" */

    /* return -1; */

    {

      OSIP_TRACE (osip_trace

		  (__FILE__, __LINE__, OSIP_INFO4, NULL,

		   "The \"s\" parameter is mandatory, but this packet does not contain any! - anyway, we don't mind about it.\n"));

    }

  ptr = next_buf;



  i = sdp_parse_i (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  ptr = next_buf;



  i = sdp_parse_u (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  ptr = next_buf;



  i = 1;

  while (i == 1)

    {

      i = sdp_parse_e (sdp, ptr, &next_buf);

      if (i == -1)		/* header is bad */

	return -1;

      ptr = next_buf;

    }

  i = 1;

  while (i == 1)

    {

      i = sdp_parse_p (sdp, ptr, &next_buf);

      if (i == -1)		/* header is bad */

	return -1;

      ptr = next_buf;

    }



  /* rfc2327: there should be at least of email or phone number! */

  if (list_size (sdp->e_emails) == 0 && list_size (sdp->p_phones) == 0)

    {

      OSIP_TRACE (osip_trace

		  (__FILE__, __LINE__, OSIP_INFO4, NULL,

		   "The rfc2327 says there should be at least an email or a phone header!- anyway, we don't mind about it.\n"));

    }



  i = sdp_parse_c (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  ptr = next_buf;



  i = 1;

  while (i == 1)

    {

      i = sdp_parse_b (sdp, ptr, &next_buf);

      if (i == -1)		/* header is bad */

	return -1;

      ptr = next_buf;

    }



  /* 1 or more "t" header + 0 or more "r" header for each "t" header */

  i = sdp_parse_t (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  else if (i == ERR_DISCARD)

    return -1;			/* t is mandatory */

  ptr = next_buf;



  if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

    return 0;



  i = 1;

  while (i == 1)		/* is a "r" header */

    {

      i = sdp_parse_r (sdp, ptr, &next_buf);

      if (i == -1)		/* header is bad */

	return -1;

      ptr = next_buf;

      if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	return 0;



    }





  {

    int more_t_header = 1;



    i = sdp_parse_t (sdp, ptr, &next_buf);

    if (i == -1)		/* header is bad */

	  return -1;

    ptr = next_buf;



    if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

      return 0;



    while (more_t_header == 1)

      {

	i = 1;

	while (i == 1)		/* is a "r" header */

	  {

	    i = sdp_parse_r (sdp, ptr, &next_buf);

	    if (i == -1)	/* header is bad */

	      return -1;

	    ptr = next_buf;

	    if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	      return 0;

	  }



	i = sdp_parse_t (sdp, ptr, &next_buf);

	if (i == -1)		/* header is bad */

	  return -1;

	else if (i == ERR_DISCARD)

	  more_t_header = 0;

	else

	  more_t_header = 1;	/* no more "t" headers */

	ptr = next_buf;

	if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	  return 0;

      }

  }



  i = sdp_parse_z (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  ptr = next_buf;

  if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

    return 0;



  i = sdp_parse_k (sdp, ptr, &next_buf);

  if (i == -1)			/* header is bad */

    return -1;

  ptr = next_buf;

  if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

    return 0;



  /* 0 or more "a" header */

  i = 1;

  while (i == 1)		/* no more "a" header */

    {

      i = sdp_parse_a (sdp, ptr, &next_buf);

      if (i == -1)		/* header is bad */

	return -1;

      ptr = next_buf;

      if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	return 0;

    }

  /* 0 or more media headers */

  {

    int more_m_header = 1;



    while (more_m_header == 1)

      {

	more_m_header = sdp_parse_m (sdp, ptr, &next_buf);

	if (more_m_header == -1)	/* header is bad */

	  return -1;

	ptr = next_buf;

	if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	  return 0;



	i = sdp_parse_i (sdp, ptr, &next_buf);

	if (i == -1)		/* header is bad */

	  return -1;

	ptr = next_buf;

	if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	  return 0;



	i = 1;

	while (i == 1)

	  {

	    i = sdp_parse_c (sdp, ptr, &next_buf);

	    if (i == -1)	/* header is bad */

	      return -1;

	    ptr = next_buf;

	    if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	      return 0;

	  }



	i = 1;

	while (i == 1)

	  {

	    i = sdp_parse_b (sdp, ptr, &next_buf);

	    if (i == -1)	/* header is bad */

	      return -1;

	    ptr = next_buf;

	    if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	      return 0;

	  }

	i = sdp_parse_k (sdp, ptr, &next_buf);

	if (i == -1)		/* header is bad */

	  return -1;

	ptr = next_buf;

	if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	  return 0;

	/* 0 or more a headers */

	i = 1;

	while (i == 1)

	  {

	    i = sdp_parse_a (sdp, ptr, &next_buf);

	    if (i == -1)	/* header is bad */

	      return -1;

	    ptr = next_buf;

	    if (*ptr == '\0' || (*ptr == '\r') || (*ptr == '\n'))

	      return 0;

	  }

      }

  }



  return 0;

}



/* internal facility */

int

sdp_append_connection (char *string, int size, char *tmp,

		       sdp_connection_t * conn, char **next_tmp)

{

  if (conn->c_nettype == NULL)

    return -1;

  if (conn->c_addrtype == NULL)

    return -1;

  if (conn->c_addr == NULL)

    return -1;



  tmp = sdp_append_string (string, size, tmp, "c=");

  tmp = sdp_append_string (string, size, tmp, conn->c_nettype);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, conn->c_addrtype);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, conn->c_addr);

  if (conn->c_addr_multicast_ttl != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "/");

      tmp = sdp_append_string (string, size, tmp, conn->c_addr_multicast_ttl);

    }

  if (conn->c_addr_multicast_int != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "/");

      tmp = sdp_append_string (string, size, tmp, conn->c_addr_multicast_int);

    }

  tmp = sdp_append_string (string, size, tmp, CRLF);

  *next_tmp = tmp;

  return 0;

}



/* internal facility */

int

sdp_append_bandwidth (char *string, int size, char *tmp,

		      sdp_bandwidth_t * bandwidth, char **next_tmp)

{

  if (bandwidth->b_bwtype == NULL)

    return -1;

  if (bandwidth->b_bandwidth == NULL)

    return -1;



  tmp = sdp_append_string (string, size, tmp, "b=");

  tmp = sdp_append_string (string, size, tmp, bandwidth->b_bwtype);

  tmp = sdp_append_string (string, size, tmp, ":");

  tmp = sdp_append_string (string, size, tmp, bandwidth->b_bandwidth);

  tmp = sdp_append_string (string, size, tmp, CRLF);



  *next_tmp = tmp;

  return 0;

}



int

sdp_append_time_descr (char *string, int size, char *tmp,

		       sdp_time_descr_t * time_descr, char **next_tmp)

{

  int pos;



  if (time_descr->t_start_time == NULL)

    return -1;

  if (time_descr->t_stop_time == NULL)

    return -1;





  tmp = sdp_append_string (string, size, tmp, "t=");

  tmp = sdp_append_string (string, size, tmp, time_descr->t_start_time);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, time_descr->t_stop_time);



  tmp = sdp_append_string (string, size, tmp, CRLF);



  pos = 0;

  while (!list_eol (time_descr->r_repeats, pos))

    {

      char *str = (char *) list_get (time_descr->r_repeats, pos);



      tmp = sdp_append_string (string, size, tmp, "r=");

      tmp = sdp_append_string (string, size, tmp, str);

      tmp = sdp_append_string (string, size, tmp, CRLF);

      pos++;

    }



  *next_tmp = tmp;

  return 0;

}



int

sdp_append_key (char *string, int size, char *tmp, sdp_key_t * key,

		char **next_tmp)

{

  if (key->k_keytype == NULL)

    return -1;



  tmp = sdp_append_string (string, size, tmp, "k=");

  tmp = sdp_append_string (string, size, tmp, key->k_keytype);

  if (key->k_keydata != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, ":");

      tmp = sdp_append_string (string, size, tmp, key->k_keydata);

    }

  tmp = sdp_append_string (string, size, tmp, CRLF);

  *next_tmp = tmp;

  return 0;

}



/* internal facility */

int

sdp_append_attribute (char *string, int size, char *tmp,

		      sdp_attribute_t * attribute, char **next_tmp)

{

  if (attribute->a_att_field == NULL)

    return -1;



  tmp = sdp_append_string (string, size, tmp, "a=");

  tmp = sdp_append_string (string, size, tmp, attribute->a_att_field);

  if (attribute->a_att_value != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, ":");

      tmp = sdp_append_string (string, size, tmp, attribute->a_att_value);

    }

  tmp = sdp_append_string (string, size, tmp, CRLF);



  *next_tmp = tmp;

  return 0;

}



/* internal facility */

int

sdp_append_media (char *string, int size, char *tmp, sdp_media_t * media,

		  char **next_tmp)

{

  int pos;



  if (media->m_media == NULL)

    return -1;

  if (media->m_port == NULL)

    return -1;

  if (media->m_proto == NULL)

    return -1;



  tmp = sdp_append_string (string, size, tmp, "m=");

  tmp = sdp_append_string (string, size, tmp, media->m_media);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, media->m_port);

  if (media->m_number_of_port != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "/");

      tmp = sdp_append_string (string, size, tmp, media->m_number_of_port);

    }

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, media->m_proto);

  pos = 0;

  while (!list_eol (media->m_payloads, pos))

    {

      char *str = (char *) list_get (media->m_payloads, pos);



      tmp = sdp_append_string (string, size, tmp, " ");

      tmp = sdp_append_string (string, size, tmp, str);

      pos++;

    }

  tmp = sdp_append_string (string, size, tmp, CRLF);



  if (media->i_info != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "i=");

      tmp = sdp_append_string (string, size, tmp, media->i_info);

      tmp = sdp_append_string (string, size, tmp, CRLF);

    }



  pos = 0;

  while (!list_eol (media->c_connections, pos))

    {

      sdp_connection_t *conn =

	(sdp_connection_t *) list_get (media->c_connections, pos);

      char *next_tmp2;

      int i;



      i = sdp_append_connection (string, size, tmp, conn, &next_tmp2);

      if (i != 0)

	return -1;

      tmp = next_tmp2;

      pos++;

    }



  pos = 0;

  while (!list_eol (media->b_bandwidths, pos))

    {

      sdp_bandwidth_t *band =

	(sdp_bandwidth_t *) list_get (media->b_bandwidths, pos);

      char *next_tmp2;

      int i;



      i = sdp_append_bandwidth (string, size, tmp, band, &next_tmp2);

      if (i != 0)

	return -1;

      tmp = next_tmp2;

      pos++;

    }



  if (media->k_key != NULL)

    {

      char *next_tmp2;

      int i;



      i = sdp_append_key (string, size, tmp, media->k_key, &next_tmp2);

      if (i != 0)

	return -1;

      tmp = next_tmp2;

    }



  pos = 0;

  while (!list_eol (media->a_attributes, pos))

    {

      sdp_attribute_t *attr =

	(sdp_attribute_t *) list_get (media->a_attributes, pos);

      char *next_tmp2;

      int i;



      i = sdp_append_attribute (string, size, tmp, attr, &next_tmp2);

      if (i != 0)

	return -1;

      tmp = next_tmp2;

      pos++;

    }



  *next_tmp = tmp;

  return 0;

}



int

sdp_2char (sdp_t * sdp, char **dest)

{

  int size;

  int pos;

  char *tmp;

  char *string;



  *dest = NULL;

  if (sdp->v_version == NULL)

    return -1;

  if (sdp->o_username == NULL ||

      sdp->o_sess_id == NULL ||

      sdp->o_sess_version == NULL ||

      sdp->o_nettype == NULL || sdp->o_addrtype == NULL

      || sdp->o_addr == NULL)

    return -1;



  /* RFC says "s=" is mandatory... rfc2543 (SIP) recommends to

     accept SDP datas without s_name... as some buggy implementations

     often forget it...

   */

  /* if (sdp->s_name == NULL)

     return -1; */



  size = 4000;

  tmp = (char *) malloc (size);

  string = tmp;



  tmp = sdp_append_string (string, size, tmp, "v=");

  tmp = sdp_append_string (string, size, tmp, sdp->v_version);

  tmp = sdp_append_string (string, size, tmp, CRLF);

  tmp = sdp_append_string (string, size, tmp, "o=");

  tmp = sdp_append_string (string, size, tmp, sdp->o_username);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, sdp->o_sess_id);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, sdp->o_sess_version);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, sdp->o_nettype);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, sdp->o_addrtype);

  tmp = sdp_append_string (string, size, tmp, " ");

  tmp = sdp_append_string (string, size, tmp, sdp->o_addr);

  tmp = sdp_append_string (string, size, tmp, CRLF);

  if (sdp->s_name != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "s=");

      tmp = sdp_append_string (string, size, tmp, sdp->s_name);

      tmp = sdp_append_string (string, size, tmp, CRLF);

    }

  if (sdp->i_info != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "i=");

      tmp = sdp_append_string (string, size, tmp, sdp->i_info);

      tmp = sdp_append_string (string, size, tmp, CRLF);

    }

  if (sdp->u_uri != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "u=");

      tmp = sdp_append_string (string, size, tmp, sdp->u_uri);

      tmp = sdp_append_string (string, size, tmp, CRLF);

    }

  pos = 0;

  while (!list_eol (sdp->e_emails, pos))

    {

      char *email = (char *) list_get (sdp->e_emails, pos);



      tmp = sdp_append_string (string, size, tmp, "e=");

      tmp = sdp_append_string (string, size, tmp, email);

      tmp = sdp_append_string (string, size, tmp, CRLF);

      pos++;

    }

  pos = 0;

  while (!list_eol (sdp->p_phones, pos))

    {

      char *phone = (char *) list_get (sdp->p_phones, pos);



      tmp = sdp_append_string (string, size, tmp, "p=");

      tmp = sdp_append_string (string, size, tmp, phone);

      tmp = sdp_append_string (string, size, tmp, CRLF);

      pos++;

    }

  if (sdp->c_connection != NULL)

    {

      char *next_tmp;

      int i;



      i =

	sdp_append_connection (string, size, tmp, sdp->c_connection,

			       &next_tmp);

      if (i != 0)

	return -1;

      tmp = next_tmp;

    }

  pos = 0;

  while (!list_eol (sdp->b_bandwidths, pos))

    {

      sdp_bandwidth_t *header =

	(sdp_bandwidth_t *) list_get (sdp->b_bandwidths, pos);

      char *next_tmp;

      int i;



      i = sdp_append_bandwidth (string, size, tmp, header, &next_tmp);

      if (i != 0)

	return -1;

      tmp = next_tmp;

      pos++;

    }



  pos = 0;

  while (!list_eol (sdp->t_descrs, pos))

    {

      sdp_time_descr_t *header =

	(sdp_time_descr_t *) list_get (sdp->t_descrs, pos);

      char *next_tmp;

      int i;



      i = sdp_append_time_descr (string, size, tmp, header, &next_tmp);

      if (i != 0)

	return -1;

      tmp = next_tmp;

      pos++;

    }



  if (sdp->z_adjustments != NULL)

    {

      tmp = sdp_append_string (string, size, tmp, "z=");

      tmp = sdp_append_string (string, size, tmp, sdp->z_adjustments);

      tmp = sdp_append_string (string, size, tmp, CRLF);

    }



  if (sdp->k_key != NULL)

    {

      char *next_tmp;

      int i;



      i = sdp_append_key (string, size, tmp, sdp->k_key, &next_tmp);

      if (i != 0)

	return -1;

      tmp = next_tmp;

    }



  pos = 0;

  while (!list_eol (sdp->a_attributes, pos))

    {

      sdp_attribute_t *header =

	(sdp_attribute_t *) list_get (sdp->a_attributes, pos);

      char *next_tmp;

      int i;



      i = sdp_append_attribute (string, size, tmp, header, &next_tmp);

      if (i != 0)

	return -1;

      tmp = next_tmp;

      pos++;

    }



  pos = 0;

  while (!list_eol (sdp->m_medias, pos))

    {

      sdp_media_t *header = (sdp_media_t *) list_get (sdp->m_medias, pos);

      char *next_tmp;

      int i;



      i = sdp_append_media (string, size, tmp, header, &next_tmp);

      if (i != 0)

	return -1;

      tmp = next_tmp;

      pos++;

    }

  *dest = string;

  return 0;

}



void

sdp_free (sdp_t * sdp)

{

  if (sdp == NULL)

    return;

  free (sdp->v_version);

  free (sdp->o_username);

  free (sdp->o_sess_id);

  free (sdp->o_sess_version);

  free (sdp->o_nettype);

  free (sdp->o_addrtype);

  free (sdp->o_addr);

  free (sdp->s_name);

  free (sdp->i_info);

  free (sdp->u_uri);



  listofchar_free (sdp->e_emails);

  free (sdp->e_emails);



  listofchar_free (sdp->p_phones);

  free (sdp->p_phones);



  sdp_connection_free (sdp->c_connection);

  free (sdp->c_connection);



  list_special_free (sdp->b_bandwidths,

		     (void *(*)(void *)) &sdp_bandwidth_free);

  free (sdp->b_bandwidths);



  list_special_free (sdp->t_descrs, (void *(*)(void *)) &sdp_time_descr_free);

  free (sdp->t_descrs);



  free (sdp->z_adjustments);

  sdp_key_free (sdp->k_key);

  free (sdp->k_key);



  list_special_free (sdp->a_attributes,

		     (void *(*)(void *)) &sdp_attribute_free);

  free (sdp->a_attributes);



  list_special_free (sdp->m_medias, (void *(*)(void *)) &sdp_media_free);

  free (sdp->m_medias);

}



int

sdp_clone (sdp_t * sdp, sdp_t ** dest)

{

  int i;

  char *body;



  i = sdp_init (dest);

  if (i != 0)

    return -1;



  i = sdp_2char (sdp, &body);

  if (i != 0)

    goto error_sc1;



  i = sdp_parse (*dest, body);

  free (body);

  if (i != 0)

    goto error_sc1;



  return 0;



error_sc1:

  sdp_free (*dest);

  free (*dest);

  return -1;

}
