# iotsa433 - 433MHz home automation interface

This is a wifi http server that allows access to simple 433 MHz home automation devices. These are usually plugin sockets, door openers, etc. and the remote controls for those.

Home page is <https://github.com/cwi-dis/iotsa433>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

## Software requirements

* Arduino IDE, v1.6 or later.
* The iotsa framework, download from <https://github.com/cwi-dis/iotsa>.

Or you can build using PlatformIO.

## Hardware requirements

* a iotsa board. Alternatively you can use any other esp8266 board, but then you may have to adapt the available I/O pins.
* a 433 MHz receiver  The **RXB6** is common, cheap and known to work. Attach receiver data pin to GPIO 4.
* a 433 MHz transmitter. The **SF R433D** is often sold together with the RXB6 and is known to work. Attach GPIO 5 to receiver transmit data pin.

## REST API

`GET /api/433` to be supplied.

`PUT /api/433` to be supplied.

`GET /api/433config` to be supplied.