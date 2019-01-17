# SmartWiFiDimmer
WiFi Smart Dimmer code

This is just code to replace what's on the $18 WF-DS01 glass touch dimmer. It has an ESP-12 in it, so it's easy to flash. It just needs an external 3.3V because the internal supply voltage is a bit low, and the trace to RXD needs to be cut and soldered back because it uses 9600 baud serial to communicate with the STM micro, which handles the dimmer at 256 levels, cap touch and LEDs.

There's a web page, websocket for constant connection, and a URL for retrieving the current state and turning it on/off (/s?key=password&on=1) for non-connected control.

![WF-DS01](http://www.curioustech.net/images/dimmer1.jpg)  
![wifi wall dimmer](http://www.curioustech.net/images/dimmer2.jpg)  
![smart wifi schedule dimmer](http://www.curioustech.net/images/dimmerpage.png)  

I've added the PS-16-DZ dimmer to this project, since it's about the same. The small ESP8285 module can be removed easily, but grounding the GPIO0 pin (pin 15) might be hard. It probably doesn't hurt to short pin 14 to it, if it can't be avoided though.  
![PS-16-DZ](http://www.curioustech.net/images/ps16dz.jpg)  
