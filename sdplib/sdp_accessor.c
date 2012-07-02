/* $Author: peltotas $ $Date: 2004/01/27 12:19:06 $ $Revision: 1.2 $ */
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

#include "const.h"

#include "sdp.h"

#include "port.h"



#include <stdio.h>

#include <stdlib.h>



int

sdp_v_version_set (sdp_t * sdp, char *v_version)

{

  if (sdp == NULL)

    return -1;

  sdp->v_version = v_version;

  return 0;

}



char *

sdp_v_version_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->v_version;

}



int

sdp_o_origin_set (sdp_t * sdp, char *username, char *sess_id,

		  char *sess_version, char *nettype, char *addrtype,

		  char *addr)

{

  if (sdp == NULL)

    return -1;

  sdp->o_username = username;

  sdp->o_sess_id = sess_id;

  sdp->o_sess_version = sess_version;

  sdp->o_nettype = nettype;

  sdp->o_addrtype = addrtype;

  sdp->o_addr = addr;

  return 0;

}



char *

sdp_o_username_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->o_username;

}



char *

sdp_o_sess_id_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->o_sess_id;

}



char *

sdp_o_sess_version_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->o_sess_version;

}



char *

sdp_o_nettype_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->o_nettype;

}



char *

sdp_o_addrtype_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->o_addrtype;

}



char *

sdp_o_addr_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->o_addr;

}



int

sdp_s_name_set (sdp_t * sdp, char *name)

{

  if (sdp == NULL)

    return -1;

  sdp->s_name = name;

  return 0;

}



char *

sdp_s_name_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->s_name;

}



int

sdp_i_info_set (sdp_t * sdp, int pos_media, char *info)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return -1;

  if (pos_media == -1)

    {

      sdp->i_info = info;

      return 0;

    }

  med = list_get (sdp->m_medias, pos_media);

  if (med == NULL)

    return -1;

  med->i_info = info;

  return 0;

}



char *

sdp_i_info_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return NULL;

  if (pos_media == -1)

    {

      return sdp->i_info;

    }

  med = list_get (sdp->m_medias, pos_media);

  if (med == NULL)

    return NULL;

  return sdp->i_info;

}



int

sdp_u_uri_set (sdp_t * sdp, char *uri)

{

  if (sdp == NULL)

    return -1;

  sdp->u_uri = uri;

  return 0;

}



char *

sdp_u_uri_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->u_uri;

}



int

sdp_e_email_add (sdp_t * sdp, char *email)

{

  if (sdp == NULL)

    return -1;

  list_add (sdp->e_emails, email, -1);

  return 0;

}



char *

sdp_e_email_get (sdp_t * sdp, int pos)

{

  if (sdp == NULL)

    return NULL;

  if (list_size (sdp->e_emails) > pos)

    return (char *) list_get (sdp->e_emails, pos);

  return NULL;

}



int

sdp_p_phone_add (sdp_t * sdp, char *phone)

{

  if (sdp == NULL)

    return -1;

  list_add (sdp->p_phones, phone, -1);

  return 0;

}



char *

sdp_p_phone_get (sdp_t * sdp, int pos)

{

  if (sdp == NULL)

    return NULL;

  if (list_size (sdp->p_phones) > pos)

    return (char *) list_get (sdp->p_phones, pos);

  return NULL;

}



int

sdp_c_connection_add (sdp_t * sdp, int pos_media,

		      char *nettype, char *addrtype,

		      char *addr, char *addr_multicast_ttl,

		      char *addr_multicast_int)

{

  int i;

  sdp_media_t *med;

  sdp_connection_t *conn;



  if (sdp == NULL)

    return -1;

  if ((pos_media != -1) && (list_size (sdp->m_medias) < pos_media + 1))

    return -1;

  i = sdp_connection_init (&conn);

  if (i != 0)

    return -1;

  conn->c_nettype = nettype;

  conn->c_addrtype = addrtype;

  conn->c_addr = addr;

  conn->c_addr_multicast_ttl = addr_multicast_ttl;

  conn->c_addr_multicast_int = addr_multicast_int;

  if (pos_media == -1)

    {

      sdp->c_connection = conn;

      return 0;

    }

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  list_add (med->c_connections, conn, -1);

  return 0;

}



/* this method should be internal only... */

sdp_connection_t *

sdp_connection_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return NULL;

  if (pos_media == -1)		/* pos is useless in this case: 1 global "c=" is allowed */

    return sdp->c_connection;

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  if (med == NULL)

    return NULL;

  return (sdp_connection_t *) list_get (med->c_connections, pos);

}



char *

sdp_c_nettype_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_connection_t *conn = sdp_connection_get (sdp, pos_media, pos);



  if (conn == NULL)

    return NULL;

  return conn->c_nettype;

}



char *

sdp_c_addrtype_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_connection_t *conn = sdp_connection_get (sdp, pos_media, pos);



  if (conn == NULL)

    return NULL;

  return conn->c_addrtype;

}



char *

sdp_c_addr_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_connection_t *conn = sdp_connection_get (sdp, pos_media, pos);



  if (conn == NULL)

    return NULL;

  return conn->c_addr;

}



char *

sdp_c_addr_multicast_ttl_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_connection_t *conn = sdp_connection_get (sdp, pos_media, pos);



  if (conn == NULL)

    return NULL;

  return conn->c_addr_multicast_ttl;

}



char *

sdp_c_addr_multicast_int_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_connection_t *conn = sdp_connection_get (sdp, pos_media, pos);



  if (conn == NULL)

    return NULL;

  return conn->c_addr_multicast_int;

}



int

sdp_b_bandwidth_add (sdp_t * sdp, int pos_media, char *bwtype,

		     char *bandwidth)

{

  int i;

  sdp_media_t *med;

  sdp_bandwidth_t *band;



  if (sdp == NULL)

    return -1;

  if ((pos_media != -1) && (list_size (sdp->m_medias) < pos_media + 1))

    return -1;

  i = sdp_bandwidth_init (&band);

  if (i != 0)

    return -1;

  band->b_bwtype = bwtype;

  band->b_bandwidth = bandwidth;

  if (pos_media == -1)

    {

      list_add (sdp->b_bandwidths, band, -1);

      return 0;

    }

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  list_add (med->b_bandwidths, band, -1);

  return 0;

}



sdp_bandwidth_t *

sdp_bandwidth_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return NULL;

  if (pos_media == -1)

    return (sdp_bandwidth_t *) list_get (sdp->b_bandwidths, pos);

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  if (med == NULL)

    return NULL;

  return (sdp_bandwidth_t *) list_get (med->b_bandwidths, pos);

}



char *

sdp_b_bwtype_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_bandwidth_t *band = sdp_bandwidth_get (sdp, pos_media, pos);



  if (band == NULL)

    return NULL;

  return band->b_bwtype;

}



char *

sdp_b_bandwidth_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_bandwidth_t *band = sdp_bandwidth_get (sdp, pos_media, pos);



  if (band == NULL)

    return NULL;

  return band->b_bandwidth;

}



int

sdp_t_time_descr_add (sdp_t * sdp, char *start, char *stop)

{

  int i;

  sdp_time_descr_t *td;



  if (sdp == NULL)

    return -1;

  i = sdp_time_descr_init (&td);

  if (i != 0)

    return -1;

  td->t_start_time = start;

  td->t_stop_time = stop;

  list_add (sdp->t_descrs, td, -1);

  return 0;

}



char *

sdp_t_start_time_get (sdp_t * sdp, int pos_td)

{

  sdp_time_descr_t *td;



  if (sdp == NULL)

    return NULL;

  td = (sdp_time_descr_t *) list_get (sdp->t_descrs, pos_td);

  if (td == NULL)

    return NULL;

  return td->t_start_time;

}



char *

sdp_t_stop_time_get (sdp_t * sdp, int pos_td)

{

  sdp_time_descr_t *td;



  if (sdp == NULL)

    return NULL;

  td = (sdp_time_descr_t *) list_get (sdp->t_descrs, pos_td);

  if (td == NULL)

    return NULL;

  return td->t_stop_time;

}



int

sdp_r_repeat_add (sdp_t * sdp, int pos_time_descr, char *field)

{

  sdp_time_descr_t *td;



  if (sdp == NULL)

    return -1;

  td = (sdp_time_descr_t *) list_get (sdp->t_descrs, pos_time_descr);

  if (td == NULL)

    return -1;

  list_add (td->r_repeats, field, -1);

  return 0;

}



char *

sdp_r_repeat_get (sdp_t * sdp, int pos_time_descr, int pos_repeat)

{

  sdp_time_descr_t *td;



  if (sdp == NULL)

    return NULL;

  td = (sdp_time_descr_t *) list_get (sdp->t_descrs, pos_time_descr);

  if (td == NULL)

    return NULL;

  return (char *) list_get (td->r_repeats, pos_repeat);

}



int

sdp_z_adjustments_set (sdp_t * sdp, char *field)

{

  if (sdp == NULL)

    return -1;

  sdp->z_adjustments = field;

  return 0;

}



char *

sdp_z_adjustments_get (sdp_t * sdp)

{

  if (sdp == NULL)

    return NULL;

  return sdp->z_adjustments;

}



int

sdp_k_key_set (sdp_t * sdp, int pos_media, char *keytype, char *keydata)

{

  sdp_key_t *key;

  sdp_media_t *med;



  if (sdp == NULL)

    return -1;

  if ((pos_media != -1) && (list_size (sdp->m_medias) < pos_media + 1))

    return -1;

  sdp_key_init (&key);

  key->k_keytype = keytype;

  key->k_keydata = keydata;

  if (pos_media == -1)

    {

      sdp->k_key = key;

      return 0;

    }

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  med->k_key = key;

  return 0;

}



char *

sdp_k_keytype_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return NULL;

  if (pos_media == -1)

    {

      if (sdp->k_key == NULL)

	return NULL;

      return sdp->k_key->k_keytype;

    }

  if ((pos_media != -1) && (list_size (sdp->m_medias) < pos_media + 1))

    return NULL;

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  if (med->k_key == NULL)

    return NULL;

  return med->k_key->k_keytype;

}



char *

sdp_k_keydata_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return NULL;

  if (pos_media == -1)

    {

      if (sdp->k_key == NULL)

	return NULL;

      return sdp->k_key->k_keydata;

    }

  if ((pos_media != -1) && (list_size (sdp->m_medias) < pos_media + 1))

    return NULL;

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  if (med->k_key == NULL)

    return NULL;

  return med->k_key->k_keydata;

}



int

sdp_a_attribute_add (sdp_t * sdp, int pos_media, char *att_field,

		     char *att_value)

{

  int i;

  sdp_media_t *med;

  sdp_attribute_t *attr;



  if (sdp == NULL)

    return -1;

  if ((pos_media != -1) && (list_size (sdp->m_medias) < pos_media + 1))

    return -1;

  i = sdp_attribute_init (&attr);

  if (i != 0)

    return -1;

  attr->a_att_field = att_field;

  attr->a_att_value = att_value;

  if (pos_media == -1)

    {

      list_add (sdp->a_attributes, attr, -1);

      return 0;

    }

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  list_add (med->a_attributes, attr, -1);

  return 0;

}



sdp_attribute_t *

sdp_attribute_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_media_t *med;



  if (sdp == NULL)

    return NULL;

  if (pos_media == -1)

    return (sdp_attribute_t *) list_get (sdp->a_attributes, pos);

  med = (sdp_media_t *) list_get (sdp->m_medias, pos_media);

  if (med == NULL)

    return NULL;

  return (sdp_attribute_t *) list_get (med->a_attributes, pos);

}



char *

sdp_a_att_field_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_attribute_t *attr = sdp_attribute_get (sdp, pos_media, pos);



  if (attr == NULL)

    return NULL;

  return attr->a_att_field;

}



char *

sdp_a_att_value_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_attribute_t *attr = sdp_attribute_get (sdp, pos_media, pos);



  if (attr == NULL)

    return NULL;

  return attr->a_att_value;

}



int

sdp_endof_media (sdp_t * sdp, int i)

{

  if (sdp == NULL)

    return -1;

  if (i == -1)

    return 0;

  if (!list_eol (sdp->m_medias, i))

    return 0;			/* not end of list */

  return -1;			/* end of list */

}



int

sdp_m_media_add (sdp_t * sdp, char *media,

		 char *port, char *number_of_port, char *proto)

{

  int i;

  sdp_media_t *med;



  i = sdp_media_init (&med);

  if (i != 0)

    return -1;

  med->m_media = media;

  med->m_port = port;

  med->m_number_of_port = number_of_port;

  med->m_proto = proto;

  list_add (sdp->m_medias, med, -1);

  return 0;

}



char *

sdp_m_media_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med = list_get (sdp->m_medias, pos_media);



  if (med == NULL)

    return NULL;

  return med->m_media;

}



char *

sdp_m_port_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med = list_get (sdp->m_medias, pos_media);



  if (med == NULL)

    return NULL;

  return med->m_port;

}



char *

sdp_m_number_of_port_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med = list_get (sdp->m_medias, pos_media);



  if (med == NULL)

    return NULL;

  return med->m_number_of_port;

}



char *

sdp_m_proto_get (sdp_t * sdp, int pos_media)

{

  sdp_media_t *med = list_get (sdp->m_medias, pos_media);



  if (med == NULL)

    return NULL;

  return med->m_proto;

}



int

sdp_m_payload_add (sdp_t * sdp, int pos_media, char *payload)

{

  sdp_media_t *med = list_get (sdp->m_medias, pos_media);



  if (med == NULL)

    return -1;

  list_add (med->m_payloads, payload, -1);

  return 0;

}



char *

sdp_m_payload_get (sdp_t * sdp, int pos_media, int pos)

{

  sdp_media_t *med = list_get (sdp->m_medias, pos_media);



  if (med == NULL)

    return NULL;

  return (char *) list_get (med->m_payloads, pos);

}
