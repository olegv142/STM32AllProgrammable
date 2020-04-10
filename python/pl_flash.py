"""
The module provides API for accessing SPI flash via USB TMC interface.
It is tested with Winbond 25Q16 flash. Flash chips from other manufacturers may
require minor modifications to the code.
"""

import pyvisa as pv

STATUS_BUSY = 1

max_rd_chunk = 4096
program_page = 256

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
		raise RuntimeError('unexpected PL:FLASH:RD response: expect %u + %u, got %u bytes' % (
				len(data), sz, len(r)))
	return r[len(data):]

def read_id(dev):
	"""Read flash ID tuple"""
	r = read(dev, '\x9f', 3)
	return ord(r[0]), ord(r[1]), ord(r[2])

def query_size(dev):
	"""Returns flash size in bytes"""
	man, mod, lsz = read_id(dev)
	return 1 << lsz

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

def read_chunk(dev, off, sz):
	"""Read chunk of flash content starting from the specified address"""
	assert sz <= max_rd_chunk
	addr = chr((off >> 16) & 0xff) + chr((off >> 8) & 0xff) + chr(off & 0xff)
	return read(dev, '\x03' + addr, sz)

def read_content(dev, addr, sz):
	"""Read flash content starting from the specified address"""
	buff = ''
	while sz:
		chunk = min(sz, max_rd_chunk)
		buff += read_chunk(dev, addr, chunk)
		addr += chunk
		sz -= chunk
	return buff

def prog_page(dev, off, data):
	"""Program flash page or some part of it"""
	assert len(data) <= program_page - (off & (program_page-1))
	addr = chr((off >> 16) & 0xff) + chr((off >> 8) & 0xff) + chr(off & 0xff)
	return write_safe(dev, '\x02' + addr + data, 1000)

def program(dev, addr, data):
	sz, off = len(data), 0
	while sz:
		chunk = min(program_page - (addr & (program_page-1)), sz)
		if not prog_page(dev, addr, data[off:off+chunk]):
			return False
		addr += chunk
		off += chunk
		sz -= chunk
	return wait_busy(dev, 1000)


if __name__ == '__main__':
	import sys
	import random

	dev = open()
	if '--erase' in sys.argv:
		if erase_all(dev):
			print 'chip erased'
		else:
			print 'failed to erase'
	elif '--chk-empty' in sys.argv:
		size = query_size(dev)
		print 'reading', size, 'bytes ..',
		content = read_content(dev, 0, size)
		print 'done'
		for i, c in enumerate(content):
			if c != '\xff':
				print '%#0.2x @%#x' % (ord(c), i)
				break
		else:
			print 'empty'
	elif '--test' in sys.argv:
		size = query_size(dev)
		off = random.randrange(0, size / 2)
		sz  = random.randrange(1, size - off + 1)
		data = str(bytearray(random.getrandbits(8) for _ in xrange(sz)))
		print 'erasing ..'
		assert erase_all(dev)
		print 'writing %u bytes at offset %u ..' % (sz, off)
		assert program(dev, off, data)
		print 'verifying ..'
		assert read_content(dev, off, sz) == data
		print 'done'
	else:
		print 'ID =', read_id(dev)

	print 'status: %#x, %#x' % (read_status(dev, 1), read_status(dev, 2))

