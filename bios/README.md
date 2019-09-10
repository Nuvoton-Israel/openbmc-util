# How to update BIOS FW:
tool/AfuEfi64 contains the efi executable for the UEFI shell and store the executable in a USB flash after unzipping it.
Please also store the firmware/F0B_1B01.07.BIN into the USB flash.

#### The steps to flash the bios and ME:
##### 1) boot into UEFI shell
##### 2) Navigate to the root directory of the USB flash in the UEFI shell, like “fs0:”.
##### 3) Input the command “AfuEfix64.efi F0B_1B01.07.BIN /P /B /X /N /K”
##### 4) After flashing the bios, input the command “AfuEfix64.efi F0B_1B01.07.BIN /ME” to update ME FW.
##### 5) After the update finished, reboot the system, the new BIOS/ME FW runs. It reboots few times.
