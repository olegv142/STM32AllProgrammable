import sys
sys.path.append('../../../../python')
import pl
import chipscope as cs

#
# Capture SPI receiver signal, see XME0601echo/echo.v for details
#
CAPTURE_PORT = 6
CAPTURE_CMD = '\1\2\3\4'
HDRS = ('nCS', 'SCLK', 'MOSI', 'SEL', 'RXE', 'START', 'STRB', 'DONE')

cs.print_trace(cs.capture_trace(pl.open(), CAPTURE_PORT, CAPTURE_CMD), sys.stdout, hdrs = HDRS)
