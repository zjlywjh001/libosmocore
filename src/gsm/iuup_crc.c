#include <stdint.h>
#include <stdio.h>

#include <osmocom/core/crc8gen.h>
#include <osmocom/core/crc16gen.h>

const struct osmo_crc8gen_code iuup_hdr_crc_code = {
	.bits = 6,
	.poly = 47,
	.init = 0,
	.remainder = 0,
};

const struct osmo_crc16gen_code iuup_data_crc_code = {
	.bits = 10,
	.poly = 563,
	.init = 0,
	.remainder = 0,
};

/* Frame 29 of MobileOriginatingCall_AMR.cap */
const uint8_t iuup_frame29[] = {
	0x02, 0x00, 0x7d, 0x27, 0x55, 0x00, 0x88, 0xb6, 0x66, 0x79, 0xe1, 0xe0,
	0x01, 0xe7, 0xcf, 0xf0, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


int main(int argc, char **argv)
{
	ubit_t buf[1024*8];
	int rc;

	osmo_pbit2ubit(buf, iuup_frame29, 2*8);
	rc = osmo_crc8gen_compute_bits(&iuup_hdr_crc_code, buf, 2*8);
	printf("Header CRC = 0x%02x\n", rc);

	osmo_pbit2ubit(buf, iuup_frame29+4, 31*8);
	rc = osmo_crc16gen_compute_bits(&iuup_data_crc_code, buf, 31*8);
	printf("Payload CRC = 0x%03x\n", rc);
}
