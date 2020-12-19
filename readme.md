# iotsaGPIO - web server that interfaces to hardware signals

This is a wifi http server that allows access to some 3.3v hardware signals (analog input, digital input, digital output, PWM output). It is built using the iotsa framework.

Home page is <https://github.com/cwi-dis/iotsaGPIO>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

As distributed the service allows access to 6 digital ports (which can be configured as input, input with pullup, output or PWM-output) and one analog input. In addition there is a readonly port `timestamp` which returns the number of milliseconds since boot.

## Software requirements

* Arduino IDE, v1.6 or later.
* The iotsa framework, download from <https://github.com/cwi-dis/iotsa>.

Or you can build using PlatformIO.

## Hardware requirements

* a iotsa board. Alternatively you can use any other esp8266 board, but then you may have to adapt the available I/O pins.

## REST API

`GET /api/io` will return the current value of all current input ports.

`PUT /api/io` expects an object that can contain `name : value` pairs for any ports configured as _output_ or _pwm___output_.

`GET /api/ioconfig` will return an object with name-value pairs containing the current configuration of each port:

- `'input'` for input ports (digital, analog or timestamp),
- `'input_pullup'` for digital input ports with the internall pullup resistor enabled (so open inputs read as 1 in stead of a random undefined value).
- `'output'` for digital output ports.
- `'pwm_output'` for digital output ports that present a pseudo-analog output value using pulse-width modulation.
- `'unused'` for ports that are not in use.

`PUT /api/ioconfig` allows to set the configuration for each port, using the same values as above. The device must be in configuration mode.