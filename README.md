# SmartStrip
SmartStrip is a sketch for Arduino meant to allow advanced control of relays through a web interface.

SmartStrip currently supports always ON/always OFF and temperature-controlled relays, i.e.: a relay can be configured to turn on when the temperature goes
above/below a given threshold (with a given hysteresis margin). The temperature can be measured through a number of different sensors.

At the moment the main targeted platform is [KMTronic's DINo](http://sigma-shop.com/product/72/web-internet-ethernet-controlled-relay-board-arduino-compatible-rs485-usb.html). I chose this board since it has everything I need for my purposes, so it is the only platform which should work "out of the box". Other boards/configurations
can probably be supported with minor changes in the code. As SmartStrip uses the [Webbino](https://github.com/SukkoPera/Webbino) library for its webserver part, it is both compatible with the official Ethernet Shield and many other network interfaces, both wired and wireless.

Features can be enabled, disabled and customized in the `common.h` file.

~Some features being investigated for the future are time-based relay switching and LCD/Keypad control, but feel free to suggest your own :).~

## Releases
Using the current git version is NOT recommended, as it might be under development and is not guaranteed to be working.

You are recommended to always use [the latest release](https://github.com/SukkoPera/SmartStrip/releases).

Every release is accompanied by any relevant notes about it, which you are recommended to read carefully.

## License
SmartStrip is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

SmartStrip is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with SmartStrip.  If not, see <http://www.gnu.org/licenses/>.
