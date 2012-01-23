/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "crc.h"

static unsigned short ccitt_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

unsigned long crc32_table[256] = {
	0x00000000L, 0x90910101L, 0x91210201L, 0x01B00300L,
	0x92410401L, 0x02D00500L, 0x03600600L, 0x93F10701L,
	0x94810801L, 0x04100900L, 0x05A00A00L, 0x95310B01L,
	0x06C00C00L, 0x96510D01L, 0x97E10E01L, 0x07700F00L,
	0x99011001L, 0x09901100L, 0x08201200L, 0x98B11301L,
	0x0B401400L, 0x9BD11501L, 0x9A611601L, 0x0AF01700L,
	0x0D801800L, 0x9D111901L, 0x9CA11A01L, 0x0C301B00L,
	0x9FC11C01L, 0x0F501D00L, 0x0EE01E00L, 0x9E711F01L,
	0x82012001L, 0x12902100L, 0x13202200L, 0x83B12301L,
	0x10402400L, 0x80D12501L, 0x81612601L, 0x11F02700L,
	0x16802800L, 0x86112901L, 0x87A12A01L, 0x17302B00L,
	0x84C12C01L, 0x14502D00L, 0x15E02E00L, 0x85712F01L,
	0x1B003000L, 0x8B913101L, 0x8A213201L, 0x1AB03300L,
	0x89413401L, 0x19D03500L, 0x18603600L, 0x88F13701L,
	0x8F813801L, 0x1F103900L, 0x1EA03A00L, 0x8E313B01L,
	0x1DC03C00L, 0x8D513D01L, 0x8CE13E01L, 0x1C703F00L,
	0xB4014001L, 0x24904100L, 0x25204200L, 0xB5B14301L,
	0x26404400L, 0xB6D14501L, 0xB7614601L, 0x27F04700L,
	0x20804800L, 0xB0114901L, 0xB1A14A01L, 0x21304B00L,
	0xB2C14C01L, 0x22504D00L, 0x23E04E00L, 0xB3714F01L,
	0x2D005000L, 0xBD915101L, 0xBC215201L, 0x2CB05300L,
	0xBF415401L, 0x2FD05500L, 0x2E605600L, 0xBEF15701L,
	0xB9815801L, 0x29105900L, 0x28A05A00L, 0xB8315B01L,
	0x2BC05C00L, 0xBB515D01L, 0xBAE15E01L, 0x2A705F00L,
	0x36006000L, 0xA6916101L, 0xA7216201L, 0x37B06300L,
	0xA4416401L, 0x34D06500L, 0x35606600L, 0xA5F16701L,
	0xA2816801L, 0x32106900L, 0x33A06A00L, 0xA3316B01L,
	0x30C06C00L, 0xA0516D01L, 0xA1E16E01L, 0x31706F00L,
	0xAF017001L, 0x3F907100L, 0x3E207200L, 0xAEB17301L,
	0x3D407400L, 0xADD17501L, 0xAC617601L, 0x3CF07700L,
	0x3B807800L, 0xAB117901L, 0xAAA17A01L, 0x3A307B00L,
	0xA9C17C01L, 0x39507D00L, 0x38E07E00L, 0xA8717F01L,
	0xD8018001L, 0x48908100L, 0x49208200L, 0xD9B18301L,
	0x4A408400L, 0xDAD18501L, 0xDB618601L, 0x4BF08700L,
	0x4C808800L, 0xDC118901L, 0xDDA18A01L, 0x4D308B00L,
	0xDEC18C01L, 0x4E508D00L, 0x4FE08E00L, 0xDF718F01L,
	0x41009000L, 0xD1919101L, 0xD0219201L, 0x40B09300L,
	0xD3419401L, 0x43D09500L, 0x42609600L, 0xD2F19701L,
	0xD5819801L, 0x45109900L, 0x44A09A00L, 0xD4319B01L,
	0x47C09C00L, 0xD7519D01L, 0xD6E19E01L, 0x46709F00L,
	0x5A00A000L, 0xCA91A101L, 0xCB21A201L, 0x5BB0A300L,
	0xC841A401L, 0x58D0A500L, 0x5960A600L, 0xC9F1A701L,
	0xCE81A801L, 0x5E10A900L, 0x5FA0AA00L, 0xCF31AB01L,
	0x5CC0AC00L, 0xCC51AD01L, 0xCDE1AE01L, 0x5D70AF00L,
	0xC301B001L, 0x5390B100L, 0x5220B200L, 0xC2B1B301L,
	0x5140B400L, 0xC1D1B501L, 0xC061B601L, 0x50F0B700L,
	0x5780B800L, 0xC711B901L, 0xC6A1BA01L, 0x5630BB00L,
	0xC5C1BC01L, 0x5550BD00L, 0x54E0BE00L, 0xC471BF01L,
	0x6C00C000L, 0xFC91C101L, 0xFD21C201L, 0x6DB0C300L,
	0xFE41C401L, 0x6ED0C500L, 0x6F60C600L, 0xFFF1C701L,
	0xF881C801L, 0x6810C900L, 0x69A0CA00L, 0xF931CB01L,
	0x6AC0CC00L, 0xFA51CD01L, 0xFBE1CE01L, 0x6B70CF00L,
	0xF501D001L, 0x6590D100L, 0x6420D200L, 0xF4B1D301L,
	0x6740D400L, 0xF7D1D501L, 0xF661D601L, 0x66F0D700L,
	0x6180D800L, 0xF111D901L, 0xF0A1DA01L, 0x6030DB00L,
	0xF3C1DC01L, 0x6350DD00L, 0x62E0DE00L, 0xF271DF01L,
	0xEE01E001L, 0x7E90E100L, 0x7F20E200L, 0xEFB1E301L,
	0x7C40E400L, 0xECD1E501L, 0xED61E601L, 0x7DF0E700L,
	0x7A80E800L, 0xEA11E901L, 0xEBA1EA01L, 0x7B30EB00L,
	0xE8C1EC01L, 0x7850ED00L, 0x79E0EE00L, 0xE971EF01L,
	0x7700F000L, 0xE791F101L, 0xE621F201L, 0x76B0F300L,
	0xE541F401L, 0x75D0F500L, 0x7460F600L, 0xE4F1F701L,
	0xE381F801L, 0x7310F900L, 0x72A0FA00L, 0xE231FB01L,
	0x71C0FC00L, 0xE151FD01L, 0xE0E1FE01L, 0x7070FF00L
};


/* Exploration ts B00214 :
   ECMA-130, 22.3.6 "CRC field"
   Generating polynomial: x^16 + x^12 + x^5 + 1
   Also known as CRC-16-CCITT, CRC-CCITT 

   Use in libburn for raw write modes in sector.c.
   There is also disabled code in read.c which would use it.

   ts B11222:
   libburn/cdtext.c uses a simple bit shifting function : crc_11021()
*/
unsigned short crc_ccitt(unsigned char *q, int len)
{
	unsigned short crc = 0;

	while (len-- > 0)
		crc = ccitt_table[(crc >> 8 ^ *q++) & 0xff] ^ (crc << 8);
	return ~crc;
}


/* Exploration ts B00214 :
   ECMA-130, 14.3 "EDC field"
   "The EDC codeword must be divisible by the check polynomial:
    P(x) = (x^16 + x^15 + x^2 + 1) . (x^16 + x^2 + x + 1)   
   "

   >>> Test whether this coincides with CRC-32 IEEE 802.3
   x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10
   + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1

   Used for raw writing in sector.c
*/
unsigned int crc_32(unsigned char *data, int len)
{
	unsigned int crc = 0;

	while (len-- > 0)
		crc = crc32_table[(crc ^ *data++) & 0xffL] ^ (crc >> 8);
	return crc;
}
