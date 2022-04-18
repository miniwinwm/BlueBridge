# BlueThing

NMEA2000 to NMEA0183 format converter with the converted messages sent out via Bluetooth. Also includes an atmospheric pressure sensor and sends pressure data out via NMEA2000 and NMEA0183 format via Bluetooth. AIS and GPS come in via a NMEA0183 connection and are sent unchanged to Bluetooth link. Some of the boat data also sent via a SIM800L GSM modem to a MQTT broker. Runs on an ESP32 DevKitC board with custom hardware. ESP32 part built with ESP-IDF.<br>
A webpage is provided that reads the data from the broker and displays the read values. Source code found under website folder.<br>
Android anchor watching app that reads NMEA0183 format data via Bluetooth. Android Studio code found under app folder.<br>
Hardware schematic and layout done in DesignSpark. Design files and PDF prints for each under hw folder.<br>
This project has not reached alpha yet and many changes will occur. To see a list of outstanding work and changes to come see todo.txt.


