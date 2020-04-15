# STM32 All Programmable system
## STM32 + Spartan6 with NI-VISA-friendly fast USB TMC interface

![The prototype system](https://github.com/olegv142/STM32AllProgrammable/blob/master/doc/prototype.jpg)

This is the full working prototype of the system comprising the flexibility of FPGA with processing power of ARM core yet having much lower price than available SoC solutions like Xilinx Zynq and being even more flexible and user friendly.

The prototype is built around 2 boards: XCore407I from WaveShare and XME0601 from PiSwords. They are interconnected by two buses. The slow SPI serial bus allows for data exchange with FPGA and serves for flash reprogramming. The fast DCMI bus provides the means for passing large amount of data from FPGA to ARM at rate up to 50MByte/sec. Access to both buses as well as to firmware loading facility is provided to host applications via USB bus. The USB interface implements Test and Measurement Class to allow for simple integration with NI-VISA family of measurement software.

## Author

Oleg Volkov (olegv142@gmail.com)
