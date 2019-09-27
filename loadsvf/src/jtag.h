#ifndef __JTAG_H__
#define __JTAG_H__
#include <stdint.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define LEV_DEBUG	1
#define LEV_INFO	2
#define LEV_ERROR	3

typedef unsigned char __u8;
typedef unsigned long __u32;
typedef unsigned long long __u64;

typedef enum {
	JtagTLR,
	JtagRTI,
	JtagSelDR,
	JtagCapDR,
	JtagShfDR,
	JtagEx1DR,
	JtagPauDR,
	JtagEx2DR,
	JtagUpdDR,
	JtagSelIR,
	JtagCapIR,
	JtagShfIR,
	JtagEx1IR,
	JtagPauIR,
	JtagEx2IR,
	JtagUpdIR,
	JTAG_STATE_CURRENT
} JtagStates;

typedef enum tap_state {
    TAP_INVALID = -1,
    /* Proper ARM recommended numbers */
    TAP_DREXIT2 = JtagEx2DR,
    TAP_DREXIT1 = JtagEx1DR,
    TAP_DRSHIFT = JtagShfDR,
    TAP_DRPAUSE = JtagPauDR,
    TAP_IRSELECT = JtagSelIR,
    TAP_DRUPDATE = JtagUpdDR,
    TAP_DRCAPTURE = JtagCapDR,
    TAP_DRSELECT = JtagSelDR,
    TAP_IREXIT2 = JtagEx2IR,
    TAP_IREXIT1 = JtagEx1IR,
    TAP_IRSHIFT = JtagShfIR,
    TAP_IRPAUSE = JtagPauIR,
    TAP_IDLE = JtagRTI,
    TAP_IRUPDATE = JtagUpdIR,
    TAP_IRCAPTURE = JtagCapIR,
    TAP_RESET = JtagTLR,
} tap_state_t;


typedef enum {
	ST_OK,
	ST_ERR
} STATUS;

typedef enum {
    JTAGDriverState_Master = 0,
    JTAGDriverState_Slave
} JTAGDriverState;

typedef struct JTAG_Handler {
    JtagStates tap_state;
    int JTAG_driver_handle;
} JTAG_Handler;

#define TDI_DATA_SIZE	    256
#define TDO_DATA_SIZE	    256
#define JTAG_MAX_XFER_DATA_LEN 65535
#define MAX_DATA_SIZE 3000
struct scan_xfer {
	unsigned int     length;      // number of bits to clock
	unsigned char    tdi[TDI_DATA_SIZE];        // data to write to tap (optional)
	unsigned int     tdi_bytes;
	unsigned char    tdo[TDO_DATA_SIZE];        // data to read from tap (optional)
	unsigned int     tdo_bytes;
	unsigned int     end_tap_state;
};
struct jtag_xfer {
	__u8	type;
	__u8	direction;
	__u8	from;
	__u8	endstate;
	__u8	padding;
	__u32	length;
	__u64	tdio;
};

struct jtag_tap_state {
	__u8	reset;
	__u8	from;
	__u8	endstate;
	__u8	tck;
};

enum jtag_xfer_type {
	JTAG_SIR_XFER = 0,
	JTAG_SDR_XFER = 1,
};

enum jtag_xfer_direction {
	JTAG_READ_XFER = 1,
	JTAG_WRITE_XFER = 2,
	JTAG_READ_WRITE_XFER = 3,
};
struct scan_field {
    /** The number of bits this field specifies */
    int num_bits;
    /** A pointer to value to be scanned into the device */
    const uint8_t *out_value;
    /** A pointer to a 32-bit memory location for data scanned out */
    uint8_t *in_value;

    /** The value used to check the data scanned out. */
    uint8_t *check_value;
    /** The mask to go with check_value */
    uint8_t *check_mask;
};

/* legacy ioctl interface */
#define JTAGIOC_BASE    'T'
#define JTAG_SIOCFREQ_OLD     _IOW( JTAGIOC_BASE, 3, unsigned int)
#define JTAG_GIOCFREQ_OLD     _IOR( JTAGIOC_BASE, 4, unsigned int)
#define JTAG_BITBANG          _IOWR(JTAGIOC_BASE, 5, struct tck_bitbang)
#define JTAG_SET_TAPSTATE     _IOW( JTAGIOC_BASE, 6, unsigned int)
#define JTAG_READWRITESCAN    _IOWR(JTAGIOC_BASE, 7, struct scan_xfer)
#define JTAG_SLAVECONTLR      _IOW( JTAGIOC_BASE, 8, unsigned int)
#define JTAG_RUNTEST          _IOW( JTAGIOC_BASE, 9, unsigned int)
#define JTAG_DIRECTGPIO       _IOW( JTAGIOC_BASE, 10, unsigned int)
#define JTAG_PSPI             _IOW( JTAGIOC_BASE, 11, unsigned int)
#define JTAG_PSPI_IRQ         _IOW( JTAGIOC_BASE, 12, unsigned int)

/* ioctl interface for ASD */
#define __JTAG_IOCTL_MAGIC	0xb2
#define JTAG_SIOCSTATE	_IOW(__JTAG_IOCTL_MAGIC, 0, struct jtag_tap_state)
#define JTAG_SIOCFREQ	_IOW(__JTAG_IOCTL_MAGIC, 1, unsigned int)
#define JTAG_GIOCFREQ	_IOR(__JTAG_IOCTL_MAGIC, 2, unsigned int)
#define JTAG_IOCXFER	_IOWR(__JTAG_IOCTL_MAGIC, 3, struct jtag_xfer)
#define JTAG_GIOCSTATUS _IOWR(__JTAG_IOCTL_MAGIC, 4, enum JtagStates)
#define JTAG_SIOCMODE	_IOW(__JTAG_IOCTL_MAGIC, 5, unsigned int)
#define JTAG_IOCBITBANG	_IOW(__JTAG_IOCTL_MAGIC, 6, unsigned int)

const char *tap_state_name(tap_state_t state);
tap_state_t tap_state_by_name(const char *name);
STATUS JTAG_set_pspi(int handle, unsigned int enable);
STATUS JTAG_set_pspi_irq(int handle, unsigned int enable);
STATUS JTAG_set_cntlr_mode(int handle, const JTAGDriverState setMode);
STATUS JTAG_set_directgpio(int handle, unsigned int enable);
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state);
STATUS JTAG_set_clock_frequency(int handle, unsigned int frequency);
STATUS JTAG_wait_cycles(JTAG_Handler* state, unsigned int number_of_cycles);
int JTAG_ir_scan(JTAG_Handler* handler, int num_bits, const uint8_t *out_bits, uint8_t *in_bits,
	tap_state_t state);
int JTAG_dr_scan(JTAG_Handler* handler, int num_bits, const uint8_t *out_bits, uint8_t *in_bits,
	tap_state_t state);


#endif
