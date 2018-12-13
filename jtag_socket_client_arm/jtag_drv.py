from ctypes import *
from enum import IntEnum
#At Scale Debug commands
TAP_RESET = 0x14

TAP_STATE_MIN = 0x20
TAP_JtagTLR   = 0x20
TAP_JtagRTI   = 0x21
TAP_JtagSelDR = 0x22
TAP_JtagCapDR = 0x23
TAP_JtagShfDR = 0x24
TAP_JtagEx1DR = 0x25
TAP_JtagPauDR = 0x26
TAP_JtagEx2DR = 0x27
TAP_JtagUpdDR = 0x28
TAP_JtagSelIR = 0x29
TAP_JtagCapIR = 0x2A
TAP_JtagShfIR = 0x2B
TAP_JtagEx1IR = 0x2C
TAP_JtagPauIR = 0x2D
TAP_JtagEx2IR = 0x2E
TAP_JtagUpdIR = 0x2F
TAP_STATE_MAX = 0x2F
TAP_STATE_MASK = 0xF 

WRITE_SCAN_MIN = 0x40
WRITE_SCAN_MAX = 0x7F

READ_SCAN_MIN = 0x80
READ_SCAN_MAX = 0xBF

READ_WRITE_SCAN_MIN = 0xC0
READ_WRITE_SCAN_MAX = 0xFF

SCAN_LENGTH_MASK = 0x3F

# Header type
class ASD_header(IntEnum):
        AGENT_CONTROL_TYPE = 0
        JTAG_TYPE = 1
        PROBE_MODE_TYPE = 2
        DMA_TYPE = 3

origin_id_pos = 0
reserved_pos = 3
enc_bit_pos = 4
type_pos = 5
size_lsb_pos = 8
size_msb_pos = 16
tag_pos = 21
cmd_stat_pos = 24

origin_id_mask = 0x7 << origin_id_pos
reserved_mask  = 0x1 << reserved_pos
enc_bit_mask   = 0x1 << enc_bit_pos
type_mask      = 0x7 << type_pos
size_lsb_mask  = 0xFF << size_lsb_pos
size_msb_mask  = 0x1F << size_msb_pos
tag_mask       = 0x7 << tag_pos
cmd_stat_mask  = 0xFF << cmd_stat_pos

class message_header(Structure):
    _pack_ = 1 
    _fields_ = [('origin_id', c_ubyte, 3),
                ('reserved', c_ubyte, 1),
                ('enc_bit', c_ubyte, 1),
                ('type', c_ubyte, 3),
                ('size_lsb', c_ubyte, 8),
                ('size_msb', c_ubyte, 5),
                ('tag', c_ubyte, 3),
                ('cmd_stat', c_ubyte, 8)
               ]

    def __init__(self):
        self.origin_id = 0x5
        self.reserved = 0x0
        self.enc_bit = 0x0
        self.type = 0x0
        self.size_lsb = 0x0
        self.size_msb = 0x0
        self.tag = 0x0
        self.cmd_tag = 0x0

def jtag_parse_header(response):
    response_inbyte = bytearray(response)
    response_header = int.from_bytes(response_inbyte[0:4], byteorder='little')
    header = message_header()
    header.origin_id = (response_header & origin_id_mask) >> origin_id_pos
    header.reserved = (response_header & reserved_mask) >> reserved_pos
    header.enc_bit = (response_header & enc_bit_mask) >> enc_bit_pos
    header.type = (response_header & type_mask) >> type_pos
    header.size_lsb = (response_header & size_lsb_mask) >> size_lsb_pos
    header.size_msb = (response_header & size_msb_mask) >> size_msb_pos
    header.tag = (response_header & tag_mask) >> tag_pos
    header.cmd_stat = (response_header & cmd_stat_mask) >> cmd_stat_pos
    return header

def jtag_calc_size(header, buff):
    length = len(buff)
    if length < 255:
        header.size_lsb = length & 0xFF
        header.size_msb = 0
    else:
        header.size_lsb = 0xFF
        header.size_msb = (length >> 8) & 0x1F

def read_int_from_response(response):
    recvheader = jtag_parse_header(response)
    if recvheader.type == ASD_header.JTAG_TYPE:
        payload = response[4:]
        cnt = 0 
        cmd = payload[cnt]
        cnt += 1
        if (cmd >= READ_SCAN_MIN and cmd <= READ_SCAN_MAX):
            read_bytes = int((cmd - READ_SCAN_MIN + 7) / 8)
        elif (cmd >= READ_WRITE_SCAN_MIN and cmd <= READ_WRITE_SCAN_MAX):
            read_bytes = int((cmd - READ_WRITE_SCAN_MIN + 7) / 8)
        integer = int.from_bytes(payload[cnt:cnt + read_bytes], byteorder='little')
        return integer
    return 0