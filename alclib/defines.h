/* $Author: peltotal $ $Date: 2004/12/28 10:16:12 $ $Revision: 1.81 $ */
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
#include <sys/types.h>

#ifndef _DEFINES_H_
#define _DEFINES_H_

/**** Defines ****/

/* Release version & date */
#define MAD_FCL_RELEASE_VERSION "1.2.2"
#define MAD_FCL_RELEASE_DATE "December 28, 2004"

/* SSM support for WIN XP (IPv4) or for Linux (IPv4 and IPv6) without GNU libc updates */
#define SSM

/* Use ZLIB compression library */
#define USE_ZLIB

/* Use  OpenSSL library for MD5*/
#define USE_OPENSSL

/* With RLC (rlclib), Reed-Solomon FEC (rsfeclib), XOR FEC (xorfeclib) and SDP (sdplib) you
   must also change flute-project's dependencies corresponding to your selections from
   Project->Dependencies in Windows. In Linux you must compile selected libraries before
   compiling flute */

/* Used Concestion Control schemes (zero, one or more are possible) */
#define USE_RLC

/* Used FECs (one or more are possible)*/
#define USE_NULL_FEC /* Mandatory, can be replaced by your own NULL-FEC library */
#define USE_SIMPLE_XOR
#define USE_REED_SOLOMON

/* Use SDP library */
#define USE_SDPLIB

/* FDT Instance FEC OTI place */
#define FDT_INST_FEC_OTI_COMMON
/*#define FDT_INST_FEC_OTI_FILE*/

/* FLUTEv07 spec FDT Instance XML schema format */
/*#define FDT_SCHEMA_FLUTEv07*/

/* bit field order is compiler/OS dependant */
#if defined(SOLARIS) || defined(__ppc__)
#undef _BIT_FIELDS_LTOH
#define _BIT_FIELDS_HTOL
#elif defined(LINUX) || defined(WIN32)
#define _BIT_FIELDS_LTOH
#undef _BIT_FIELDS_HTOL
#endif

#define FDT_TOI		0				/* TOI for FDT */	
#define EXT_FDT		192				/* Extension header defined by FLUTE */
#define EXT_CENC	193				/* Extension header defined by FLUTE */

/* ALC version */
#define ALC_VERSION		1
#define FLUTE_VERSION	1

#define DEF_FEC			0			/* 0 = Null-FEC, 1 = Simple XOR FEC, 2 = Reed-Solomon FEC */

/* FEC schemes */ 
#define COM_NO_C_FEC_ENC_ID		0
#define SIMPLE_XOR_FEC_ENC_ID	2
#define SB_LB_E_FEC_ENC_ID		128
#define SB_SYS_FEC_ENC_ID		129
#define COM_FEC_ENC_ID			130

#ifdef USE_REED_SOLOMON
/* FEC algorithms */
#define REED_SOL_FEC_INST_ID	0
#endif

/* Congestion Control mechanism */
#define Null 0
#ifdef USE_RLC
#define RLC	1
#endif

#ifdef USE_RLC
/* for RLC */
#define RLC_SP_CYCLE		250		/* 250 ms, for fast layer addition */
#define RLC_WAIT_AFTER_SP	2
#define RLC_DEAF_PERIOD		3500	/* 10000 ms, due to IGMP leave latency */
#define RLC_LATE_ACCEPTED	0
#define RLC_LOSS_ACCEPTED	0
#define RLC_PKT_TIMEOUT		500		/* 500 ms */
#define RLC_LOSS_LIMIT		1
#define RLC_LOSS_TIMEOUT	20
#define RLC_MAX_LATES		100
#endif

/* Extension headers defined by LCT */
#define EXT_NOP		0
#define	EXT_AUTH	1

/* Extension header defined by ALC */
#define	EXT_FTI		64

/* Operation modes */
#define SENDER		0
#define RECEIVER	1	

/* Maximum values for some parameters */
#define MAX_ALC_SESSIONS			100
#define MAX_CHANNELS_IN_SESSION		10

/* MAX encoding symbol lengths for IPv4 */
#define MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130		1428	/* calculated for FDT with TSI&TOI 4+4bytes */
#define MAX_SYMB_LENGTH_IPv4_FEC_ID_128_129		1424
/* MAX encoding symbol length for IPv6 */
#define MAX_SYMB_LENGTH_IPv6_FEC_ID_0_130		1408
#define MAX_SYMB_LENGTH_IPv6_FEC_ID_128_129		1404	

#ifdef USE_NULL_FEC
#define MAX_SB_LEN_NULL_FEC			1000
#endif
#ifdef USE_SIMPLE_XOR
#define MAX_SB_LEN_SIMPLE_XOR_FEC	100
#endif
#ifdef USE_REED_SOLOMON
#define MAX_N_REED_SOLOMON			256
#endif

#define MAX_PACKET_LENGTH	1500	/* MTU */
#define MAX_LENGTH			100

/* Default values for some parameters */

#ifdef USE_RLC
#define DEF_CC				Null
#else
#define DEF_CC				Null
#endif

#define DEF_MCAST_IPv4_ADDR		"226.10.40.1"
#define DEF_MCAST_IPv6_ADDR		"ff1e::6181"
#define DEF_MCAST_PORT			"4001"
#define DEF_NB_CHANNEL			1
#define DEF_TSI					1
#define DEF_FDT					"fdt.xml"
#define DEF_SYMB_LENGTH			MAX_SYMB_LENGTH_IPv4_FEC_ID_0_130
#define DEF_MAX_SB_LEN			10			
#define DEF_TX_RATE				250
#define DEF_TTL					15
#define DEF_TX_NB				1
#define DEF_BASE_DIR			"download"
#define DEF_SRC_IPv4_ADDR		"127.0.0.1"
#define DEF_SRC_IPv6_ADDR		"::1"
#define DEF_FEC_ENC_ID			COM_NO_C_FEC_ENC_ID
#ifdef USE_REED_SOLOMON
#define DEF_FEC_INST_ID			REED_SOL_FEC_INST_ID
#else
#define DEF_FEC_INST_ID			0
#endif
#define DEF_FEC_RATIO			20
#define DEF_FILES_IN_FDT_INST	2
#define DEF_ADDR_FAMILY			PF_INET
#define DEF_DURATION			604800 /* one week */

/* Others */
#define OK					4
#define EMPTY_PACKET		3
#define HDR_ERROR			2
#define MEM_ERROR			1
#define DUP_PACKET			0

#ifndef MAX_PATH
	#define MAX_PATH 1024
#endif

#define MAX_LENGTH 100

#define MAX_TX_QUEUE_SIZE 1000 /* default == 1000 */

/* Packet loss probabilities */

#define P_LOSS_WHEN_OK		5
#define P_LOSS_WHEN_LOSS	50

#ifdef USE_ZLIB

/* Content-Encoding Algoriths */

#define ZLIB 1
#define DEFLATE 2
#define GZIP 3

#define ZLIB_BUFLEN	16384

#define GZ_SUFFIX "~gz"
#define GZ_SUFFIX_LEN (sizeof(GZ_SUFFIX)-1)

#define ENCODE_FDT 1
#define ENCODE_FILES 2

#endif

#endif /* _DEFINES_H_ */
