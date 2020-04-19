#
# Capture and print chip scope traces.
# The chip scope is implemented as DCMICaptureBuffer
# module in hdl/lib/dcmi_util.v. See hdl/test/XME0601echo/echo.v
# and hdl/test/XME0601echo/test/capture.py for usage example.
#

import pl

TRACE_LEN = 1024 # In 32 bit words

def capture_trace(dev, addr, cmd):
	return pl.pull(dev, addr, cmd, TRACE_LEN)

def print_trace(t, f, line_len = 128, hdrs = ()):
	size, off = len(t), 0
	hwidth = 0
	if hdrs: hwidth = max(len(h) for h in hdrs)
	hwidth = 2 + max(1, hwidth)
	skip, last_byte = False, None
	while size:
		chunk = min(size, line_len)
		for c in t[off:off+chunk]:
			if c != last_byte:
				break
		else:
			if not skip:
				skip = True
				f.write('...\n\n')
			size -= chunk
			off += chunk
			continue

		skip = False
		f.write('+%u\n' % off)
		for b in range(8):
			if b < len(hdrs):
				h = hdrs[b]
			else:
				h = '_'
			f.write(h + ' ' * (hwidth - len(h)))
			bit = 1 << b
			for i in range(chunk):
				if ord(t[off + i]) & bit:
					f.write('-')
				else:
					f.write('_')
			f.write('\n')
		f.write('\n')
		last_byte = t[off + chunk - 1]
		size -= chunk
		off += chunk
