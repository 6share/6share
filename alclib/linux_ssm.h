/* $Author: peltotal $ $Date: 2004/06/18 10:35:13 $ $Revision: 1.3 $ */
/*
 *   MAD-ALC, implementation of ALC/LCT protocols
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

/*
 * All of the below should be in linux/in.h on new enough systems, and
 * so there's an attempt to protect them with an #ifdef
 */

#ifdef LINUX

#ifndef IP_ADD_SOURCE_MEMBERSHIP

/* Order within structures is important, the kernel assumes its own
   structure layout */

struct ip_mreq_source {
  struct in_addr imr_multiaddr;  /* IP address of group */
  struct in_addr imr_interface;  /* IP address of interface */
  struct in_addr imr_sourceaddr; /* IP address of source */
};

#define IP_ADD_SOURCE_MEMBERSHIP        39
#define IP_DROP_SOURCE_MEMBERSHIP       40

#endif

#ifndef MCAST_JOIN_SOURCE_GROUP
   struct group_req {
      uint32_t                gr_interface;	/* interface index */
      struct sockaddr_storage gr_group;     /* group address */
   };

   struct group_source_req {
      uint32_t                gsr_interface; /* interface index */
      struct sockaddr_storage gsr_group;     /* group address */
      struct sockaddr_storage gsr_source;    /* source address */
   };

#define MCAST_JOIN_SOURCE_GROUP  46
#define MCAST_LEAVE_SOURCE_GROUP  47

#endif

#endif

