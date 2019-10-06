# Yet Another VFD Clock

This clock is based on an ESP8266.

The interface is designed to be simple. And this meant that there is completely no buttons.

Software was written with [Arduino Core](https://github.com/esp8266/Arduino) in one night.

Software Requirements:
- Must be able to display the correct time without any interaction form the user.
- The time must not drift.

In order to achieve these 2 objective, I used an NTP on startup and and teh code is set to check the current time from the NTP server every couple of minutes.

In order to simplify the connecting of the ESP8266 to an AP, a library, [WiFiManager](https://github.com/tzapu/WiFiManager) was used.

[FastLED library](https://github.com/FastLED/FastLED) was used to run the WS2812 on the PCB.

Ideally, I would write the code with the native ESP8266 framework where the time is kept by a hardware timer. Considering that this was written in a night, I am contented with the results and have no intentions on continuing the development.
