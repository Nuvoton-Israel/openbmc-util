SUMMARY = "Phosphor VDM test."
DESCRIPTION = "Phosphor VDM test."
PR = "r1"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit autotools pkgconfig
inherit systemd

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus"
DEPENDS += "sdeventplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "systemd"

S = "${WORKDIR}"

SRC_URI += " \
    file://bootstrap.sh \
    file://configure.ac \
    file://Makefile.am \
    file://main.cpp \
    file://vdm_module.h \
    file://vdm_per_test.cpp \
    file://LICENSE \
    "


