**qt-1090**

----------------------------------------------------------------------------

**qt-1090** is a variant of the Dump1090 program. The latter was
designed  as a command line utility for RTLSDR devices. 
This version was created to allow the use of SDRplay devices,
it supports SDRplay devices, RTLSDR devices and HACKRF One, and
is equipped with a simple GUI.

![qt-1090 ](/qt-1090-1.png?raw=true)
the display shows the preamble and the first 16 bits as appearing
in the incoming signal of a recognized message.

![qt-1090 ](/qt-1090-2.png?raw=true)
the display shows the last few incoming (recognized) messages

The current version is 0.8: while it is definitely running, it
is still an experimental version and does not have all of
its final functionality.

---------------------------------------------------------------------------
Windows
------------------------------------------------------------------------

For windows there is an installer, setup-qt1090.exe, in the releases
section of this repository. The installer will install the executable
and the required libraries (dll's). It will call upon the installer
for the dll implementing the api to get access to the SDRplay devices,
if that api is not installed already.

-----------------------------------------------------------------------
Linux
-----------------------------------------------------------------------

For linux one has to create the executable

Step 1 is of course downloading the sourcetree

* git clone https://githib.com/JvanKatwijk/qt-1090

To compile, you should have C++ and Qt5 installed.
The current version of qt-1090 uses the qhttpserver library,
the sources of this library are included in the source tree.

Step 2 is creating the qhttpserver library

Creating a  qhttpserver library and installing (assuming the
current directory is the qt-1090 directory):

* cd qhttpserver
* qmake-qt5
* make
* sudo make install

Step 3 is configuring the executable 

* cd ..			/* back in the qt-1090 directory
 
edit qt-1090.pro to select your device(s) by commenting out or uncommenting

	CONFIG  += sdrplay
	CONFIG  += dabstick
	CONFIG  += hackrf
	CONFIG	+= rtl_tcp		// untested as yet

Note that the software loads - in run time - the support library for the
selected device. So, even if you do not have a device installed, you can
select it for inclusion in the configuration.

Step 4 is running qmake/make

* qmake-qt5
* make

the created qt-1090 executable is in ./linux-bin

A CMakeLists.txt file is available in the source directory with which
an executable can be created using cmake.
However, one needs to have the http library installed.

----------------------------------------------------------------------------


---------------------------------------------------------------------------
Devices
---------------------------------------------------------------------------

Support can be configured for SDRplay, RTLSDR based
devices and  the hackrf one. The
software will attempt to open one of the configures devices,
first the SDRplay, if that fails,
an attempt is made to open the hackRF device (if configured),
and if that fails
an RTLSDR based device (if configured).

---------------------------------------------------------------------------
Normal usage
---------------------------------------------------------------------------

Running
    ./qt-1090
the program will start. No need to specify any command line parameter.
As said above, if so configured the program will try to connect to an SDRplay
device, if that is not found, it will try to connect to an RTLSDR based dongle.
Next on the list is the HACKRF One. If connecting fails here as well,
the basic assumption is that you want to open a file and a menu appears
allowing you to select a file with the extension ".iq".

If/when a device  is found and initialized, a widget for the
control of the device appears. Depending on the device,
device parameters, such as gain, autogain and ppm offset can be set.

----------------------------------------------------------------------------
Command line parameters and ini file settings
----------------------------------------------------------------------------

with -f xxxx another frequency can be selected
with -n a choice is made for an input from an rtl_tcp server

Next to command line parameters, a few configuration parameters can be
set in the ini file (.qt-1090.ini, a file in the home directory)

 * the http_port, default 8080, can be set by setting "http_port=xxxx"
 * the amount of bits to be shown in the display can be set by "bitstoShow=x"

Note that (a) each bit takes two samples, and (b), the first 16 samples, the preamble, are always shown.

----------------------------------------------------------------------------
File input
----------------------------------------------------------------------------

It is assumed that the file
is created as raw file with elements of 2 * 8 bits, speed 2.4 Mhz.
Such a file can be created
with various rtlsdr based tools.

---------------------------------------------------------------------------
GUI
---------------------------------------------------------------------------

The GUI contains:
* a. A display showing samples
Touching the "preamble" button will show all preambles passing some elementary tests (red and green),
or only the preambles of messages passing a CRC test (green);
* b. For the different message types the number of occurrences, detected in the input stream;
* c. a list of numbers indicating statistics, such as the number of messages with a preamble passing the preamble test, and the number of messages passing the crc check;
* d. a row buttons.

----------------------------------------------------------------------------
What is shown
----------------------------------------------------------------------------

The preamble of an ads-b message starts with 16
pulses. Each pulse has a length of 0.5 micro seconds,

	*      0   - 0.5 usec: first impulse.
	*      1.0 - 1.5 usec: second impulse.
	*      3.5 - 4   usec: third impulse.
	*      4.5 - 5   usec: last impulse.


The display on the GUI shows pulses from a preamble (with on the
X axis -16 .. 0, and the pulses for a user specified amount of
samples from the accompanying message.)

The amount of bits of the message shown is default 16, it can be changed by
setting the value for "bitstoShow" in the ini file. This ini file is to be
found "~/.qt-1090.ini".

---------------------------------------------------------------------------
The Buttons
----------------------------------------------------------------------------

The buttons from left to right:
* a	a push button switching the terminal output from "all messages" to "interactive". In the latter case, a list of planes as is shown, regularly updated;
* b	a spinbox, with which the ttl can be set, the number of seconds within which a description of a plane has to be seen again in order to maintain it on the list of planes;
* c	a puhsbutton to switch http output. If selected, the output
is sent to a predefined port. A a browser, listening to this port,
will show a google map with planes (i.e. a "flight plane radar").
* d.	a combobox for selecting the degree of error correction. Options are 
"no correction", "1 bit correction", or "2 bit correction".
It goes without saying that the latter is pretty cpu
intensive;
* e	a push button switching between metrics and non metrics data in interactive mode;
* f	a push button switching between displaying (part of) a single
recognized frame, or a sequence of frames.

The bottom line contains a button "dump", pushing this button causes
some data to be written onto a file. This data described the entry and
exit points of planes in the system, data as given below

	Plane  4951cc TAP764
	36975     52.036  3.960    entered at Fri May 25 16:50:30 2018
	37000     52.289  4.084    left at Fri May 25 16:53:45 2018
	
	Plane  484188
	20300     51.719  4.497    entered at Fri May 25 16:50:50 2018
	21725     51.553  4.403    left at Fri May 25 16:54:19 2018
	
	Plane  484557 KLM19P
	14900     51.964  4.753    entered at Fri May 25 16:50:33 2018
	18100     51.953  4.749    left at Fri May 25 16:55:01 2018
	

----------------------------------------------------------------------------
Showing planes on google maps
----------------------------------------------------------------------------

On pressing the button to switch on http, a browser can display the planes
on a map, listening to a designated  port.
The HTTP server assumes that the file "gmap.html" is stored in the same
directory where the qt-1090 program resides. Note that the browser should
listen to the port specified in the field next to that button.
The port can be set (changed) in the ini file, default is 8080.


--------------------------------------------------------------------------
TODO
--------------------------------------------------------------------------

A major reason to use a different http library is the plan to make
the software available under Windows. The zipped folder contains a
DLL implementing the http library.

On the todo list:

* a. Generate an AppImage for use under Linux (x64) and the RPI 2/3
* b. Generate output to use with "radar" programs.
* c. Generating database output

---------------------------------------------------------------------------
Copyrights
---------------------------------------------------------------------------


dump1090 is a program developed by Salvatore Sanfilippo <antirez@gmail.com>
and is released under the BSD three clause license.

The idea and code of the bit decoder to work at 2400000 rather than at
2000000 samples/second is
Copyright (c) 2014,2015 Oliver Jowett <oliver@mutability.co.uk>

qt-1090 uses source code from Dump1090, both the 
original version (Salvatore Sanfilippo) and a derived version (Oliver Jowett),
Dump1090 was written by Salvatore Sanfilippo <antirez@gmail.com> and is
released under the BSD three clause license.
dump1090  as derived version is Copyright (c) 2014,2015 Oliver Jowett
<oliver@mutability.co.uk>

Copyright of the httplibrary Nikhil Marathe <nsm.nikhil@gmail.com>
Copyright of the modifications is J van Katwijk, Lazy Chair computing

