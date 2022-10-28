
from dataclasses import dataclass
from pathlib import Path
import sys


def fromhex(hex_string: str) -> bytearray:
    return bytearray(int(x, 16) for x in  hex_string)

def ashex(value: int) -> str:
    return hex(value)[2:].zfill(2).upper()


@dataclass(init=False)
class HexRecord:

    def __init__(self, line: str) -> None:
        content = line.split(':')[-1]
        self.size        = int(content[:2],  16) # Byte count, two hex digits
        self.address     = int(content[2:6], 16) # Address, four hex digits
        self.address_hex = ashex(self.address)
        self.type        = int(content[6:8], 16) # Record type, two hex digits
        self.raw         = content
        data             = content[8:8+self.size*2]
        self.data        = [int(data[i:i+2], 16) for i in range(0, len(data), 2)]
        self.data_hex    = ''.join([ashex(d) for d in self.data])
        self.crc         = int(content[-2:], 16) # CRC, two hex digits

    def __str__(self) -> str:
        return f'Size: {ashex(self.size)}, Address: {ashex(self.address)}, Type: {ashex(self.type)}, Crc: {ashex(self.crc)}'


from flask import Flask, render_template, request
app = Flask(__name__, template_folder=Path(__file__).absolute().parent)


#hexpath = '/home/victor/Documents/PlatformIO/Projects/TEST_BUSYBEE/.pio/build/efm8bb1/firmware.hex'
hexpath = '/home/victor/Documents/PlatformIO/Projects/TEST_BUSYBEE/build/firmware.hex'

def read_hex_file(path) -> str:
    with open(path) as f:
        return f.read()


@app.route('/')
def index():
    hexdata = read_hex_file(hexpath)
    records = [HexRecord(line) for line in hexdata.splitlines()]
    return render_template('hexviewer.html', records=records)
