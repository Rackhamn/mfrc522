#ifndef MFRC522_H
#define MFRC522_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_LENGTH	16

#define MI_OK		0
#define MI_NOTAGERR	1
#define MI_ERR		2

// TODO: add defines for all hex values in .c

// mf522 specific commands
#define PCD_IDLE	0x00
#define PCD_AUTH	0x0E
#define PCD_RECEIVE	0x08
#define PCD_TRANSMIT	0x04
#define PCD_TRANSCEIVE	0x0C
#define PCD_RESET_PHASE	0x0F
#define PCD_CALC_CRC	0x03

// mifare specific commands
#define PICC_REQIDL	0x26
#define PICC_REQALL	0x52
#define PICC_ANTICOLL	0x93
#define PICC_SELECTTAG	0x93
#define PICC_AUTH1A	0x60
#define PICC_AUTH1B	0x61
#define PICC_READ	0x30
#define PICC_WRITE	0xA0
#define PICC_DECREMENT	0xC0
#define PICC_INCREMENT	0xC1
#define PICC_RESTORE	0xC2
#define PICC_TRANSFER	0xB0
#define PICC_HALT	0x50

// MFRC522 registers
#define Reserved	0x00
#define CommandReg	0x01
#define CommIEnReg	0x02
#define DivIEnReg	0x03
#define CommIrqReg	0x04
#define DivIrqReg	0x05
#define ErrorReg	0x06
#define Status1Reg	0x07
#define Status2Reg	0x08
#define FIFODataReg	0x09
#define FIFOLevelReg	0x0A
#define WaterLevelReg	0x0B
#define ControlReg	0x0C
#define BitFramingReg	0x0D
#define CollReg		0x0E

#define TxControlReg	0x14

#define ModeReg		0x11
#define TxModeReg	0x12
#define RxModeReg	0x13
#define RxControlReg	0x14
#define TxAutoReg	0x15
#define TxSelReg	0x16
#define RxSelReg	0x17
#define RxThresholdReg	0x18
#define DemodReg	0x19
#define MifareReg	0x1C	// what does this one do?
#define SerialSpeedReg	0x1F

#define TModeReg	0x2A
#define TPrescalerReg	0x2B
#define TReloadRegL	0x2D
#define TReloadRegH	0x2C
#define TxAutoReg	0x15

#define CRCResultRegH	0x21
#define CRCResultRegL	0x22
#define RFCfgReg	0x26

#define AUTH_A		PICC_AUTH1A
#define AUTH_B		PICC_AUTH1B
#define REQ_WUPA	0x52
#define REQ_REQA	0x26


/*
// mfrc522 library state data
spi_inst_t * _spi;
uint8_t _sck, _mosi, _miso;
uint8_t _cs, _rst;
*/

// TODO: convert to rfid_xxx or move wreg/rreg & bitmask to .c
void write_reg(uint8_t reg, uint8_t val);
uint8_t read_reg(uint8_t reg);

void set_bit_mask(uint8_t reg, uint8_t mask);
void clear_bit_mask(uint8_t reg, uint8_t mask);

void antenna_on(void);
void reset(void);

void rfid_init(uint32_t spi_baud, spi_inst_t * spix, uint8_t sck_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t cs_pin, uint8_t rst_pin);

uint8_t rfid_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *recv_data, uint8_t *recv_len);

uint8_t card_command(uint8_t command, uint8_t * send_data, uint8_t send_len, uint8_t * back_data, uint8_t * back_len);

void rfid_calculate_crc(uint8_t * data, uint8_t len, uint8_t * crc);

void rfid_halt(void);

uint8_t rfid_anticoll(uint8_t * data, uint8_t * len);

uint8_t rfid_request(uint8_t mode, uint8_t * tag_type);

/*
0x08    MIFARE Classic 1K/4K
0x09    MIFARE Mini (320 bytes)
0x18    MIFARE DESFire
0x20    NFC Forum Type 2
0x28    NFC Forum Type 4
*/
uint8_t rfid_get_sak(uint8_t * atqa, uint8_t * uid);

uint8_t rfid_auth(uint8_t picc_auth_mode, uint8_t sector, uint8_t * key, uint8_t * uid);
void rfid_clear_after_auth(void);

uint8_t rfid_write_block(uint8_t block, uint8_t * data);
uint8_t rfid_read_block(uint8_t block, uint8_t * data);

// dont use this plz
void rfid_dump_classic_1k(uint8_t * keya, uint8_t * keyb, uint8_t * uid);

#endif /* MFRC522_H */
