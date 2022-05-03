<p align="center">
<img src="https://static.wixstatic.com/media/850cc4_b160987cf0174eba9a19a07ac7d762d0~mv2.jpg/v1/fill/w_83,h_116,al_c,lg_1,q_80/850cc4_b160987cf0174eba9a19a07ac7d762d0~mv2.webp">
</p>
<H1 align="center">Blue Bridge</H1>
<p align="center">
<img width=820 height=600 src="https://static.wixstatic.com/media/850cc4_febccf9a46cb411f876876ded8a68677~mv2.png">
</p>

The Blue Bridge project is a hardware device, an Android application and a webpage for sharing of boat data to allow remote monitoring and anchor watching. Boat data are received from the boat's instruments via NMEA2000 and NMEA0183 connections and are then distributed via Bluetooth to local recipients or via the internet for remote recipients with internet access. Also included in the hardware is an atmospheric pressure sensor which is shared both with the data recipients via Bluetooth and the internet and also the boat's NMEA2000 data network.

For local recipients the boat data are converted to NMEA0183 format and sent via a Bluetooth link to a local device. This could be a laptop, tablet or phone running a navigation software package (the format of the data are tailored for OpenCPN but will work with other navigation packages). Another recipient of the boat data over Bluetooth can be the Android anchor watching app that is part of this project.

For remote recipients the boat data are sent out using a GSM modem with a data connection to a server on the internet. This server is a MQTT broker from which other internet connected devices can receive the data. Remote recipients of the data can be a webpage included as part of this project or the same Android anchor watching app mentioned above using an internet connection rather than a Bluetooth one. This allows remote monitoring of the boat's instruments and position using the webpage or 
remote anchor watching using the Android application.

The diagram above shows the various connections. The hardware device has no user interface so is configured via SMS messages from any mobile phone. Details of these messages are under the docs folder.

This is the webpage that is found under the webpage folder above:
<br>
http://miniwinwm.000webhostapp.com/
<br><br>
This is the Android anchor watching app found under the app folder above built with Android Studio:
<p align="center">
<img width=300 height=600 src="https://static.wixstatic.com/media/850cc4_394de733e7264cf09104f523497a742f~mv2.png">
</p>
<br>
This project using custom hardware running an ESP32 based DevKitC board. In addition to this are the following daughter boards: a BMP280 pressure sensor board, a SIM800L GSM modem board and a modem power supply board. The schematic and PCB layout are created in DesignSpark from RS. All hardware design files are found under the hw folder above. This is the hardware as assembled:
<p align="center">
<img width=520 height=600 src="https://static.wixstatic.com/media/850cc4_b6b30b905b334b57aa9830c94b619491~mv2.jpg">
</p>
Software development for the ESP32 is done using the Espressif ESP-IDF development environment.
<br><br>
This project has now reached beta. Bug fixing and minor changes and feature additions will still occur. To see a list of outstanding work and changes to come see todo.txt.
