# esphome-amis

This is a component for ESPHome that allows to receive data from an AMIS
smart meter. This meter is used in Upper Austria and provided by Netz OÖ GmbH,
it is actually a Siemens TD-3511, but uses AES encrypted M-Bus frames for 
communication.

## Implementation

The ESP communicates via Serial-over-Infrared with the AMIS meter. Therfore,
an IR communication device is needed. The data exchange is entirely driven
by the meter, it sends data in 1s intervals. The ESPHome integration parses
the data and provides sensors for all currently supported values - see the
example configuration file for details.

This requires at least ESPHome 1.18.0.

## Usage

You need the AES Key from Netz OÖ GmbH. You can request the AES key online
in the customer service area. The hex key needs to be copied to the
configuration files as-is and the implementation takes care of the rest.

## Credits

This implmentation was tested with the AMIS Reader by Gottfried Prandstetter,
but an ESP32 instead of the ESP8266 was used. His implmenetation and the reader
are available at http://www.mitterbaur.at/amis-leser.html.

Parts of the parsing are based on the VZLogger implementation at 
https://github.com/volkszaehler/vzlogger/blob/master/src/protocols/MeterOMS.cpp
