from __future__ import print_function

import sys
from os.path import isfile


firmware_hex = sys.argv[1]
assert isfile(firmware_hex)
firmware_mem = firmware_hex[0:-3] + "mem"
with open(firmware_mem) as fp:
    print(fp.read())
