import pyvisa as pv
import struct
import sys

#
# FPGA status values
#
STA_INACTIVE   = 0 # inactive (PROGRAM_B low)
STA_ACTIVE     = 1 # active, not configured
STA_CONFIGURED = 2 # active, configured

sta_names = {
	STA_INACTIVE   : "Inactive",
	STA_ACTIVE     : "Active",
	STA_CONFIGURED : "Configured"
}

def status_name(sta):
	if sta in sta_names:
		return sta_names[sta]
	else:
		return "Unknown"

def open(res = None):
	"""Open device"""
	rm = pv.ResourceManager()
	if not res:
		res = rm.list_resources()[0]		
	return rm.open_resource(res)

def get_status(dev):
	"""Get FPGA status"""
	dev.write_raw('PL:ACTIVE?')
	r = dev.read_raw()
	assert len(r) == 1
	return ord(r[0]) - ord('0')

def get_status_name(dev):
	"""Get FPGA status string representation"""
	return status_name(get_status(dev))

def tx(dev, addr, data):
	"""
	Exchange data with FPGA over SPI bus given the target port address and data to transmit.
	Returns data receiving during SPI data exchange.
	"""
	dev.write_raw('PL:TX' + chr(addr) + data)
	r = dev.read_raw()
	assert len(r) == 1 + len(data)
	return r[1:]

def pull_raw(dev, addr, cmd, frame_words):
	"""
	Receive data over DCMI bus given the port address and command that should be sent over SPI to
	trigger DCMI data transfer as well as the number of 4 byte words to receive. Returns the
	concatenation of the SPI and DCMI data received with possible padding in between.
	"""
	dev.write_raw('PL:PULL #H%X#' % frame_words + chr(addr) + cmd)
	return dev.read_raw()

def pull(dev, addr, cmd, frame_words):
	"""
	Receive data over DCMI bus given the port address and command that should be sent over SPI to
	trigger DCMI data transfer as well as the number of 4 byte words to receive. Returns the DCMI
	data frame received.
	"""
	r = pull_raw(dev, addr, cmd, frame_words)
	return r[-frame_words*4:]

if __name__ == '__main__':
	dev = open()
	print 'PL is', get_status_name(dev)

