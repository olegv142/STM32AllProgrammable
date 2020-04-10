import pyvisa as pv

STATUS_BUSY = 1

def open(res = None):
	rm = pv.ResourceManager()
	if not res:
		res = rm.list_resources()[0]		
	return rm.open_resource(res)

def write(dev, data):
	"""Send data to flash"""
	dev.write_raw('PL:FLASH:WR' + data)
	r = dev.read_raw()
	if len(r) > 0:
		raise RuntimeError('unexpected PL:FLASH:WR response')

def read(dev, data, sz):
	"""Send data and read response"""
	dev.write_raw('PL:FLASH:RD #H%X#' % sz + data)
	r = dev.read_raw()
	if len(r) != len(data) + sz:
		raise RuntimeError('unexpected PL:FLASH:RD response')
	return r[len(data):]

def read_id(dev):
	"""Read flash ID tuple"""
	r = read(dev, '\x9f', 3)
	return ord(r[0]), ord(r[1]), ord(r[2])

def read_status(dev, i = 1):
	"""Read one of the 2 status registers"""
	assert i == 1 or i == 2
	r = read(dev, '\x05' if i == 1 else '\x35', 1)
	return ord(r[0])

def write_en(dev, en):
	"""Enable / disable writing to flash"""
	write(dev, '\x06' if en else '\x04')

def wait_busy(dev, tout_ms):
	"""Wait flash busy status clear"""
	tout = dev.timeout
	dev.timeout = tout_ms + 1000
	dev.write_raw('PL:FLASH:WAit #%u' % tout_ms)
	r = dev.read_raw()
	dev.timeout = tout
	if len(r) != 1:
		raise RuntimeError('unexpected PL:FLASH:WAit response')
	return (ord(r[0]) & STATUS_BUSY) == 0

def write_safe(dev, data, tout_ms):
	"""Combines waiting non-busy status, enabling write and writing data in single request"""
	tout = dev.timeout
	dev.timeout = tout_ms + 1000
	dev.write_raw('PL:FLASH:PRog #%u#' % tout_ms + data)
	r = dev.read_raw()
	dev.timeout = tout
	if len(r) != 1:
		raise RuntimeError('unexpected PL:FLASH:PRog response')
	return (ord(r[0]) & STATUS_BUSY) == 0

def erase_all(dev):
	"""Erase entire chip"""
	return write_safe(dev, '\xC7', 1000) and wait_busy(dev, 4000)


if __name__ == '__main__':
	import sys

	dev = open()
	if '--erase' in sys.argv:
		if erase_all(dev):
			print 'chip erased'
		else:
			print 'failed to erase'
	else:
		print 'ID =', read_id(dev)

	print 'Status-1 = %#x' % read_status(dev, 1)
	print 'Status-2 = %#x' % read_status(dev, 2)

