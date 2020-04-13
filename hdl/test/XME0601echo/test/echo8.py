import sys
import random

sys.path.append('../../../../python')
import pl

ECHO8_PORT = 1

dev = pl.open()
print 'PL is', pl.get_status_name(dev)
cnt, last_byte = 0, None
while True:
	sz  = random.randrange(1, 1024)
	data = str(bytearray(random.getrandbits(8) for _ in xrange(sz)))
	resp = pl.tx(dev, ECHO8_PORT, data)
	if last_byte != None:
		assert resp[0] == last_byte
	assert resp[1:] == data[:-1]
	last_byte = data[-1]
	cnt += 1
	if cnt % 100 == 0:
		print '*',
