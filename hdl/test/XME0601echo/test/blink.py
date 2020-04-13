import sys
import time

sys.path.append('../../../../python')
import pl

LED_PORT = 0
LED_RD_VAL = 0xdead

dev = pl.open()
print 'PL is', pl.get_status_name(dev)
while True:
	assert pl.tx(dev, LED_PORT, 1)[0] == LED_RD_VAL
	time.sleep(.5)
	assert pl.tx(dev, LED_PORT, 2)[0] == LED_RD_VAL
	time.sleep(.5)
