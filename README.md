        **qt-1090**

----------------------------------------------------------------------------

**qt-1090** is a variant of the popular Dump1090 program. The latter was
designed  as a command line utility for RTLSDR devices. 
The qt-1090 version was created to allow the use of SDRplay devices,
but it supports SDRplay devices, RTLSDR devices, HACKRF One, Lime devoces
and the Adalm Pluto.
It is equipped  with a simple GUI.

![qt-1090 ](/qt-1090-1.png?raw=true)
The display shows the frequency spectrum of the region of 1090 MHz.

The default choice is to show the list of visible planes on the command window,
by starting the http server and a browser, the position of the planes
can be made visible as can be seen in the picture.

The icons for the planes are made slightly larger, clicking with the mouse
on a plane icon will show some details of the flight on the right hand
side of the browser.


![qt-1090 ](/qt-1090-A.png?raw=true)

The basic idea in the design of the GUI is that for each session one selects
the input device. The GUI therefore contains a device selector; once
a device is selected and could be opened, the selector is not visible
anymore.


The default mode is to display the list of planes currently being
seen (either completely or partly), the button on the widget - whentouched -
will change that in showing the list of messages that is being received.

The http server is now integrated in the software.
Touching the http button will start the server.

qt1090 has as option to automatically start the system's default browser,
the checkbox on the bottom line of the main widget will activatee
or deactivate that option.

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


--------------------------------------------------------------------------
Changing the appearance
--------------------------------------------------------------------------

One may choose among three appearances of the control widget,
Passing a '-A' or a '-B' parameter a stylesheet will be used to
color the elements in the control  widget.
Passing a 'C' as parameter will show a widget without any color (other than the
color of the brush on the display).

The settings will be saved.

![qt-1090 ](/qt-1090-B.png?raw=true)
![qt-1090 ](/qt-1090-C.png?raw=true)

---------------------------------------------------------------------------
Creating an executable
---------------------------------------------------------------------------
For linux one has to create the executable

Step 1 is of course downloading the sourcetree

* git clone https://githib.com/JvanKatwijk/qt-1090

To compile, you should have C++ and Qt5 installed.

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

