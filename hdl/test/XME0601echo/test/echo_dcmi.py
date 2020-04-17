import sys
import random

sys.path.append('../../../../python')
import pl

ECHO8_PORT = 1
ECHO16_PORT = 2
ECHO_DCMI_PORT = 4
DCMI_ECHO_MAX = 1024

cnt = 0
dev = pl.open()
print 'PL is', pl.get_status_name(dev)
while True:
	sz = random.randrange(1, 1 + DCMI_ECHO_MAX / 4) # in 32 bit words
	data = str(bytearray(random.getrandbits(8) for _ in xrange(sz*4)))
	rdata = pl.pull(dev, ECHO_DCMI_PORT, data, sz)
	assert rdata == data
	cnt += 1
	if cnt % 100 == 0:
		print '*',

