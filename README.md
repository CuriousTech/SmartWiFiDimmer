# SmartWiFiDimmer
WiFi Smart Dimmer code

This is just code to replace what's on the $18 WF-DS01 glass touch dimmer. It has an ESP-12 in it, so it's easy to flash. It just needs an external 3.3V because the internal supply voltage is a bit low, and the trace to RXD needs to be cut because it uses 9600 baud serial to communicate with the STM micro, which handles the 256 level dimmer, cap touch and LEDs.

There's a web page, websocket for constant connection, and a URL for retrieving the current state and turning it on/off (/s?key=password&on=1) for non-connected control.

![smartdimmer](http://www.curioustech.net/images/dimmer1.jpg)  
![smartdimmer](http://www.curioustech.net/images/dimmer2.jpg)  
![smartdimmer](http://www.curioustech.net/images/dimmerpage.png)  
