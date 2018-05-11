**qt-1090**

----------------------------------------------------------------------------

**qt-1090** is a variant of the Dump1090 program. The latter was
designed  as a command line utility for RTLSDR devices. 
This  version supports SDRplay devices, as well as RTLSDR devices and
is equipped with a simple GUI.

![qt-1090 ](/Screenshot_qt-1090.png?raw=true)

=============================================================================

Installation
---

* mkdir build
* cd build
* cmake .. -DXXX=ON (where XXX is SDRPLAY, RTLSDR)
* make
* sudo make install

---------------------------------------------------------------------------
Devices
---------------------------------------------------------------------------

Support can be configured for either or both the SDRplay or RTLSDR based
devices. If both devices are configured, the software will attempt to
open the SDRplay, if that fails, an attempt is made to open an RTLSDR
based device. If that fails, a default device - doing nothing - is
selected.

---------------------------------------------------------------------------
Normal usage
---------------------------------------------------------------------------

Running
    ./qt-1090
the program will start. As said above, if so configured the program will try to connect to an SDRplay
device, if that is not found, it will try to connect to an RTLSDR based dongle.
If that fails as well,  a dummy input driver is initiated.

If a device  is found and initialized, two widgets appear, one with the GUI and one for
control of the device. With the latter, gain, autogain and ppm offset can be set.

----------------------------------------------------------------------------
File input
----------------------------------------------------------------------------

Adding "--ifile xxxx" as command line parameter will cause the program
to try to open the file, denoted here by xxxx. It is assumed that the file
is created as raw file with elements of 2 * 8 bits. Such a file can be created
with various rtlsdr based tools.



---------------------------------------------------------------------------
GUI
---------------------------------------------------------------------------

The GUI contains:
* a. A display showing samples, starting with the preamble.
Touching the "preamble" button will show all preambles passing some elementary tests (red and green),
or only the preambles of messages passing a CRC test.
* b. A table with a count of the number of occurrences for each type of message
that was detected in the input stream.
* c. a list of numbers indicating statistics.
* d. a list of buttons.

----------------------------------------------------------------------------
The preamble
----------------------------------------------------------------------------

The preamble of an ads-b message starts with a predefined sequence of
pulses. Each pulse has a length of 0.5 micro seconds,

	*      0   - 0.5 usec: first impulse.
	*      1.0 - 1.5 usec: second impulse.
	*      3.5 - 4   usec: third impulse.
	*      4.5 - 5   usec: last impulse.

The GUI shows the first samples of a message,
each sample represented as a bar. The "preamble" button
selects what is shown: either the start of a message where the preamble
passed some checks to determine whether it seems a reasonable
preamble, or the start of a message that 
also passed a CRC test. The messages that passed the CRC
test are represented in green, those of messages that did not pass the CRC test in red.


---------------------------------------------------------------------------
The Buttons
----------------------------------------------------------------------------

The buttons from left to right:
* a	a push button switching the terminal output from "all messages" to "interactive". In the latter case, a list of planes as is shown, regularly updated;
* b	a spinbox, with which the ttl can be set, the number of seconds within which a description of a plane has to be seen again in order to maintain it on the list of planes;
* c	a puhsbutton to switch http output. If selected, the output
is sent to port 8080 and a browser, listening to port 8080, will show a google map with planes;
* d.	a combobox for selecting the degree of error correction. Options are 
"no correction", "normal correction" (i.e. single bit), or "strong correction"
(i.e. 2 bit correction). It goes without saying that the latter is pretty cpu
intensive;
* e	a push button switching between metrics and non metrics data in interactive mode;
* f	a push button switching between "all preambles shown" and "preambles with good crc" shown. The distinction is in the color, green is "crc ok".

----------------------------------------------------------------------------
Using google maps
----------------------------------------------------------------------------

The HTTP server assumes that the file "gmap.html" is stored in the same
directory where the qt-1090 program resides


Copyrights
---

Dump1090 was written by Salvatore Sanfilippo <antirez@gmail.com> and is
released under the BSD three clause license. qt-1090 uses source code
from Dump1090.
Copyright of the modifications is J van Katwijk, Lazy Chair computing

