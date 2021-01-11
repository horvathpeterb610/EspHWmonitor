# EspHWmonitor
Thanks for OpenHardwareMonitor Remote Web server option, this is a Hwmonitor over wifi with OpenHw monitor data.json, and a weather station if dont start remote server.

Check firewall if hw monitor wont start on the screen.

I follow Bodmer/TFT_eSPI driver setup to solder the esp8266 to the display.<br />
Used display is an ST7796 driverIC screen.<br />
Display SDO/MISO to NodeMCU pin D6 (or leave disconnected if not reading TFT)<br />
Display LED to NodeMCU pin 3.3V <br />
Display SCK to NodeMCU pin D5<br />
Display SDI/MOSI to NodeMCU pin D7<br />
Display DC (RS/AO)to NodeMCU pin D3<br />
Display RESET to NodeMCU pin esp RST<br />
Display CS to NodeMCU pin D8<br />
Display GND to NodeMCU pin GND (0V)<br />
Display VCC to NodeMCU 3.3V

![alt text](/img1.jpg)
![alt text](/img2.jpg)
