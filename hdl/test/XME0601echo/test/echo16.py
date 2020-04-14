import sys
import random

sys.path.append('../../../../python')
import pl

ECHO16_PORT = 2

dev = pl.open()
print 'PL is', pl.get_status_name(dev)
cnt, last = 0, None
while True:
	data = str(bytearray(random.getrandbits(8) for _ in range(2)))
	resp = pl.tx(dev, ECHO16_PORT, data)
	assert last is None or resp == last
	last = data
	cnt += 1
	if cnt % 100 == 0:
		print '*',
