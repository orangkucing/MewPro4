#MewPro 2

Arduino BacPac™ for GoPro Hero 4 Black: GoPro can be controlled by ATtiny1634 attached on Herobus.

Resources:

- How To Use MewPro 2 and Application (This is an old documentation for GoPro Hero 3+ Black support, however, the installation procedure is the same to the application): [https://mewpro.cc/2015/06/29/how-to-use-mewpro-2-and-application/]

- Schematic Drawing of MewPro 2 board: [https://mewpro.cc/wp-content/uploads/MewPro2.pdf]

------

###Prerequisites

In order to use MewPro 2 as a Hero 4 Black controller you need the following hardwares:

- GoPro Hero 4 Black
- [MewPro 2](http://mewpro.cc/product/mewpro-2/)
- [Temporary FTDI Header](https://sites.google.com/site/handymaneric2/electronics/arduinominitemporaryheader) (included in MewPro 2 package)
- [Sparkfun FTDI Basic Breakout - 3.3V](https://www.sparkfun.com/products/9873) and USB cable
 - Use 3.3V version of the breakout board. Any pin compatible board should work.

Softwares:

MewPro 2 is shipped with optiboot as well as MewPro4 software for genlocking. But if you like to modify/update the software you will need to prepare the following IDE, core. To install each software please refer their documentations.

- [Arduino IDE 1.6.12 or newer](http://arduino.cc/en/main/software)
 - Older versions of Arduino IDE might work but we don't confirm that.
- [ATtiny Core](https://github.com/SpenceKonde/ATTinyCore)
 - 1634, x313, x4, x41, x5, x61, x7, x8 and 828 for Arduino 1.6.x

(A modified WireS library is distributed with MewPro 4 source code package, so you don't need to download the original version separately.)

Lastly grab the MewPro4 application:

- MewPro4 Application
 - This is an open source software (MIT license). You can modify and distribute it as you like.

###Connections

On your PC launch Arduino IDE that was installed as described in the above. In Arduino IDE [File]→[Open...]→ then open MewPro4.ino.

Connect MewPro 2

!(http://mewpro.cc/wp-content/uploads/connection2.jpg)

to your PC with FTDI board and the temporary header.

!(http://mewpro.cc/wp-content/uploads/conne2.jpg)

Connecting FTDI please refer the pinout image below:

!(http://mewpro.cc/wp-content/uploads/MewPro2.jpg)

Then connect them to GoPro Hero 4 Black.

!(http://mewpro.cc/wp-content/uploads/conne1.jpg)

In Arduino IDE application, select [Tools]→[Board]→[ATTiny1634 (optiboot)] and [Tools]→[Port]→[(the port where you connected the FTDI cable)]. (B.O.D. and Clock settings are “don’t care” as these values are only effective when you burn a bootloader to the microcontroller by using an ISP programmer.)

MewPro4 source code is targeted to MewPro 2 board, genlocking with Iliad, and Hero 4 Black firmware version v3.0.0 or v4.0.0 (please note v4.0.0 has a fatal bug in bulk setting transfer). So the code should be complied successfully without any modifications. Click "Verify" the MewPro4 sketch and "Upload" it to MewPro 2 board.

