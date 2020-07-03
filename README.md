# Nabaztag-messenger
Nabby - Nabaztag rabbit messagenger
Project created by Raj Bose, July 2020

This project was created to convert an old non functioning NABAZTAG rabbit into a messaging system connected on Wifi.
Features are:
  1. MP3 player inside (KT403A based)
  2. ESP8266 and ESP32 based
  3. Nabaztag is fully functional:
      - Left and right ear positioning using encoder
      - Top button to kill sound
      - RGB LED's connected via I2C IO expander
  4. Seperate doorbell unit connecting to Wifi
  5. Nabaztag (called Nabby) connecting to Wifi and publishing mDNS service 'Nabby'
  6. Doorbell unit searching for all mDNS Nabby services called 'Nabby'
  7. communication between doorbell and Nabby via UDP
  8. Nabby contains command language
  9. From PC or mobile, via UDP Nabby can be commanded using commandlanguage
  10. For debugging Nabby can be commanded via command language vis serial (connected via USB)
  11. Commands support: ear control, LED control, MP3 player control, version readout etc.

The repository contains electronic schematic diagrams of Nabby and of the doorbell unit.

