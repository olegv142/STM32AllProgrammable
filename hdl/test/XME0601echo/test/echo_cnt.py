import sys
import random

sys.path.append('../../../../python')
import pl

ECHO_CNT_PORT = 5

cnt_str = ''.join([chr(i) for i in range(256)])
master_str = cnt_str * 4

dev = pl.open()
print 'PL is', pl.get_status_name(dev)
cnt = 0
while True:
	sz = random.randrange(1, len(master_str) + 1)
	resp = pl.tx(dev, ECHO_CNT_PORT, '\0' * sz)
	assert resp == master_str[:sz]
	cnt += 1
	if cnt % 100 == 0:
		print '*',
