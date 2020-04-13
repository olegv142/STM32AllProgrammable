import pyvisa as pv
import struct
import sys

STA_INACTIVE   = 0
STA_ACTIVE     = 1
STA_CONFIGURED = 2

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
	rm = pv.ResourceManager()
	if not res:
		res = rm.list_resources()[0]		
	return rm.open_resource(res)

def get_status(dev):
	dev.write_raw('PL:ACTIVE?')
	r = dev.read_raw()
	assert len(r) == 1
	return ord(r[0]) - ord('0')

def get_status_name(dev):
	return status_name(get_status(dev))

def tx(dev, addr, data):
	dev.write_raw('PL:TX' + chr(addr) + data)
	r = dev.read_raw()
	assert len(r) == 1 + len(data)
	return r[1:]

if __name__ == '__main__':
	dev = open()
	print 'PL is', get_status_name(dev)

