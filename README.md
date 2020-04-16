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

The above figure shows 17 signals connecting FPGA and MCU. 3 of them belong to SPI port of MCU, 10 belong to DCMI port and 4 are GPIO pins - 2 used as SPI chip select signals and 2 are used to control FPGA configuration state. To save pins the same SPI signals are used for flash programming and data exchange with FPGA. Those two modes use separate chip select signals shown as FLASH_CS and PL_CS on the figure. The former is connected to the flash so it is used for flash programming. To gain access to the flash the MCU set PROGRAM_B signal to low. While it is low the FPGA turns all its pins to high impedance state. When MCU releases control of the flash it turns SPI pins to high impedance state and set PROGRAM_B to high allowing FPGA to configure itself. Once FPA configuration completes the DONE signal turns high. The MCU monitors DONE signal and reconfigure SPI interface when its high to be able to exchange data with FPGA. The DCMI bus connection is pretty straightforward. The FPGA plays the master role so every transaction should be triggered by MCU using SPI communication channel. The horizontal sync signal is not used and just connected to ground at MCU.

## The application level API
The application can communicate with MCU by sending messages and reading responses via USB TMC. Messages are always start from the text command optionally followed by numeric parameters and/or binary data packet. Parameters always start from # symbol. The number may begin with radix prefix - one of the X, x, H, h (hex), Q (octal) and B (binary). If the parameter is followed by binary data the # symbol is used as separator. There are 2 types of commands - system and device-specific. The system commands start from * symbol. The device commands start from : symbol and may form the hierarchy. The first : prefix at the top of the hierarchy may be omitted. Spaces may be optionally inserted between hierarchical parts of the command and between command and parameter. Only single command may be sent at a time. Below is the list of commands supported so far.

__*IDN?__  
The only system command supported. Returns device identification string that consists of vendor name, product name, serial number and version separated by commas.

__TEST:ECHO\<DATA\>__  
Reply with the same data that follows the command. The maximum size of the data is 1024 bytes. The test/echo.py script uses this command to test USB communications.

__PL:ACTIVE?__  
Returns single character string describing the configuration state of the FPGA: '0' - idle, '1' - active, not configured, '2' - active, configured

__PL:ACTIVE #N__  
Change the configuration state of the FPGA according to the numerical parameter passed. Technically the command just change the state of the PROGRAM_B signal. Zero N turns PROGRAM_B low, so the FPGA becomes idle, any non-zero N turns PROGRAM_B high so the FPGA becomes active and make an attempt to configure itself. The command has no response.

__PL:FLASH:WR\<PACKET\>__  
Send binary data packet to FPGA flash. The FPGA state should be set to idle before using any PL:FLASH commands. The data packet typically starts from the flash command byte optionally followed by the address and data. See flash chip datasheet for the details. The PL:FLASH:WR command just treats the data packet as opaque. The command response is empty string.

__PL:FLASH:RD #Len#\<PACKET\>__  
Send binary data packet to FPGA flash and read response. The command response will include the data read from the flash in response to the packet plus additional Len bytes read after the packet is sent.

__PL:FLASH:WAit #Tout__  
Continuously read flash status register waiting for the flash to become non-busy. The Tout parameter specifies the maximum time to wait in milliseconds. The command response is single flash status byte so the sender may check if the wait was successful or not.

__PL:FLASH:PRog #Tout#\<PACKET\>__  
This is the flash programming helper command. It combines waiting for the non-busy status, enabling write and writing binary data PACKET in the single command. In case the status waiting was not succeeded within Tout (expressed in milliseconds) the last two operations are skipped. The command response is single flash status byte so the sender may check if the wait (and so subsequent write) was successful or not.

__PL:TX\<PACKET\>__  
Exchange binary data PACKET with FPGA via SPI bus. Respond with the data received. So the response length always equals to the PACKET length.

__PL:PULL #Len#\<PACKET\>__  
Receive data from FPGA via DCMI bus. The binary data PACKET will be sent first via SPI bus to trigger data transaction on DCMI bus (so the FPGA should handle PACKET sent appropriately). The data length parameter Len is expressed in 4 byte units so there is no way to receive an amount of data with size not multiple of 4. The response is the concatenation of the response to the PACKET received via FPGA bus and the data received via DCMI bus. Those two parts of the response may be separated by alignment padding.

The python modules python/pl.py and python/pl_flash.py contains code for sending requests described above as well as receiving and parsing responses.

## FPGA firmware loading
The API described above have all necessary commands to load FPGA firmware to the flash. The python/pl_flash.py module implements flash loading routine. To use it issue the following command in console:  
**python pl_flash.py --program \<your_firmware.bin\>**  
Note that you need binary firmware image rather than bitstream file. Creating binary image is not enabled by default in Xilinx ISE. You need to go to the Process Properties of Generate Programming File (by right-clicking it) and select Create Binary Configuration File option. You can also increase Configuration Rate and set proper SPI Configuration Bus Width (4 for the quad SPI) to speed up FPGA configuration.

## FPGA API and test projects

## Author

Oleg Volkov (olegv142@gmail.com)
