# garbagereminder
Don't forget to put out the trash. ESP8266 OLED reminder (for Esslingen)


![IMG_20250108_155158165_HDR](https://github.com/user-attachments/assets/7645b1aa-8f41-40b8-ac0a-21d00888c02d)


ESP8266_OLED_HW-364A Trash Pickup Calendar Program
Welcome to your new favorite way of keeping track of trash pickups (who knew it could be so exciting?)! This program is designed specifically for the ESP8266_OLED_HW-364A board, which comes with a nifty OLED display for showing off your trash schedule.

I got mylittle device here, but there are many more resources:
https://de.aliexpress.com/item/1005006582898369.html

And for some extra geeky details, check out a helpful community resource here:
GitHub: https://github.com/peff74/esp8266_OLED_HW-364A

What does it do?
This program gets your ICL Calendar from AWB-ES so that you’ll never miss a trash pickup again. 🎉

Setup Instructions:
Change the aipUrl: You'll need to adjust the aipUrl to match your own trash schedule’s .ics file. You can grab it here:
https://www.awb-es.de/abfuhr/abfuhrtermine/abfuhrtermine-suchen.html

You will also need to configure your Wi-Fi credentials.

Configure the software using preprocessor macros.
Either predefine the macros in your build environment or create a `src/local_secrets.h` file.
In case neither is defined, hard-coded fallback values will be used, which may not work for your setup.

The following macros are available for configuration:
- `WIFI_SSID`: Your Wi-Fi network's SSID
- `WIFI_PASSWORD`: Your Wi-Fi network's password
- `API_URL`: The URL to your trash pickup calendar in .ics format

For some LED bling: Want a flashing light when it’s trash day? Sure! Just hook up an LED to D2 and Ground, and you’ll have your very own optical alarm.

Disclaimer:
I am not liable if your trash is missed, your board catches fire, or the LED blinds you with its brilliance. Use at your own risk—if your trash isn’t picked up, blame the calendar, not me!
