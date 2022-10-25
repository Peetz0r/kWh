Originally based on https://juerd.nl/site.plp/kwh (dutch) and therefore forked from https://github.com/Juerd/kWh/

Then I dumbed down the arduino part because I don't want any buttons or displays. I just want MQTT!

So I added an esp8266. Why not just use the 8266? Because I can't have blocking network traffic while also looking for pulses on the same µC core.

Why not pick an esp32 instead? Hey stop asking difficult questions okay!

But you removed all the settings stuff, and now my compiler complains about missing constants?! Yes, try this instead:

    PLATFORMIO_BUILD_FLAGS='
      -DCYCLES_PER_KWH=375
      -DLOWER_THRESHOLD=101
      -DUPPER_THRESHOLD=105
      -DMAX_WATT=6000
    ' pio run; pio device monitor
