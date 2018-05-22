**qt-1090**

----------------------------------------------------------------------------

**qt-1090** is a variant of the Dump1090 program. The latter was
designed  as a command line utility for RTLSDR devices. 
This  version supports SDRplay devices, as well as RTLSDR devices and
is equipped with a simple GUI.

![qt-1090 ](/screenshot-qt-1090.png?raw=true)


Note that the current version is 0.7: while it is running, it
is still an experimental version.

=============================================================================

-----------------------------------------------------------------------------
Installation under Windows
-----------------------------------------------------------------------------

The releases section contains a zipped folder with sdr-j software,
including the current version of the qt-1090 software.
Installation is by - obviously - downloading the zipped folder
unzipping and selecting the program to run.

-----------------------------------------------------------------------------
Installation under Linux
-----------------------------------------------------------------------------

First of all, you should have C++ and Qt5 installed.
The current version of qt-1090 uses the qhttpserver library,
the sources of this library are included in the source tree.

Creating a  qhttpserver library and installing (assuming the
current directory is the qt-1090 directory):

* cd qhttpserver
* qmake-qt5
* make
* sudo make install

--------------------------------------------------------------------------
Issues with qhttpServer
--------------------------------------------------------------------------

I am looking at other http libraries, since qhttpServer lib
gives some issues when cloding under windows.



--------------------------------------------------------------------------
Now constructing the executable
--------------------------------------------------------------------------

* cd ..			/* back in the qt-1090 directory
 
edit qt-1090.pro to select your device(s) by commenting out or uncommenting

	CONFIG  += sdrplay
	CONFIG  += dabstick
	CONFIG  += hackrf
	CONFIG	+= rtl_tcp		// untested as yet

Note that the software loads - in run time - the support library for the
selected device. So, even if you do not have a device installed, you can
select it for inclusion in the configuration.

The steps to create an executable are

* qmake-qt5
* make

the created qt-1090 executable is in ./linux-bin

A CMakeLists.txt file is available in the source directory with which
an executable can be created using cmake.
However, one needs to have the http library installed.

---------------------------------------------------------------------------
Devices
---------------------------------------------------------------------------

Support can be configured for SDRplay, RTLSDR based
devices and  the hackrf one. If all devices are configured, the
software will first attempt to open the SDRplay, if that fails,
an attempt is made to open  the hackRF device, and if that fails
an RTLSDR based device.
If that fails, it is assumed that - since no devices are apparently connected -
you want to read in a file, and a file selector will show.

As a - yet - untested addition, a client for the rtl_tcp server is implemented.
In order to select this, use a command line parameter "-n".

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

If a device  is found and initialized, a widget for the
control of the device appears. Depending on the device,
device parameters, such as gain, autogain and ppm offset can be set.

----------------------------------------------------------------------------
File input
----------------------------------------------------------------------------

It is assumed that the file
is created as raw file with elements of 2 * 8 bits, speed 2 Mhz.
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


The display on the GUI shows samples comprising a preamble (with on the
X axis -16 .. 0, and a user specified amount of samples from the accompanying
message. 
In normal use, the bars on the display are green, indicating that the message
these values were taken from passed a CRC check. Switching the pushbutton
labeled "preambles" causes all messages, passing some elementary checks
on the preamble, to be shown. In that case the bars are green for a message
passing the CRC check, red otherwise,


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
"no correction", "normal correction" (i.e. single bit), or "strong correction"
(i.e. 2 bit correction). It goes without saying that the latter is pretty cpu
intensive;
* e	a push button switching between metrics and non metrics data in interactive mode;
* f	a push button switching between "all preambles shown" and "preambles with good crc" shown. The distinction is in the color, green is "crc ok".

----------------------------------------------------------------------------
Using google maps
----------------------------------------------------------------------------

On pressing the button to switch on http, 
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

---------------------------------------------------------------------------
Copyrights
---------------------------------------------------------------------------


Dump1090 was written by Salvatore Sanfilippo <antirez@gmail.com> and is
released under the BSD three clause license. qt-1090 uses source code
from Dump1090.
Copyright of the httplibrary Nikhil Marathe <nsm.nikhil@gmail.com>
Copyright of the modifications is J van Katwijk, Lazy Chair computing

