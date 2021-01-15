# iotsa433 - 433MHz home automation interface

This is a wifi http server that allows access to simple 433 MHz home automation devices. These are usually plugin sockets, door openers, etc. and the remote controls for those.

Home page is <https://github.com/cwi-dis/iotsa433>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

## Software requirements

* Arduino IDE, v1.6 or later.
* The iotsa framework, download from <https://github.com/cwi-dis/iotsa>.
* To build without modifications (but see _Remotes and Sockets_ below) a forked _rc-switch_ from <https://github.com/jackjansen/rc-switch.git>.

Or you can build using PlatformIO.

## Hardware requirements

* a iotsa board. Alternatively you can use any other esp8266 board, but then you may have to adapt the available I/O pins.
* a 433 MHz receiver  The **RXB6** is common, cheap and known to work. Attach receiver data pin to GPIO 4.
* a 433 MHz transmitter. The **SF R433D** is often sold together with the RXB6 and is known to work. Attach GPIO 5 to receiver transmit data pin.

## Remotes and sockets

All 433MHz remote controls and controlled devices (such as sockets) use slightly different protocols. Most use simple AM modulation (also known as OOK, on-off-keying), but they differ in how a bit is encoded, how many bits there are in a packet and many other parameters.

For this reason you will most likely have to adapt the code to whatever remote controls you happen to have. The library <https://github.com/sui77/rc-switch> supports many types, you should start there. That repo also has information on how to find out what your switches send (and your sockets receive).

But: you could be lucky: the type that is called _HEMA_ in the code is a fairly common type. The chance that you have an _ELRO Flamingo FA500_ set is a lot less likely, so you could rip that code out and use the standard rc-switch library. 

## REST API

`GET /api/433` to be supplied.

`PUT /api/433` to be supplied.

`GET /api/433config` to be supplied.