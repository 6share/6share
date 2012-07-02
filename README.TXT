----------------------------------------------------------------------------------------------------

						MAD-FCL archive

----------------------------------------------------------------------------------------------------

	MAD-ALC, implementation of ALC/LCT protocols
	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi

	MAD-FLUTE, implementation of FLUTE protocol
	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi

	MAD-Null-FEC, implementation of Compact No-Code FEC codec
	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi
	
	MAD-RLC, implementation of RLC Congestion Control protocol
	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi

	MAD-RS-FEC, implementation of Reed-Solomon FEC codec
	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi

	MAD-XOR-FEC, implementation of Simple XOR FEC codec
	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi

	MAD-SDP, implementation of SDP protocol
 	Copyright (c) 2003-2004 TUT - Tampere University of Technology
	main authors/contacts: jani.peltotalo@tut.fi and sami.peltotalo@tut.fi

	This program is free software; you can redistribute it and/or modify	
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

----------------------------------------------------------------------------------------------------

	MAD-FCL archive consist of:

	- alclib:	Source files for MAD-ALC library. 
	- flute:	Source files for MAD-FLUTE application.
	- nullfeclib:	Source files for MAD-Null-FEC (Compact No-Code FEC codec) library.
	- rlclib:	Source files for MAD-RLC (RLC Congestion Cotrol) library.
	- rsfeclib:	Source files for MAD-RS-FEC (Reed-Solomon FEC codec) library.
	- xorfeclib:	Source Files for MAD-XOR-FEC (Simple XOR FEC codec) library.
	- sdplib:	Source files for MAD-SDP library.
	- doc:		Example File Delivery Table (FDT) and example SDP file.
	- Makefile:	Makefile for Linux OS.
	- mad_fcl.sln:	Microsoft Visual Studio .NET solution file.
	- README.TXT:	This readme file.
	- LICENCE.TXT:	Archive's license file.
	
----------------------------------------------------------------------------------------------------

	Supported Operation Systems are:

	- Debian GNU/Linux (Tested with version 2.6.5) 
	- Windows 2000/XP

----------------------------------------------------------------------------------------------------

	How to compile MAD-FCL in Linux:

	The MAD-FCL archive contains a Makefile to compile the MAD-ALC, the MAD-Null-FEC,
	the MAD-RLC, the MAD-RS-FEC, the MAD-XOR-FEC and the MAD-SDP libraries and the
	MAD-FLUTE application. Please note that Makefile is defined for the GNU make.

	Only write (g)make in the top directory, and all directories are scanned through and
	the MAD-ALC, the MAD-Null-FEC, the MAD-RLC, the MAD-RS-FEC, the MAD-XOR-FEC and the
	MAD-SDP libraries and the MAD-FLUTE application are compiled. 

	The MAD-ALC, the MAD-Null-FEC, the MAD-RLC, the MAD-RS-FEC, the MAD-XOR-FEC and the
	MAD-SDP libraries are created to the lib directory, and the MAD-FLUTE application is
	created to the bin directory.
	
----------------------------------------------------------------------------------------------------

	How to compile MAD-FCL in Windows:

	The MAD-FCL archive contains a Microsoft Visual Studio .NET solution file (mad_fcl.sln)
	to compile the MAD-ALC, the MAD-Null-FEC, the MAD-RLC, the MAD-RS-FEC, the MAD-XOR-FEC
	and the MAD-SDP libraries and the MAD-FLUTE application.

	First start the Microsoft Visual Studio .NET and open the MAD-FCL solution, and then use
	Build menu (Build Solution) to create the MAD-ALC, the MAD-Null-FEC, the MAD-RLC,
	the MAD-RS-FEC, the MAD-XOR-FEC and the MAD-SDP libraries and the MAD-FLUTE application.
	There is dependencies set so everything should go automatically.

	The MAD-ALC, the MAD-Null-FEC, the MAD-RLC, the MAD-RS-FEC, the MAD-XOR-FEC and the
	MAD-SDP libraries are created to the lib\win32 directory, and the MAD-FLUTE application
	is created to the bin\win32 directory.

----------------------------------------------------------------------------------------------------

	How to use:

	Go to the bin directory in Linux or bin/win32 directory in Windows and write ./flute -h or
	flute -h respectively to see what are possible parameters and use cases.

----------------------------------------------------------------------------------------------------	
