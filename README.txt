			GEM REVISION 0.50 DISTRIBUTION
		  	Geometry Environment for MDAO


1. Prerequisites

	The current prerequisites are either EGADS with OpenCSM and/or CAPRI. 
The open source projects EGADS and OpenCSM can be obtained via GitHub at
https://github.com/OpenMDAO/EGADS and https://github.com/OpenMDAO/OpenCSM
respectively. CAPRI, which is licensed software, can be obtained for commercial
purposes from CADNexus, Inc. (http://www.CADNexus.com) or by contacting 
Bob Haimes.

1.1 GEM Source Distribution Layout

README.txt - this file
diamond    - source files for building the GEM/OpenCSM/EGADS libraries
docs       - documentation 
include    - headers used by GEM and those used to build GEM apps
pyrite     - source files used to interface GEM with Python
quartz     - source files for building the GEM/CAPRI libraries
src        - general source files
test       - test and example code

1.2 GEM & EGADS Binary Distribution

	Some test and example codes use graphics through a non-Open Source
library: GV (the Geometry Viewer). The library is made available as part
of the current GEM distribution but will disappear in the future. A
separate download (in the form of a g'zipped tarball) is required which 
when unpacked also provides a target for the GEM build. The tarball is
located at http://openmdao.org/releases/misc/EGADSbin.tgz. The tar image 
"EGADSbin.tgz" contains the following directories:

DARWIN   - binary target for OSX
DARWIN64 - binary target for OSX 64-bit
LINUX    - binary target for LINUX
LINUX64  - binary target for LINUX 64-bit
WIN32    - binary target for 32-bit Windows
WIN64    - binary target for 64-bit Windows

Each directory has subdirectories "lib", "obj" and "test".


2. Building GEM

	Note that if EGADS is to be used it must be constructed before building
here. GEM does not use the typical "configure-make-install" procedure as seen 
for many LINUX packages. Simple Make (or NMake on Windows) files are used. 
These are driven by a small number of environment variables as seen below.

2.1 Build Environment Variables

	GEM_ARCH - tells EGADS the base-level OS/architecture for the target:
			DARWIN   - MAC OSX 10.5 or greater at 32-bits
			DARWIN64 - MAC OSX 10.5 or greater at 64-bits
			LINUX    - LINUX at 32-bits
			LINUX64  - LINUX at 64-bits
			WIN32    - XP, Vista or Windows7 at 32-bits
			WIN64    - XP, Vista or Windows7 at 64-bits
        GEM_BLOC - the build location (path) for the appropriate binary 
		   target. This must must contain the "lib", "obj" and 
		   "test" subdirectories for GEM_ARCH.

For EGADS/OpenCSM builds:

	OCSM_SRC - the source location for OpenCSM
	EGADSINC - the path to find the EGADS include files
	EGADSLIB - the location to find the EGADS shared objects or dynamic
			libraries

For CAPRI builds:

	CAPRIINC - the path to find the CAPRI include files
	CAPRILIB - the location to find the CAPRI libraries, shared objects
			or dynamic libraries

2.2 The Build

	Once the above is all set, just go into the GEM "src" directory 
and type: make.
	Then go into the diamond directory for the EGADS/OpenCSM build (if
this is desired) and type: make.
	Finally go into the quartz directory for the CAPRI build (if this is 
desired) and again type: make.
	For Windows, there are no MSVS project files. It is assumed that a
"command window" is open and the environment has been setup for the 
appropriate compiler(s). There is a "make.bat" in each directory that executes
"nmake".

2.3 The Tests

	The small test examples can be made by executing "make" (or "nmake"
when using Visual Studio) from within the "test" GEM directory. This 
can be simply done by: "make -f XYZ.make" (or "nmake -f XYZ.mak" at a
command prompt under Windows). Where XYZ is the name of any of the 
test/example codes.

2.4 Python access through pyrite


2.5 GV, Windows & Visual Studio

	Building on Windows is challenging with prebuilt binaries. This is
because there are no "system libraries" and Microsoft (in its infinite
wisdom) changes the run-time components for each release of Visual Studio. 
One can mix components (with care) if things are primarily in the form of 
DLLs. Therefore it is usually important to match the compiler (and compiler
options) with any larger build. The prebuilt GV library for 32-bit Windows 
is based on MSVS 2003 (also known as Version 7.1). The 64-bit library uses
MSVS 2008 (Version 9.0).
	Please contact Bob Haimes for Geometry Viewer Libraries that may be 
required for other versions of Visual Studio.


3. Running a GEM Application

	Before a GEM application can be executed, the dynamic loader must
be told where to find the required components. This includes the paths for
both OpenCASCADE/EGADS and/or the CAPRI modules. The dynamic loader is informed
where to look by environment variable (but the name differs depending on the 
OS):

	MAC OSX		DYLD_LIBRARY_PATH (colon is the separator)
	LINUX		LD_LIBRARY_PATH   (colon is the separator)
	WIN32/64	PATH          (semicolon is the separator)

For example, this can be done on Windows at the "command window" or for a
"bat" file with the command:

  % set PATH=%CASROOT%\%CASARCH%\bin;%GEM_BLOC%\lib;%PATH%

For an OSX install it could be:

  % export DYLD_LIBRARY_PATH="$EGADSLIB":"$CAPRILIB":$DYLD_LIBRARY_PATH   -or-
  % setenv DYLD_LIBRARY_PATH "$EGADSLIB":"$CAPRILIB":$DYLD_LIBRARY_PATH

