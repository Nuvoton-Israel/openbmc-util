import socket, sys, errno
from jtag_arm9_func import *

target_host = "192.168.2.100"
target_port = 5123


if __name__ == '__main__':
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # registers
        PC = 0
        cpsr = 0
        Regs = [0] * 15 # R15 is PC
        client.connect((target_host, target_port))
        buff = bytearray()
        header = message_header()
        header.type = ASD_header.JTAG_TYPE
        buff.append(TAP_RESET)
        buff.append(TAP_JtagRTI)
        jtag_calc_size(header, buff)
        client.sendall(string_at(addressof(header), sizeof(header)) + buff)
        response = bytearray(client.recv(128))
        while True:
            client_cmd = input("jtag_client>>>").split()
            args = len(client_cmd)
            header = message_header()
            header.type = ASD_header.JTAG_TYPE
            buff = bytearray()
            if client_cmd[0] == "?":
                print("id - read target ID code")
                print("halt - halts the CPU core")
                print("go - starts the CPU core")
                print("wreg - write register")
                print("rreg - read register")
                print("regs - show all registers")
                print("mem32 - read 32-bit item")
                print("wmem32 - write 32-bit item")
                print("setBP - set breakpoint")
                print("clrBP - clear breakpoint")
                print("quit - close jtag-client")
            elif client_cmd[0] == "id":      
                buff.append(TAP_RESET)
                jtag_compose_ir(buff, IDCODE)
                buff.append(TAP_JtagShfDR)
                buff.append(READ_SCAN_MIN + 32)                                                                                                                                                                               
                buff.append(TAP_JtagRTI)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
                idcode = read_int_from_response(response)
                print(hex(idcode))
            elif client_cmd[0] == "halt":
                print("halt")
                jtag_select_scan_chain(buff, SCAN_2)
                # Rasie DBGRQ
                jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], DBGCTRLREG.DBGRQ, SCAN_2_RW.W)
                buff.append(TAP_JtagRTI)
                # clear DBGRQ
                jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], 0x0, SCAN_2_RW.W)
                #### store PC ####
                # select scan chain 1
                jtag_select_scan_chain(buff, SCAN_1)
                # STR R0, [R0]  opcode: 0xE5800000
                jtag_read_reg(buff, 0xE5800000)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
                Regs[0] = read_int_from_response(response) # get R0
                buff = bytearray()
                # MOV R0, PC opcode: 0xE1A0000F
                jtag_execute_cmd(buff, 0xE1A0000F)
                # STR R0, [R0] opcode: 0xE5800000
                jtag_read_reg(buff, 0xE5800000)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
                PC = read_int_from_response(response)
                PC = PC - 0x24 # PC = PC - (4 + N + 5S), here N = 5, S = 0
                # dump cpsr
                buff = bytearray()
                jtag_execute_cmd(buff, 0xE10F0000)
                jtag_read_reg(buff, 0xE5800000)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
                cpsr = read_int_from_response(response)
                mode_bytes = PROCESSOR_MODE_MASK & cpsr
                if mode_bytes == ABT:
                    mode = "ABT"
                elif mode_bytes == FIQ:
                    mode = "FIQ"
                elif mode_bytes == IRQ:
                    mode = "IRQ"
                elif mode_bytes == SVC:
                    mode = "SVC"
                elif mode_bytes == SYS:
                    mode = "SYS"
                elif mode_bytes == UND:
                    mode = "UND"
                elif mode_bytes == USR:
                    mode = "USR"
                if (cpsr & THUMB_STATE_MASK) == 1:
                    state = "THUMB"
                else:
                    state = "ARM"
                    # restore R0
                buff = bytearray()
                    # MOV R0, #0 opcode: 0xE3A00000
                jtag_execute_cmd(buff, 0xE3A00000)
                    # LDR R0, [R0] opcode: 0xE5900000
                jtag_write_reg(buff, 0xE5900000, Regs[0])
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
                # dump other registers
                jtag_select_scan_chain(buff, SCAN_1)
                # STMIA pc, {r1-r14} opcode: 0xE88F7FFE
                jtag_compose_chain1_0(buff, 0, 0xE88F7FFE, SCAN_1_RW.W)
                # NOP opcode: 0xE1A00000
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.W)
                buff.append(TAP_JtagRTI)
                # TDO is value of R1
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R2
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R3
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R4
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R5
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R6
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R7
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R8
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R9
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R10
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R11
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R12
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R13
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R14
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(1024))
                recvheader = jtag_parse_header(response)
                if recvheader.type == ASD_header.JTAG_TYPE:
                    payload = response[4:]
                    cnt = 0
                    for i in range(1, 15):
                        cmd = payload[cnt]
                        cnt += 1
                        if (cmd >= READ_WRITE_SCAN_MIN and cmd <= READ_WRITE_SCAN_MAX):
                            read_bytes = int((cmd - READ_WRITE_SCAN_MIN + 7) / 8)
                        Regs[i] = int.from_bytes(payload[cnt:cnt + read_bytes], byteorder='little')
                        cnt += read_bytes
                print("PC: %x" %PC)
                print("CPSR: %x <%s mode, %s>" %(cpsr, mode, state))
                for i in range(15):
                    print("R%d: %x" %(i, Regs[i]))
            elif client_cmd[0] == "go":
                jtag_select_scan_chain(buff, SCAN_2)
                # clear DBGRQ
                jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], 0x0, SCAN_2_RW.W)
                jtag_select_scan_chain(buff, SCAN_1)
                # STR R0, [R0] opcode: 0xE5800000
                jtag_read_reg(buff, 0xE5800000)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
                R0 = read_int_from_response(response)
                buff = bytearray()
                # MOV R0, #0 opcode: 0xE3A00000
                jtag_execute_cmd(buff, 0xE3A00000)
                # LDR R0, [R0] opcode: 0xE5900000
                jtag_write_reg(buff, 0xE5900000, PC)
                # MOV PC, R0 opcode: 0xE1A0F000
                jtag_execute_cmd(buff, 0xE1A0F000)
                # MOV R0, #0 opcode: 0xE3A00000
                jtag_execute_cmd(buff, 0xE3A00000)
                    # LDR R0, [R0] opcode: 0xE5900000
                jtag_write_reg(buff, 0xE5900000, R0)
                # NOP opcode: 0xE1A00000
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.W)
                buff.append(TAP_JtagRTI)
                # NOP opcode: 0xE1A00000
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.W)
                # NOP opcode: 0xE1A00000
                jtag_compose_chain1_0(buff, 1, 0xE1A00000, SCAN_1_RW.W)

                jtag_compose_chain1_0(buff, 0, 0xEAFFFFF6, SCAN_1_RW.W)
                # BYPASS
                jtag_compose_ir(buff, BYPASS)
                # RESTART
                jtag_compose_ir(buff, RESTART)
                buff.append(TAP_JtagRTI)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
            elif client_cmd[0] == "regs":
                jtag_select_scan_chain(buff, SCAN_1)
                # STMIA pc, {r0-r14} opcode: 0xE88F7FFE
                jtag_compose_chain1_0(buff, 0, 0xE88F7FFF, SCAN_1_RW.W)
                # NOP opcode: 0xE1A00000
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.W)
                buff.append(TAP_JtagRTI)
                # TDO is value of R0
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R1
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R2
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R3
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R4
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R5
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R6
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R7
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R8
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R9
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R10
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R11
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R12
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R13
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                # TDO is value of R14
                jtag_compose_chain1_0(buff, 0, 0xE1A00000, SCAN_1_RW.RW)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(1024))
                recvheader = jtag_parse_header(response)
                if recvheader.type == ASD_header.JTAG_TYPE:
                    payload = response[4:]
                    cnt = 0
                    for i in range(15):
                        cmd = payload[cnt]
                        cnt += 1
                        if (cmd >= READ_WRITE_SCAN_MIN and cmd <= READ_WRITE_SCAN_MAX):
                            read_bytes = int((cmd - READ_WRITE_SCAN_MIN + 7) / 8)
                        Regs[i] = int.from_bytes(payload[cnt:cnt + read_bytes], byteorder='little')
                        cnt += read_bytes
                print("PC: %x" %PC)
                for i in range(15):
                    print("R%d: %x" %(i, Regs[i]))
            elif client_cmd[0] == "wreg":
                if args != 3:
                    print("Syntex: wreg <RegIndex> <data>")
                    continue
                try:
                    index = int(client_cmd[1], 10)
                    if index < 0 or index > 15:
                        print("Regindex is out of bound")
                    data = int(client_cmd[2], 16)
                    jtag_select_scan_chain(buff, SCAN_1)
                    jtag_write_reg(buff, 0xE5900000 + (index << 12), data)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                except Exception as e:
                    print(str(e))    
            elif client_cmd[0] == "rreg":
                if args != 2:
                    print("Syntex: rreg <RegIndex>")
                    continue
                try:
                    index = int(client_cmd[1], 10)
                    if index < 0 or index > 15:
                        print("Regindex is out of bound")
                    jtag_select_scan_chain(buff, SCAN_1)
                    jtag_read_reg(buff, 0xE5800000 + (index << 12))
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    tmp = read_int_from_response(response)
                    print("tmp %x, R%d: %x" %(tmp, index, Regs[index]))
                except Exception as e:
                    print(str(e))
            elif client_cmd[0] == "mem32":
                if args != 2:
                    print("Syntex: mem32 <Addr>")
                    continue
                try:
                    address = int(client_cmd[1], 16)
                    # select scan chain 1
                    jtag_select_scan_chain(buff, SCAN_1)
                    # STR R0, [R0]  opcode: 0xE5800000
                    jtag_read_reg(buff, 0xE5800000)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    R0 = read_int_from_response(response) # get R0
                    buff = bytearray()
                    # STR R1, [R0]  opcode: 0xE5801000
                    jtag_read_reg(buff, 0xE5801000)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    R1 = read_int_from_response(response) # get R1
                    buff = bytearray()
                    # LDR R0, PC opcode: 0xE59F0000
                    jtag_write_reg(buff, 0xE59F0000, address)
                    # LDR R1, [R0] opcode: 0xE5901000
                    jtag_compose_chain1_0(buff, 0, 0xE5901000, SCAN_1_RW.W)
                    jtag_compose_chain1_0(buff, 1, ARM_NOP, SCAN_1_RW.W)
                    jtag_compose_ir(buff, RESTART)
                    buff.append(TAP_JtagRTI)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    buff = bytearray()
                    jtag_select_scan_chain(buff, SCAN_2)
                    # Rasie DBGRQ
                    jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], DBGCTRLREG.DBGRQ, SCAN_2_RW.W)
                    buff.append(TAP_JtagRTI)
                    # clear DBGRQ
                    jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], 0x0, SCAN_2_RW.W)
                    jtag_select_scan_chain(buff, SCAN_1)
                    # STR R1, [R0]  opcode: 0xE5801000
                    jtag_read_reg(buff, 0xE5801000)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    mem_value = read_int_from_response(response) # get R1
                    print("%x: %x" %(address, mem_value))
                    # restore R0
                    buff = bytearray()
                    # MOV R0, #0 opcode: 0xE3A00000
                    jtag_execute_cmd(buff, 0xE3A00000)
                    # LDR R0, [R0] opcode: 0xE5900000
                    jtag_write_reg(buff, 0xE5900000, R0)
                    # restore R1
                    # MOV R1, #0 opcode: 0xE3A01000
                    jtag_execute_cmd(buff, 0xE3A01000)
                    # LDR R1, [R0] opcode: 0xE5901000
                    jtag_write_reg(buff, 0xE5901000, R1)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                except Exception as e:
                    print(str(e))
            elif client_cmd[0] == "wmem32":
                if args != 3:
                    print("Syntex: wmem32 <Addr> <data>")
                    continue
                try:
                    address = int(client_cmd[1], 16)
                    data = int(client_cmd[2], 16)
                    # select scan chain 1
                    jtag_select_scan_chain(buff, SCAN_1)
                    # STR R0, [R0]  opcode: 0xE5800000
                    jtag_read_reg(buff, 0xE5800000)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    R0 = read_int_from_response(response) # get R0
                    buff = bytearray()
                    # STR R1, [R0]  opcode: 0xE5801000
                    jtag_read_reg(buff, 0xE5801000)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    R1 = read_int_from_response(response) # get R1
                    buff = bytearray()
                    # LDR R0, [PC] opcode: 0xE59F0000
                    jtag_write_reg(buff, 0xE59F0000, address)
                    # LDR R1, [PC] opcode: 0xE59F1000
                    jtag_write_reg(buff, 0xE59F1000, data)
                    # STR R1, [R0] opcode: 0xE5801000
                    jtag_compose_chain1_0(buff, 0, 0xE5801000, SCAN_1_RW.W)
                    jtag_compose_chain1_0(buff, 1, ARM_NOP, SCAN_1_RW.W)
                    jtag_compose_ir(buff, RESTART)
                    buff.append(TAP_JtagRTI)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                    buff = bytearray()
                    jtag_select_scan_chain(buff, SCAN_2)
                    # Rasie DBGRQ
                    jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], DBGCTRLREG.DBGRQ, SCAN_2_RW.W)
                    buff.append(TAP_JtagRTI)
                    # clear DBGRQ
                    jtag_compose_chain2(buff, ARM_ICE_REG[DBGCTRL][ICE_REG_ADDR], 0x0, SCAN_2_RW.W)
                    jtag_select_scan_chain(buff, SCAN_1)
                    # restore R1
                    # MOV R0, #0 opcode: 0xE3A00000
                    jtag_execute_cmd(buff, 0xE3A00000)
                    # LDR R0, [R0] opcode: 0xE5900000
                    jtag_write_reg(buff, 0xE5900000, R0)
                    # restore R1
                    # MOV R1, #0 opcode: 0xE3A01000
                    jtag_execute_cmd(buff, 0xE3A01000)
                    # LDR R1, [R0] opcode: 0xE5901000
                    jtag_write_reg(buff, 0xE5901000, R1)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                except Exception as e:
                    print(str(e))
            elif client_cmd[0] == "setBP":
                if args != 2:
                    print("Syntex: setBP <Addr>")
                    continue
                try:
                    address = int(client_cmd[1], 16)
                    jtag_select_scan_chain(buff, SCAN_2)
                    jtag_compose_chain2(buff, ARM_ICE_REG[WP0CTRLVAL][ICE_REG_ADDR], 0x0, SCAN_2_RW.W)
                    jtag_compose_chain2(buff, ARM_ICE_REG[WP0ADDRVAL][ICE_REG_ADDR], address, SCAN_2_RW.W)
                    jtag_compose_chain2(buff, ARM_ICE_REG[WP0ADDRMSK][ICE_REG_ADDR], 0x3, SCAN_2_RW.W)
                    jtag_compose_chain2(buff, ARM_ICE_REG[WP0DATAMSK][ICE_REG_ADDR], 0xFFFFFFFF, SCAN_2_RW.W)
                    jtag_compose_chain2(buff, ARM_ICE_REG[WP0CTRLMSK][ICE_REG_ADDR], 0xF7, SCAN_2_RW.W)
                    jtag_compose_chain2(buff, ARM_ICE_REG[WP0CTRLVAL][ICE_REG_ADDR], 0x100, SCAN_2_RW.W)
                    jtag_calc_size(header, buff)
                    client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                    response = bytearray(client.recv(128))
                except Exception as e:
                    print(str(e))
            elif client_cmd[0] == "clrBP":
                jtag_select_scan_chain(buff, SCAN_2)
                jtag_compose_chain2(buff, ARM_ICE_REG[WP0CTRLVAL][ICE_REG_ADDR], 0x0, SCAN_2_RW.W)
                jtag_calc_size(header, buff)
                client.sendall(string_at(addressof(header), sizeof(header)) + buff)
                response = bytearray(client.recv(128))
            elif client_cmd[0] == "quit":
                break
            else:
                print("%s is not supported" % client_cmd)
    except socket.error:
        print("Connection refused.")
    finally:
        print("disconnect.")
        client.close()
        sys.exit()