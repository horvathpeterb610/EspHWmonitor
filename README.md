# EspHWmonitor
Thanks for OpenHardwareMonitor Remote Web server option, this is a Hwmonitor over wifi with OpenHw monitor data.json, and a weather station if dont start remote server.

Check firewall if hw monitor wont start on the screen.

I follow Bodmer/TFT_eSPI driver setup to solder the esp8266 to the display.
Used display is an ST7796 driverIC screen.
Display SDO/MISO to NodeMCU pin D6 (or leave disconnected if not reading TFT)
Display LED to NodeMCU pin 3.3V 
Display SCK to NodeMCU pin D5
Display SDI/MOSI to NodeMCU pin D7
Display DC (RS/AO)to NodeMCU pin D3
Display RESET to NodeMCU pin esp RST
Display CS to NodeMCU pin D8
Display GND to NodeMCU pin GND (0V)
Display VCC to NodeMCU 3.3V
