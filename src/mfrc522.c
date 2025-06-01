#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mfrc522.h"

// mfrc522 library state data
spi_inst_t * _spi;
uint8_t _sck, _mosi, _miso;
uint8_t _cs, _rst;


void write_reg(uint8_t reg, uint8_t val) {
        uint8_t data[2] = { (reg << 1) & 0x7E, val };
	gpio_put(_cs, 0);
        spi_write_blocking(_spi, data, 2);
        gpio_put(_cs, 1);
}

uint8_t read_reg(uint8_t reg) {
        uint8_t addr = ((reg << 1) & 0x7E) | 0x80;
        uint8_t val = 0;

	gpio_put(_cs, 0);
	spi_write_blocking(_spi, &addr, 1);
	spi_read_blocking(_spi, 0x00, &val, 1);
        gpio_put(_cs, 1);

        return val;
}

void set_bit_mask(uint8_t reg, uint8_t mask) {
        write_reg(reg, read_reg(reg) | mask);
}

void clear_bit_mask(uint8_t reg, uint8_t mask) {
        write_reg(reg, read_reg(reg) & (~mask));
}

void antenna_on(void) {
        if(~(read_reg(TxControlReg) & 0x03)) {
                set_bit_mask(TxControlReg, 0x03);
        }
}

void reset(void) {
        write_reg(CommandReg, PCD_RESET_PHASE);
        sleep_ms(50); // ..
}

void rfid_init(uint32_t spi_baud, spi_inst_t * spix, uint8_t sck_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t cs_pin, uint8_t rst_pin) {
/*
        if(spix == 0)
                spi_init(spi0, spi_baud);
        else if(spix == 1)
                spi_init(spi1, spi_baud);
        else {
                printf("rfid_init :: ERROR - spix is not 0 or 1\n");
                return;
        }
*/
        _spi = spix;
        _sck = sck_pin;
        _mosi = mosi_pin;
        _miso = miso_pin;
        _cs = cs_pin;
        _rst = rst_pin;

        spi_init(_spi, spi_baud);

        gpio_set_function(_miso, GPIO_FUNC_SPI);
        gpio_set_function(_mosi, GPIO_FUNC_SPI);
        gpio_set_function(_sck, GPIO_FUNC_SPI);
        gpio_set_function(_cs, GPIO_FUNC_SIO);

        gpio_init(_cs);
        gpio_set_dir(_cs, GPIO_OUT);
        gpio_put(_cs, 1);

        gpio_init(_rst);
        gpio_set_dir(_rst, GPIO_OUT);
        gpio_put(_rst, 1);

        // write_reg(_rst, 1);
        reset();
        write_reg(TModeReg, 0x8D); // 0x2A
        write_reg(TPrescalerReg, 0x3E); // 0x2B
        write_reg(TReloadRegL, 30); // 0x2D
        write_reg(TReloadRegH, 0); // 0x2C
        write_reg(TxAutoReg, 0x40); // 0x15
        write_reg(ModeReg, 0x3D); // 0x11
        antenna_on();
}

uint8_t rfid_transceive(uint8_t *send_data, uint8_t send_len, uint8_t *recv_data, uint8_t *recv_len) {
    uint8_t irq_en = 0x77;   // Enable RX, TX, ERR IRQs
    uint8_t wait_irq = 0x30; // Wait for RX IRQ or Idle IRQ
    uint32_t timeout = 10000; // Timeout counter

    // Enable interrupts and clear pending IRQs
    write_reg(CommIEnReg, irq_en | 0x80);  // Enable IRQ + global IRQ enable - CommIEnReg
    write_reg(CommIrqReg, 0x7F);           // Clear all IRQ flags - CommIrqReg

    // Flush FIFO
    write_reg(FIFOLevelReg, 0x80); // Flush FIFO
    write_reg(CommandReg, PCD_IDLE); // Stop any active command, IDLE

    // Write to FIFO
    for (uint8_t i = 0; i < send_len; i++) {
        write_reg(FIFODataReg, send_data[i]); // FIFODataReg
    }

    // Adjust BitFramingReg (7-bit TX mode if needed)
    write_reg(BitFramingReg, 0x00); // Default: 8-bit per byte

    // Start transmission (Transceive command)
    write_reg(CommandReg, PCD_TRANSCEIVE); // CommandReg, TRANSCEIVE
    set_bit_mask(BitFramingReg, 0x80); // BitFramingReg Start transmission

    // Wait for IRQ
    uint8_t irq_reg = 0;
    while (timeout--) {
        irq_reg = read_reg(CommIrqReg); // CommIrqReg
        if (irq_reg & wait_irq) break;
    }
    if (timeout == 0) {
        printf("rfid_transceive: Timeout waiting for response\n");
        return MI_ERR;
    }

    // Check for errors
    uint8_t error = read_reg(ErrorReg); // ErrorReg
    if (error & 0x1B) { // Check collision, parity, or buffer overflow errors
        printf("rfid_transceive: Error detected (0x%02X)\n", error);
        return MI_ERR;
    }

    // Ensure FIFO has data
    uint8_t attempts = 10;
    while (!(read_reg(FIFOLevelReg)) && attempts--) {
        sleep_ms(1);
    }

    uint8_t fifo_level = read_reg(FIFOLevelReg); // FIFOLevelReg
    if (fifo_level > *recv_len) {
        fifo_level = *recv_len; // Prevent buffer overflow
    }

    // Read FIFO
    for (uint8_t i = 0; i < fifo_level; i++) {
        recv_data[i] = read_reg(FIFODataReg); // FIFODataReg
    }

    *recv_len = fifo_level; // Update received length

    return MI_OK;
}

uint8_t card_command(uint8_t command, uint8_t * send_data, uint8_t send_len, uint8_t * back_data, uint8_t * back_len) {

        uint8_t status = MI_ERR;
        uint8_t irq_en = 0x77;
        uint8_t wait_irq = 0x30;

        switch(command) {
                // auth
                case 0x0E: {
                        irq_en = 0x12;
                        wait_irq = 0x10;
                } break;
                // transceive
                case 0x0C: {
                        irq_en = 0x77;
                        wait_irq = 0x30;
                } break;
        }

        *back_len = 0;

        write_reg(CommIEnReg, irq_en | 0x80);
        clear_bit_mask(CommIrqReg, 0x80);
        set_bit_mask(FIFOLevelReg, 0x80);
        write_reg(CommandReg, PCD_IDLE);

        for(uint8_t i = 0; i < send_len; i++) {
                write_reg(FIFODataReg, send_data[i]);
        }

        write_reg(CommandReg, command);
        if(command == PCD_TRANSCEIVE) {
                set_bit_mask(BitFramingReg, 0x80);
        }

	uint16_t timeout = 2000;
        uint8_t irq_reg = 0;
        do {
                irq_reg = read_reg(CommIrqReg);
                timeout--;
        } while(!(irq_reg & 0x30) && timeout > 0);
        if(timeout == 0) {
                // printf("card_command: read timeout!\n");
                return MI_ERR;
        }

        clear_bit_mask(BitFramingReg, 0x80);

        if(read_reg(ErrorReg) & 0x1B) {
                printf("card_command: error 0x1B\n");
                return MI_ERR;
        }

        if(command == PCD_TRANSCEIVE) {
                uint8_t n = read_reg(FIFOLevelReg);
                if(n >= 16) n = 16;
                *back_len = n;

                for(uint8_t i = 0; i < n; i++) {
                        back_data[i] = read_reg(FIFODataReg);;
                }
        }

        return MI_OK;
}

void rfid_calculate_crc(uint8_t * data, uint8_t len, uint8_t * crc) {
        write_reg(CommandReg, 0x00);
        write_reg(DivIrqReg, 0x04); // clear rcr irq
        write_reg(FIFOLevelReg, 0x80);

        for(uint8_t i = 0; i < len; i++) {
                write_reg(FIFODataReg, data[i]);
        }

        // start crc calc
        write_reg(CommandReg, PCD_CALC_CRC);

        for(uint16_t i = 5000; i > 0; i--) {
                if(read_reg(DivIrqReg) & 0x04) {
                        // crc irq bit is set
                        break;
                }
        }

	// 0x22, 0x21
        crc[0] = read_reg(CRCResultRegL); // crc result reg low
        crc[1] = read_reg(CRCResultRegH); // crc result reg highi

	printf("CRC: %02X %02X\n", crc[0], crc[1]);
}

void rfid_halt(void) {
        uint8_t buf[4] = { PICC_HALT, 0x00 };

        rfid_calculate_crc(buf, 2, &buf[2]);

        uint8_t len;
        uint8_t status = card_command(PCD_TRANSCEIVE, buf, 4, buf, &len);
        /*
        uint8_t response[1];
        uint8_t response_len = 1;
        uint8_t result = rfid_transceive(halt_cmd, 4, response, &response_len);
        if(result == MI_OK && response_len == 0) {
                return MI_OK;
        }

        return MI_ERR;
        */
}

uint8_t rfid_anticoll(uint8_t * data, uint8_t * len) {
        // printf("attempt anticoll\n");
        // BitFramingReg 0x0D
        write_reg(BitFramingReg, 0x00);
        // PICC_ANITCOLL, NVB
        // NVB = number of valid bits, with 0x93, 0x20 : 5-bytes (4-uid + 1 bcc checksum)
        // 0x93 : 4-byte uid, 0x95 : 7-byte uid, 0x97 : 10-byte uid
        uint8_t buf[2] = { PICC_ANTICOLL, 0x20 };
        uint8_t buflen = 2;

        uint8_t back_data[16] = { 0 };
        uint8_t back_data_len = 0;

        uint8_t status = card_command(PCD_TRANSCEIVE, buf, buflen, back_data, &back_data_len);
        // printf("cc : %i, len: %i\n", status, back_data_len);

#if defined(TEST)
        if(back_data_len != 0) {
                printf("anticoll: ");
                for(int i = 0; i < back_data_len; i++) {
                        printf("%02X ", back_data[i]);
                }
                printf("\n");
        }
#endif

        // we only handle 5 bytes (4-byte uid + checkbyte)
        if(status == MI_OK && back_data_len == 5) {
                memcpy(data, back_data, sizeof(back_data[0]) * back_data_len);
                *len = back_data_len;
                return MI_OK;
        }

        // printf("anticoll failed\n");
        return MI_ERR;
}

uint8_t rfid_request(uint8_t mode, uint8_t * tag_type) {
        write_reg(BitFramingReg, 0x07);
        uint8_t status;
        uint8_t back_bits = 0;

        tag_type[0] = mode;
        status = card_command(PCD_TRANSCEIVE, tag_type, 1, tag_type, &back_bits);

#if defined(TEST)
        printf("request status: %i, 0x%02X\n", status, back_bits);
        if(back_bits != 0) {
                printf("request: ");
                for(int i = 0; i < 2; i++) {
                        printf("%02X ", tag_type[i]);
                }
                printf("\n");
        }
#endif

        // discard error
        if((status != MI_OK) || (back_bits != 0x10)) {
                // return MI_ERR;
        }

        return status;
}

uint8_t rfid_get_sak(uint8_t * atqa, uint8_t * uid) {
        // uid[4] == BCC
        // = (uid[0] ^ uid[1] ^ uid[2] ^ uid[3])
        // we should send 9 bytes
        // command + crc
        uint8_t command[9] = { 0x93/*PICC_SELECTTAG*/, 0x70, uid[0], uid[1], uid[2], uid[3], uid[4], 0x00, 0x00 };
        uint8_t sak = 0;
        uint8_t back_data[MAX_LENGTH] = { 0 };
        uint8_t back_data_len = MAX_LENGTH;

        // [ 93 70 D3 2A C6 E7 D8 ] => 0x8D 0x4A        (LSB)

        // compute CRC_A
        rfid_calculate_crc(command, 7, &command[7]);
        // printf("CRC_A : 0x%02X 0x%02X\n", command[7], command[8]);

        uint8_t result = rfid_transceive(command, 9, back_data, &back_data_len);

        /*
        printf("* get_sak\n");
        printf("write[9]: \n");
        for(int i = 0; i < 9; i++) {
                printf("%02X ", command[i]);
        }
        printf("\n");
        printf("status: %i\n", result);
        printf("read[%i]: ", back_data_len);
        for(int i = 0; i < back_data_len; i++) {
                printf("%02X ", back_data[i]);
        }
        printf("\n\n");
	*/
        if(result == MI_OK && back_data_len > 0) {
                return back_data[0];
        }

        return 0;
}

uint8_t rfid_auth(uint8_t picc_auth_mode, uint8_t sector, uint8_t * key, uint8_t * uid) {
        uint8_t status;
        uint8_t recv_bits; // do 32b
        uint8_t i;
        uint8_t buf[12];

        buf[0] = picc_auth_mode;
        buf[1] = sector;

        for(i = 0; i < 6; i++) {
                buf[2 + i] = key[i];
        }

        for(i = 0; i < 4; i++) {
                buf[8 + i] = uid[i];
        }

        // 0x0E = pcd_authent
        status = card_command(PCD_AUTH, buf, 12, buf, &recv_bits);

        // 0x08 = Status2Reg
        if((status != MI_OK) || (!(read_reg(Status2Reg) & 0x08))) {
                status = MI_ERR;
                uint8_t error = read_reg(ErrorReg);
                printf("auth fail - err: 0x%02X\n", error);
        }

        // clear_bit_mask(0x08, 0x08);

        return status;
}

void rfid_clear_after_auth(void) {
	// Status2Reg or PCD_RECEIVE
	clear_bit_mask(0x08, 0x08);
}

uint8_t rfid_write_block(uint8_t block, uint8_t * data) {
        uint8_t status;
        uint8_t recv_bits;
        uint8_t i;
        uint8_t buf[18] = { PICC_WRITE, block }; // select_cmd

        if(block % 4 == 3) {
                printf("Error: Cannot write to sector trailer block %i\n", block);
                return MI_ERR;
        }

        rfid_calculate_crc(buf, 2, &buf[2]);
        status = rfid_transceive(buf, 4, buf, &recv_bits);

        /*
        // 0xA0 rec. = 0x0A
        if((status != MI_OK) || (recv_bits != 4) || ((buf[0] & 0xFF) != 0x0A)) {
                return MI_ERR;
        }
        */
        if(status != MI_OK) {
                printf("write_block: transceive select FAIL\n");
                return MI_ERR;
        }

        for(i = 0; i < 16; i++) {
                buf[i] = data[i];
        }
        rfid_calculate_crc(buf, 16, &buf[16]);
        status = rfid_transceive(buf, 18, buf, &recv_bits);

        /*
        if((status != MI_OK) || (recv_bits != 4) || ((buf[0]) != 0x0A)) {
                printf("write_block: issue writing data\n");
                return MI_ERR;
        }
        */
        if(status != MI_OK) {
                printf("write_block_issue writing data\n");
                return MI_ERR;
        }
        // printf("Block %i written to!\n", block);

        return MI_OK;
}

uint8_t rfid_read_block(uint8_t block, uint8_t * data) {

        uint8_t status;
        uint8_t recv_bits;
        uint8_t i;
        uint8_t buf[18] = { PICC_READ, block }; // select_cmd

        rfid_calculate_crc(buf, 2, &buf[2]);
        status = rfid_transceive(buf, 4, buf, &recv_bits);

        if(status != MI_OK) {
                printf("read_block: transceive select FAIL\n");
                return MI_ERR;
        }

        for(i = 0; i < 16; i++) {
                data[i] = buf[i];
        }
        // printf("Block %i read from!\n", block);

        return MI_OK;
}

// dont use this plz
void rfid_dump_classic_1k(uint8_t * keya, uint8_t * keyb, uint8_t * uid) {
        // dump mifare classic 1k
        uint8_t result;
        uint8_t sector = 0;
        uint8_t block = 0;
        uint8_t buf[16] = { 0 };

        printf("rfid_dump_classic_1k = {\n");
        while(sector < 16) {
                result = rfid_auth(AUTH_A, sector, keya, uid);
                if(result != MI_OK) {
                        result = rfid_auth(AUTH_B, sector, keya, uid);
                        if(result != MI_OK) {
                                block += 4;
                                sector++;
                                continue;
                        }
                }

                uint8_t nblock = block + 4;
                while(block < nblock) {
                        result = rfid_read_block(block, buf);
                        if(result == MI_OK) {
                                printf("\t");
                                for(int i = 0; i < 16; i++) {
                                        printf("%02X ", buf[i]);
                                }
                                printf("\n");
                        } else {
                                printf("\t [ auth error ]\n");
                        }
                        block++;
                }

                sector++;
        }
        printf("}\n");
/*
        printf("DUMP {\n");
        for(int i = 0; i < 16; i++) {
                if(rfid_auth(0x60, i, key, uid) == MI_OK) {
                        if(rfid_read_block(i, buf) == MI_OK) {
                                printf("\t");
                                for(int i = 0; i < 16; i++) {
                                        printf("%02X ", buf[i]);
                                }
                                printf("\n");
                        } else {
                                printf("\t?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??\n");
                        }
                }
        }
        printf("}\n");
*/
}

