# xfer_mbox_mem

This is host tool to get smbios table from host /sys/firmware/dmi/tables/DMI then copy to mailbox shared memory for BMC to parse smbios binary file and show BIOS/CPU/DIMM informations on WebUI.

## Building

```bash
sudo ./b.sh
```

## Usage

Before using xfer_mbox_mem tool to transfer smbios table to BMC, host side need to build pciutils:
Check out the pciutils source from https://github.com/pciutils/pciutils
Then run these commands in the source directory.

```
make SHARED=yes
make SHARED=yes install
make install-lib
sudo ./setpci -d 1050:0750 04.B=02
```

xfer_mbox_mem command usage:
```
sudo ./xfer_mbox_mem --command update --interface ipmipci --image DMI --sig xfer_mbox_mem
```
