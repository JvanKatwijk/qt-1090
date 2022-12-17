**qt-1090**

----------------------------------------------------------------------------

**qt-1090** is a variant of the Dump1090 program. The latter was
designed  as a command line utility for RTLSDR devices. 
This version was created to allow the use of SDRplay devices,
it supports SDRplay devices, RTLSDR devices and HACKRF One, and
is equipped with a simple GUI.

![qt-1090 ](/qt-1090-B.png?raw=true)
The display shows the frequency spectrum of the region of 1090 MHz.

The default choice is to show the list of visible planes on the command window,
by starting the http server and a browser, the position of the planes
can be made visible.

![qt-1090 ](/qt-1090-2.png?raw=true)

The icons for the planes are made slightly larger, clicking with the mouse
on a plane icon will show some details of the flight on the right hand
side of the browser.

---------------------------------------------------------------------------
Creating an executable
---------------------------------------------------------------------------
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

	CONFIG  += sdrplay-v2
	CONFIG  += sdrplay-v3
	CONFIG  += dabstick
	CONFIG  += hackrf
    CONFIG  += lime
    CONFIG  += pluto

Later versions will include driver support for the Adalm pluto and the
lime device.

Note that the software - as usual with my software - will dynamically load
the device driver when a device is selected.

Step 4 is running qmake/make

* qmake-qt5
* make

the created qt-1090 executable is in ./linux-bin


---------------------------------------------------------------------------
GUI
---------------------------------------------------------------------------

The GUI contains:
* a. A display showing the spectrum of 1090 Mhz with a width of 2.4 Mhz
* b. For the different message types the number of occurrences, detected in the input stream;
* c. a list of numbers indicating statistics, such as the number of messages with a preamble passing the preamble test, and the number of messages passing the crc check;
*d. a device selector. 

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

---------------------------------------------------------------------------
Changing the appearance
---------------------------------------------------------------------------

One may choose among three appearances of the control widget,
Passing a '-A' or a '-B' parameter a stylesheet will be used to
color the elements in the control  widget.
Passing a 'C' as parameter will show a widget without any color (other than the
color of the brush on the display).

The settings will be saved.

![qt-1090 ](/qt-1090-A.png?raw=true)
![qt-1090 ](/qt-1090-C.png?raw=true)

----------------------------------------------------------------------------
Showing planes on google maps
----------------------------------------------------------------------------

On pressing the button to switch on http, a browser can display the planes
on a map, listening to a designated  port.
The HTTP server assumes that the file "gmap.html" is stored in the same
directory where the qt-1090 program resides. Note that the browser should
listen to the port specified in the field next to that button.
The port can be set (changed) in the ini file, default is 8080.

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

