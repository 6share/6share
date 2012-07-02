/* $Author: peltotal $ $Date: 2004/06/18 06:58:04 $ $Revision: 1.17 $ */
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

#ifndef _INC_H_
#define _INC_H_

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <io.h>
#include <process.h>
#include <direct.h>

#else	/* LINUX/UNIX */

#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/uio.h>
#include <sys/socket.h>

/*#include <linux/in.h>
#include <linux/in6.h>*/

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <dirent.h>
/*#include <mpatrol.h>*/

#endif

#include "defines.h"

#if defined(LINUX) && defined(SSM)
#include "linux_ssm.h"
#endif

#include "mad.h"
#include "alc_session.h"
#include "alc_channel.h"
#include "lct_hdr.h"
#include "alc_hdr.h"
#include "alc_tx.h"
#include "alc_rx.h"
#include "alc_socket.h"
#include "transport.h"
#include "blocking_alg.h"

#ifdef USE_NULL_FEC
#include "../nullfeclib/null_fec.h"
#endif

#ifdef USE_SIMPLE_XOR
#include "../xorfeclib/xor_fec.h"
#endif

#ifdef USE_REED_SOLOMON
#include "../rsfeclib/rs_fec.h"
#endif

#ifdef USE_RLC
#include "../rlclib/mad_rlc.h"
#endif

#endif /* _INC_H_ */
