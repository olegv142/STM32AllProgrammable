import pyvisa as pv
import sys

rm = pv.ResourceManager()
dev = rm.open_resource(rm.list_resources()[0])

dev.write_raw('PL:ACTIVE?')
print 'status=' + dev.read_raw()

