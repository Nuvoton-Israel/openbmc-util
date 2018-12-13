from ctypes import *
from jtag_drv import *

#-----------------------
ARM_REGISTER_LEN_SCAN1_0 = 33
ARM_REGISTER_LEN_SCAN1_1 = 67
ARM_REGISTER_LEN_SCAN2 = 38
#-----------------------
SCAN_0 = 0 # reversed
SCAN_1 = 1 # debug
SCAN_2 = 2 # EmbeddedICE-RT programming
SCAN_3 = 3 # External boundary scan
#-----------------------
class SCAN_1_RW(IntEnum):
        W = 0x1
        RW = 0x2

class SCAN_2_RW(IntEnum):
        R = 0x0
        W = 0x1
        RW = 0x2
# ARM IR Instructions
IR_SIZE = 0x4
SCAN_N = 0x2 
INTEST = 0xC 
IDCODE = 0xE 
BYPASS = 0xF 
RESTART = 0x4
SCAN_CNAIN_REG_SIZE = 0x5
#-----------------------
# ARM instruction
ARM_NOP = 0xE1A00000
#-----------------------
# ARM Processor mode
ABT = 0x17
FIQ = 0x11
IRQ = 0x12
SVC = 0x13
SYS = 0x1F
UND = 0x1B
USR = 0x10
PROCESSOR_MODE_MASK = 0x1F
THUMB_STATE_MASK = 0x20
#-----------------------
# ARM9E-S core EmbeddedICE-RT
ICE_REG_ADDR = 0
ICE_REG_WIDTH = 1
DBGCTRL = "DBGCTRL"
DBGSTAT = "DBGSTAT"
VECCCTRL = "VECCCTRL"
DBGCCTRL = "DBGCCTRL"
DBGCDATA = "DBGCDATA"
WP0ADDRVAL = "WP0ADDRVAL"
WP0ADDRMSK = "WP0ADDRMSK"
WP0DATAVAL = "WP0DATAVAL"
WP0DATAMSK = "WP0DATAMSK"
WP0CTRLVAL = "WP0CTRLVAL"
WP0CTRLMSK = "WP0CTRLMSK"
WP1ADDRVAL = "WP1ADDRVAL"
WP1ADDRMSK = "WP1ADDRMSK"
WP1DATAVAL = "WP1DATAVAL"
WP1DATAMSK = "WP1DATAMSK"
WP1CTRLVAL = "WP1CTRLVAL"
WP1CTRLMSK = "WP1CTRLMSK"
ARM_ICE_REG = {
        DBGCTRL: [0x00, 6],       # Debug control                 R/W
        DBGSTAT: [0x01, 5],       # Debug status                  R-O
        VECCCTRL: [0x02, 8],      # Vector catch control          R/W
        DBGCCTRL: [0x04, 6],      # Debug comms control           R-O
        DBGCDATA: [0x05, 32],     # Debug comms data              R/W
        WP0ADDRVAL: [0x08, 32],   # Watchpoint 0 address value    R/W
        WP0ADDRMSK: [0x09, 32],   # Watchpoint 0 address mask     R/W
        WP0DATAVAL: [0x0A, 32],   # Watchpoint 0 data value       R/W
        WP0DATAMSK: [0x0B, 32],   # Watchpoint 0 data mask        R/W
        WP0CTRLVAL: [0x0C, 9],    # Watchpoint 0 control value    R/W
        WP0CTRLMSK: [0x0D, 8],    # Watchpoint 0 control mask     R/W
        WP1ADDRVAL: [0x10, 32],   # Watchpoint 1 address value    R/W
        WP1ADDRMSK: [0x11, 32],   # Watchpoint 1 address mask     R/W
        WP1DATAVAL: [0x12, 32],   # Watchpoint 1 data value       R/W
        WP1DATAMSK: [0x13, 32],   # Watchpoint 1 data mask        R/W
        WP1CTRLVAL: [0x14, 9],    # Watchpoint 1 control value    R/W
        WP1CTRLMSK: [0x15, 8]     # Watchpoint 1 control mask     R/W
}

class DBGCTRLREG(IntEnum):
        DBGACK = 0x1
        DBGRQ = 0x2
        INTDIS = 0x4
        MONITOR = 0x10
        EmbICEdisable = 0x20
#-----------------------

def jtag_compose_chain1_0(buff, sysspeed, opcode, rw):
    buff.append(TAP_JtagShfDR)
    if rw == SCAN_1_RW.W:
        buff.append(WRITE_SCAN_MIN + ARM_REGISTER_LEN_SCAN1_0)
    elif rw == SCAN_1_RW.RW:
        buff.append(READ_WRITE_SCAN_MIN + ARM_REGISTER_LEN_SCAN1_0)
    reverse = int('{0:32b}'.format(opcode)[::-1], 2)
    buff.append(((reverse & 0xFF) << 1) & 0xFF | sysspeed)
    buff.append((reverse >> 7) & 0xFF)
    buff.append((reverse >> 15) & 0xFF)
    buff.append((reverse >> 23) & 0xFF)
    buff.append((reverse >> 31))
    buff.append(TAP_JtagRTI)

def jtag_compose_chain1_1(buff, opcode, reg):
    buff.append(TAP_JtagShfDR)
    buff.append(WRITE_SCAN_MAX)
    buff.append(ARM_REGISTER_LEN_SCAN1_1)
    buff.append((reg) & 0xFF)
    buff.append((reg >> 8) & 0xFF)
    buff.append((reg >> 16) & 0xFF)
    buff.append((reg >> 24) & 0xFF)
    reverse = int('{0:32b}'.format(opcode)[::-1], 2)
    buff.append(((reverse & 0xFF) << 3) & 0xFF)
    buff.append((reverse >> 5) & 0xFF)
    buff.append((reverse >> 13) & 0xFF)
    buff.append((reverse >> 21) & 0xFF)
    buff.append((reverse >> 29))
    buff.append(TAP_JtagRTI)
#------------------------------------
#| R/W (1) | Address (4) | Data (32)|
#------------------------------------
def jtag_compose_chain2(buff, addr, val, rw):
    buff.append(TAP_JtagShfDR)
    if rw == SCAN_2_RW.W:
        buff.append(WRITE_SCAN_MIN + ARM_REGISTER_LEN_SCAN2)
    buff.append(val & 0xFF)
    buff.append((val >> 8) & 0xFF)
    buff.append((val >> 16) & 0xFF)
    buff.append((val >> 24) & 0xFF)
    buff.append((rw << 5) | addr)
    buff.append(TAP_JtagRTI)

def jtag_compose_ir(buff, instruction):
    buff.append(TAP_JtagShfIR)
    buff.append(WRITE_SCAN_MIN + IR_SIZE)
    buff.append(instruction)
    buff.append(TAP_JtagEx1IR)
    
def jtag_select_scan_chain(buff, chain):
    jtag_compose_ir(buff, SCAN_N)
    buff.append(TAP_JtagShfDR)
    buff.append(WRITE_SCAN_MIN + SCAN_CNAIN_REG_SIZE)
    buff.append(chain)
    buff.append(TAP_JtagEx1DR)
    jtag_compose_ir(buff, INTEST)

def jtag_read_reg(buff, opcode):
    jtag_compose_chain1_0(buff, 0, opcode, SCAN_1_RW.W)
    jtag_compose_chain1_0(buff, 0, ARM_NOP, SCAN_1_RW.W)
    buff.append(TAP_JtagRTI)
    jtag_compose_chain1_0(buff, 0, ARM_NOP, SCAN_1_RW.RW)

def jtag_write_reg(buff, opcode, val):
    jtag_compose_chain1_0(buff, 0, opcode, SCAN_1_RW.W)
    jtag_compose_chain1_0(buff, 0, ARM_NOP, SCAN_1_RW.W)
    buff.append(TAP_JtagRTI)
    jtag_compose_chain1_1(buff, ARM_NOP, val)
    buff.append(TAP_JtagRTI)

def jtag_execute_cmd(buff, opcode):
    jtag_compose_chain1_0(buff, 0, opcode, SCAN_1_RW.W)
    jtag_compose_chain1_0(buff, 0, ARM_NOP, SCAN_1_RW.W)
    buff.append(TAP_JtagRTI)
    buff.append(TAP_JtagRTI)
    buff.append(TAP_JtagRTI)