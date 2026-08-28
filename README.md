        **qt-1090**

---------------------------------------------------------------------------
NEW: entry for sdrconnect
----------------------------------------------------------------------------

A new version of qt-1090 contains improved http handling and an
entry to use the program as "backend" for sdrconnect.

----------------------------------------------------------------------------

**qt-1090** is a variant of the popular Dump1090 program. The latter was
designed  as a command line utility for RTLSDR devices. 
The qt-1090 version was created to allow the use of SDRplay devices,
but it supports SDRplay devices, RTLSDR devices, HACKRF One, Lime devoces,
the Adalm Pluto and the sdrConnect framework.
It is equipped  with a simple GUI.

![qt-1090 ](/qt-1090-mainwindow.png?raw=true)

The display shows the frequency spectrum of the region of 1090 MHz.

The top line of the GUI shows some selectors

 * the selector with text **streaming** selects the way the output is presented on the command window. In **streaming mode** the decoded incoming messages are shown, in **plane list** mode, details of the recognized places are shown. 

 *  **http output**, when touched shows a webbrowser with the planes on it. It is evident that some form of home position is known;

 *  the selector with text **no correction** can be used to selected between no correction, one bit correction or two bit correction;

 * the selector with text ** not metric** is used to select between metric and not metric data.

 * On start up the last selector on the first line is labeled **device** and is used to select one of the configured input devices. Once a device is selected, the selector dispappears from the gui.


![qt-1090 ](/qt-1090-web-1.png?raw=true)

The web browser shows the planes that are detected, clicking on a place shows at the tight hand side some information on the selected plane.

![qt-1090 ](/qt-1090-web-2.png?raw=true)

The default mode is to display the list of planes currently being
seen (either completely or partly).

![qt-1090 ](/qt-1090-stream.png?raw=true)

As said, alternatively, descriptions of the planes seen pass by

![qt-1090 ](/qt-1090-stream-2.png?raw=true)

The center of the map can be set by filling in a "home address",
just give (an estimate of) your location in latitude and longitude
(decimal values), and the software will center the map to that
location.

The port used can be changed in the ".ini" file, a configiration file
located in the user's home directory. Setting "http_port" variable
in the ini file can be used to define another port than the default
one "8080".

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


On executables
----------------------------------------------------------------------------

The software is developed on a Linux system, and cross compiled for 
Windows.
While it is relatively easy to build an executable, an executable version 
for Linux (an AppImage) and  an installer for Windows are available
in the  releases section

---------------------------------------------------------------------------
Creating an executable
---------------------------------------------------------------------------

NOTE: the CMakeLists.txt file is NOT up to date

For linux one has to create the executable

Step 1 is of course downloading the sourcetree

* git clone https://githib.com/JvanKatwijk/qt-1090

To compile, you should have C++ and Qt6 installed.

Step 2 is configuring the executable 

* cd qt-1090			/* into the qt-1090 directory
 
edit qt-1090.pro to select your device(s) by commenting out or uncommenting

	CONFIG  += sdrplay-v2
	CONFIG  += sdrplay-v3
	CONFIG  += dabstick
	CONFIG  += hackrf
	CONFIG  += lime
	CONFIG  += pluto

Note that the software - as usual with my software - will dynamically load
the device driver when a device is selected.

Step 3 is running qmake/make

* qmake-qt5
* make

the created qt-1090 executable is in ./linux-bin

---------------------------------------------------------------------------
Windows
---------------------------------------------------------------------------

For Windows an installer is available

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

Copyright of the modifications is J van Katwijk, Lazy Chair computing

