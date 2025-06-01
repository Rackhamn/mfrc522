#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mfrc522.h"

int main() {
	stdio_init_all();

	uint8_t default_key_a[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	rfid_init(
		// spi baud
		1 * 1000 * 1000,
		// spi inst *
		spi0,
		// sck, mosi, miso
		18, 19, 16, 
		// cs, rst
		17, 20);

	printf("RFID is initialized\n");

	uint8_t tagtype[MAX_LENGTH] = { 0 };
	uint8_t uid[MAX_LENGTH] = { 0 };
	uint8_t buf[MAX_LENGTH] = { 0 };
	uint8_t uid_len = 0;
	uint8_t block_index = 1;
	
	while(1) {
		memset(tagtype, 0, sizeof(tagtype[0]) * MAX_LENGTH);
		memset(uid, 0, sizeof(uid[0]) * MAX_LENGTH);
		memset(buf, 0, sizeof(buf[0]) * MAX_LENGTH);
		uid_len = 0;

		// REQ_WUPA == 0x52, wakeup halted cards && new card
		// REQ_REQA == 0x26, read only "new" cards
		uint8_t req_mode = REQ_WUPA;
		uint8_t status = rfid_request(req_mode, tagtype);

		if(status != MI_OK) {
			// printf("RFID timeout!\n");
			goto lawait;
		}

		if(tagtype[0] != 0x04 && tagtype[1] != 0x00) {
			printf("RFID tag type not allowed!\n");
			goto lawait;
		}

		status = rfid_anticoll(uid, &uid_len);
		if(status != MI_OK) {
			printf("RFID tag anti collision failed!\n");
			goto lawait;
		}

		// dump general card information
		uint8_t pad = 10;
		printf("%-*s: ", pad, "ATQA");
		for(int i = 0; i < 2; i++) {
			printf("%02X", tagtype[i]);
		}
		printf("\n");

		printf("%-*s: ", pad, "UID");
		for(int i = 0; i < 4; i++) {
			printf("%02X ", uid[i]);
		}
		printf("\n");

		uint8_t sak = rfid_get_sak(tagtype, uid);
		if(sak != 0) {
			printf("%-*s: %02X\n", pad, "SAK", sak);
		}

		printf("%-*s: %02X\n", 10, "Check Byte", uid[4]);

		// authenticate and read card data
		uint8_t * key = default_key_a;
		status = rfid_auth(AUTH_A, 0, key, uid);
		if(status == MI_OK) {
			status = rfid_read_block(block_index, buf);
			printf("block %i: ", block_index);
			for(int i = 0; i < 16; i++) {
				printf("%02X ", buf[i]);
			}
			printf("\n");
			printf("block %i (str): ", block_index);
			for(int i = 0; i < 16; i++) {
				printf("%c", (char)buf[i]);
			}
			printf("\n");

			// make sure that the cards dont hang after auth
			rfid_clear_after_auth();
		} else {
			printf("Auth Failed!\n");
		}

		printf("\n");

lawait:
		sleep_ms(1000);
	}

	return 0;
}
