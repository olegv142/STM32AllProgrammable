# STM32 All Programmable system
## STM32 + Spartan6 with NI-VISA-friendly fast USB TMC interface

![The prototype system](https://github.com/olegv142/STM32AllProgrammable/blob/master/doc/prototype.jpg)

This is the full working prototype of the system comprising the flexibility of FPGA with processing power of ARM core yet having much lower price than available SoC solutions like Xilinx Zynq and being even more flexible and user friendly.

The prototype is built around 2 boards: XCore407I from WaveShare and XME0601 from PiSwords. They are interconnected by two buses. The slow SPI serial bus allows for data exchange with FPGA and serves for flash reprogramming. The fast DCMI bus provides the means for passing large amounts of data from FPGA to ARM at rate up to 50MByte/sec. Access to both buses as well as to firmware loading facility is provided to host applications via USB bus. The USB interface implements Test and Measurement Class to allow for simple integration with NI-VISA family of measurement software.

## The overview of the design

The problem of measuring something, preprocessing results and transferring them to desktop computer is constantly reappearing. In wide range of cases the resources of the single microcontroller is just not enough for data acquisition. The all-programmable SoC solutions like Xilinx Zynq looks like overkill for the problem in wast majority of cases. Besides being relatively expensive they have quite steep learning curve. The plain combination of cheep microcontroller and entry level FPGA will suffice in most cases, providing excellent flexibility and value-for-money. Besides, such system is much easy to learn and troubleshoot which is very important for reducing time to market of the end product. The only problem yet to be solved while designing such system is interface between microcontroller (MCU) and FPGA. In ideal case such interface should be fast, simple to implement and use not much pins of both MCU and FPGA.

The existing solutions used to implement MCU to FPGA interface via external memory interface controller (AKA FMC or FSMC). The problem with such approach is that it is much slower than the parallel bus could be. There are two reasons for that. First the memory bus access involves transferring address to the bus before data transfer. Second, memory bus access occurs via relatively slow internal interface. For example Zynq with 200MHz internal interface (AKA AXI4) is able to transfer to memory mapped 32 bit wide port only 25MBytes per second.

The solution implemented in this project is based on the DCMI (digital camera interface). It turns out that in JPEG mode DCMI is very handy. It looks like parallel analogue of SPI with vertical sync playing the role of CS signal. The only difference is that the clock must be provided even when VSINC is active and no data is transferred. The horizontal sync in such mode is not required at all. The downside of such solution is that the MCU may play the role of data receiver only. Therefore we implemented the second slow serial data channel based on the SPI protocol. It may be used for bi-directional data exchange as well as for triggering fast burst transfers via DCMI interface. Such solution is perfectly suitable for applications involving fast data acquisitions and transferring large amounts of data in one direction - from FPGA to MCU.

To cope with transferring large amounts of data the USB interface utilizes high speed port with external phy. The only downside of such solution is impossibility to utilize standard DFU-class based mechanism of updating MCU firmware via high speed port. If such possibility is required the second full speed port with internal phy should be utilized. It may be multiplexed to the same USB connection externally. To support seamless integration with National Instruments Virtual Instrumentation software (AKA LabView, VISA, etc) the USB port implements Test and Measurements Class (TMC). From the implementation point of view it looks like serial communication protocols with frame headers defined in the standard. There are two distinct kinds of frames for reading and writing. Device may return data to the host only upon receiving  read request. It never respond to the host by its own. The MCU provides access to both MCU to FPGA communication channels as well as to FPGA flash reprogramming facility via USB interface. The application level protocol implemented for that purpose follows to some extent to SCPI standard (Standard Commands for Programmable Instruments). The full set of commands implemented so far will be listed below.

## The core components interconnections

![The core component interconnections](https://github.com/olegv142/STM32AllProgrammable/blob/master/doc/schematic.png)

## The application level API

## FPGA firmware loading

## FPGA test projects

## Author

Oleg Volkov (olegv142@gmail.com)
