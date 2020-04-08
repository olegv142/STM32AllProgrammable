import pyvisa as pv
import random
import time

max_size = 1024
rand_buff = None

def init():
	global rand_buff
	l = [chr(i) for i in range(255)]
	random.shuffle(l)
	l = l * (2 * max_size / len(l))
	random.shuffle(l)
	rand_buff = ''.join(l)	

def open_device():
	rm = pv.ResourceManager()
	r = rm.list_resources()
	return rm.open_resource(r[0])

def random_buff():
	sz = random.randrange(0, max_size + 1)
	off = random.randrange(0, len(rand_buff) - sz + 1)
	return rand_buff[off:off+sz]

def echo(dev, b):
	dev.write_raw(':ECHO' + b)
	r = dev.read_raw(max_size)
	if r != b:
		print 'data mismatch'
		print len(b), 'written', len(r), 'read'
		raise RuntimeError

def test():
	init()
	dev = open_device()
	print dev.query('*IDN?')
	loops, bytes = 0, 0
	started = time.time() 
	try:
		while True:
			b = random_buff()
			echo(dev, b)
			loops += 1
			bytes += len(b)
			if loops % 100 == 0:
				print '*',
	except KeyboardInterrupt:
		pass
	elapsed = time.time() - started
	print
	print loops / elapsed, 'reqs / sec'
	print bytes / (1000 * elapsed), 'KB / sec'

if __name__ == '__main__':
	test()


