"""
The module provides API for accessing SPI flash via USB TMC interface.
It is tested with Winbond 25Q16 flash. Flash chips from other manufacturers may
require minor modifications to the code.
"""

import pyvisa as pv
import time

class PLFlash:
	STATUS_BUSY = 1
	max_rd_chunk = 4096
	program_page = 256

	def __init__(self, res = None):
		rm = pv.ResourceManager()
		if not res:
			res = rm.list_resources()[0]		
		self.dev = rm.open_resource(res)

	def pl_status(self):
		"""Returns the FPGA status: 0 - inactive, 1 - active, 2 - configured"""
		self.dev.write_raw('PL:ACTIVE?')
		sta = self.dev.read_raw()
		assert len(sta) == 1
		return ord(sta[0]) - ord('0')

	def __enter__(self):
		"""Reset FPGA to gain access to the flash"""
		self.dev.write_raw('PL:ACTIVE#0')
		started = time.time()
		while self.pl_status() != 0:
			if time.time() - started > 1000:
				raise RuntimeError('Timeout waiting PL inactive status')
		return self

	def __exit__(self, type, value, traceback):
		"""Resume FPGA"""
		self.dev.write_raw('PL:ACTIVE#1')

	def write(self, data):
		"""Send data to flash"""
		self.dev.write_raw('PL:FLASH:WR' + data)
		r = self.dev.read_raw()
		if len(r) > 0:
			raise RuntimeError('unexpected PL:FLASH:WR response')

	def read(self, data, sz):
		"""Send data and read response"""
		self.dev.write_raw('PL:FLASH:RD #H%X#' % sz + data)
		r = self.dev.read_raw()
		if len(r) != len(data) + sz:
			raise RuntimeError('unexpected PL:FLASH:RD response: expect %u + %u, got %u bytes' % (
					len(data), sz, len(r)))
		return r[len(data):]

	def read_id(self):
		"""Read flash ID tuple"""
		r = self.read('\x9f', 3)
		return ord(r[0]), ord(r[1]), ord(r[2])

	def query_size(self):
		"""Returns flash size in bytes"""
		man, mod, lsz = self.read_id()
		return 1 << lsz

	def read_status(self, i = 1):
		"""Read one of the 2 status registers"""
		assert i == 1 or i == 2
		r = self.read('\x05' if i == 1 else '\x35', 1)
		return ord(r[0])

	def write_en(self, en):
		"""Enable / disable writing to flash"""
		self.write('\x06' if en else '\x04')

	def wait_busy(self, tout_ms):
		"""Wait flash busy status clear"""
		tout = self.dev.timeout
		self.dev.timeout = tout_ms + 1000
		self.dev.write_raw('PL:FLASH:WAit #%u' % tout_ms)
		r = self.dev.read_raw()
		self.dev.timeout = tout
		if len(r) != 1:
			raise RuntimeError('unexpected PL:FLASH:WAit response')
		return (ord(r[0]) & PLFlash.STATUS_BUSY) == 0

	def write_safe(self, data, tout_ms):
		"""Combines waiting non-busy status, enabling write and writing data in single request"""
		tout = self.dev.timeout
		self.dev.timeout = tout_ms + 1000
		self.dev.write_raw('PL:FLASH:PRog #%u#' % tout_ms + data)
		r = self.dev.read_raw()
		self.dev.timeout = tout
		if len(r) != 1:
			raise RuntimeError('unexpected PL:FLASH:PRog response')
		return (ord(r[0]) & PLFlash.STATUS_BUSY) == 0

	def erase_all(self):
		"""Erase entire chip"""
		return self.write_safe('\xC7', 1000) and self.wait_busy(4000)

	def read_chunk(self, off, sz):
		"""Read chunk of flash content starting from the specified address"""
		assert sz <= PLFlash.max_rd_chunk
		addr = chr((off >> 16) & 0xff) + chr((off >> 8) & 0xff) + chr(off & 0xff)
		return self.read('\x03' + addr, sz)

	def read_content(self, addr, sz):
		"""Read flash content starting from the specified address"""
		buff = ''
		while sz:
			chunk = min(sz, PLFlash.max_rd_chunk)
			buff += self.read_chunk(addr, chunk)
			addr += chunk
			sz -= chunk
		return buff

	def prog_page(self, off, data):
		"""Program flash page or some part of it"""
		assert len(data) <= PLFlash.program_page - (off & (PLFlash.program_page-1))
		addr = chr((off >> 16) & 0xff) + chr((off >> 8) & 0xff) + chr(off & 0xff)
		return self.write_safe('\x02' + addr + data, 1000)

	def program(self, addr, data):
		sz, off = len(data), 0
		while sz:
			chunk = min(PLFlash.program_page - (addr & (PLFlash.program_page-1)), sz)
			if not self.prog_page(addr, data[off:off+chunk]):
				return False
			addr += chunk
			off += chunk
			sz -= chunk
		return self.wait_busy(1000)


if __name__ == '__main__':
	import sys
	import random

	with PLFlash() as dev:
		if '--erase' in sys.argv:
			if dev.erase_all():
				print 'chip erased'
			else:
				print 'failed to erase'
		elif '--chk-empty' in sys.argv:
			size = dev.query_size()
			print 'reading', size, 'bytes ..',
			content = dev.read_content(0, size)
			print 'done'
			for i, c in enumerate(content):
				if c != '\xff':
					print '%#0.2x @%#x' % (ord(c), i)
					break
			else:
				print 'empty'
		elif '--test' in sys.argv:
			size = dev.query_size()
			off = random.randrange(0, size / 2)
			sz  = random.randrange(1, size - off + 1)
			data = str(bytearray(random.getrandbits(8) for _ in xrange(sz)))
			print 'erasing ..'
			assert dev.erase_all()
			print 'writing %u bytes at offset %u ..' % (sz, off)
			assert dev.program(off, data)
			print 'verifying ..'
			rdata = dev.read_content(off, sz)
			assert len(rdata) == sz
			for i, c in enumerate(rdata):
				if c != data[i]:
					print '%#0.2x != %#0.2x @%#x' % (ord(c), ord(data[i]), i)
					break
			print 'done'
		else:
			print 'ID =', dev.read_id()

		print 'status: %#x, %#x' % (dev.read_status(1), dev.read_status(2))

