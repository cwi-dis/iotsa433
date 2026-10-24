# iotsa433 - 433MHz home automation interface

![build-platformio](https://github.com/cwi-dis/iotsa433/workflows/build-platformio/badge.svg)
![build-arduino](https://github.com/cwi-dis/iotsa433/workflows/build-arduino/badge.svg)

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

## Terminology

The wording used is different for each manufacturer of 433MHz devices and software. We use the following terminology:

- `brand` is a class of devices and remotes that are somehow compatible with each other.
- `group` can usually be set on the remote using dip-switches, or is hardcoded by the remote.
- `appliance` is a socket or bulb or motor or other devices controlled by (usually) one set of on/off buttons on one remote.
- `state` whether the device is on or off. Support for multi-valued states (such dimmers) is easy to add.
- `telegram_protocol`, `telegram_pulsewidth`, `telegram_tristate`, `telegram_binary`, `telegram_bits` see  <https://github.com/sui77/rc-switch>

## Web pages, REST API and trigger

### The receiver

The page `/433receiver` shows the recently received telegrams and the forwarders, and lets you add and remove forwarders.

`GET /api/433receiver` returns:

-  `received` is a list of the 16 most recently received commands from remote controls (telegrams). Fields as above, field `time` states how many seconds ago the command was received, and `status` what forwarding did with it (`queued`, `ignored`, `sent`, `failed`).
-  `forwarders` is a list of filters and URLs. When a telegram is received, **every** forwarder that matches it sends a `GET` request to its URL, in list order (cwi-dis/iotsa433#2). An empty filter field always matches. Available fields:
	-  `brand`, `group`, `appliance`, `state` and `telegram_tristate` are as explained above.
	-  `url` the URL to send the request to, with `credentials` (`user:password`), `token` (a bearer token) and `rootCA` (a PEM certificate for `https:` URLs). These three are never shown, only whether they are set.
	-  `parameters` is a boolean. If true the fields are added as a query with _name=value_ to the URL, in the form the `/433send` trigger takes them, so a forwarder can call the sender of another iotsa433 (a relay).

The forwarders are a collection, with iotsa's collection shape (cwi-dis/iotsa#309):

-  `PUT /api/433receiver` with `{"forwarders": [...]}` replaces the whole list. This is what `iotsa backup`/`restore` use; a `received` field in the body is ignored: it is GET-only live data, not restorable config.
-  `PUT /api/433receiver` with `{"forwarders": {"1": {...}}}` changes the second forwarder (the key is the index), `{"forwarders": {"1": null}}` removes it.
-  `POST /api/433receiver` with `{"forwarders": {"url": "...", ...}}` adds a forwarder to the end of the list; it needs a `url`.

### The sender

The page `/433sender` has a form for each kind of command: a switch command, a tristate telegram and a binary telegram.

`PUT` or `POST /api/433sender` sends one command. Either `telegram_tristate`, or `telegram_binary`, or a switch command with `group`, `appliance`, `state` (`on`/`off`, `true`/`false` or `1`/`0`) and optionally `brand`. `telegram_protocol` and `telegram_pulsewidth` are optional for all three; 0 or absent means the default. `GET /api/433sender` returns the `brands` this build knows.

The trigger `/433send` takes the same fields as URL arguments and answers `OK` in plain text, or the error. For a script or another device:

```
curl 'http://iotsa433.local/433send?group=01011&appliance=A&state=on'
```
