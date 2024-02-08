# SmartWiFiDimmer
WiFi Smart Dimmer code
  
A simple replacement for many different ESP8266 dimmers and switches like the WF-DS01, PS-16-DZ, Tuya, and generic relay switches and modules, without cloud based control. They find each other, communicate status, time and can control each other, and have web page settings, websocket for constant connection, along with basic URL parameters for retrieving the current state and turning it on/off (/s?key=password&pwr0=1) for non-connected control.
  
Set the watts and PPKWH to monitor power used. 2 LEDs can be turned on/off with the light, or reversed for being on when the light is off. Scheduling, added motion sensors, and linking to other dimmers. The components at the bottom are custom name, IP, mode (disabled, link, or reverse) checkbox 1 is for linking dimmer value, box 2 is for motion sending to another device, and last is off delay (sends value as seconds to other device to turn off).  
  
![smart wifi schedule dimmer](http://www.curioustech.net/images/wifiswitchweb2.png)  
![WF-DS01](http://www.curioustech.net/images/dimmer1.jpg)  
![wifi wall dimmer](http://www.curioustech.net/images/dimmer2.jpg)  
![dimmer with PIR](http://curioustech.net/images/moes.jpg)  
I've added the PS-16-DZ dimmer to this project, since it's about the same. The small ESP8285 module can be removed easily, but grounding the GPIO0 pin (pin 15) might be hard. It probably doesn't hurt to short pin 14 to it, if it can't be avoided though.  
![PS-16-DZ](http://www.curioustech.net/images/ps16dz.jpg)  
