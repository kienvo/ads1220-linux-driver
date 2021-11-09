/**
 * @file		main.c
 * @author		github.com/kienvo (kienvo@kienlab.com)
 * @brief 		Linux_spi with ADS1220
 * @version		0.1
 * @date		Oct-30-2021
 * 
 * @copyright	Copyright (c) 2021 by kienvo@kienlab.com
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>

#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "ads1220/ads1220.h"

static int verbose = 1;

static void pabort(const char *s) 
{
	if (errno != 0)
		perror(s);
	else 
		printf("%s\n", s);
	abort();
}

/**
 * @brief 		Dump the hex
 * 
 * @param		src point to memory address to be dump
 * @param		len length of bytes to be dump
 * @param		line_size of console 
 * @param		prefix of dump bytes
 */
static void hex_dump(const void *src, size_t len, size_t line_size, 
	char *prefix) 
// TODO: implement line_size later
{
	if(!src) { // == NULL
		printf("__null__\n");
		return;
	}
	const uint8_t *mem = src;

	printf("%s | ", prefix);
	while(len-- >0) {
		printf("%02X ", *mem++);
	}
	printf("\n");
}


static void dumpstat(int fd)
{
	__u8	lsb, bits;
	__u32	mode, speed;

	if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
		perror("SPI rd_mode");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
		perror("SPI rd_lsb_fist");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
		perror("SPI bits_per_word");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
		perror("SPI max_speed_hz");
		return;
	}

	printf("spi mode 0x%x, %d bits %sper word, %d Hz max\n",
		mode, bits, lsb ? "(lsb first) " : "", speed);
}

static void spi_transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len) 
{
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len 	= len,
		// .delay_usecs
		.speed_hz = 10000
		// .bits_per_word
	};

	int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
	if (verbose) {
		hex_dump(tx, len, 32, "TX");
		hex_dump(rx, len, 32, "RX");
	}
}

// static void ads1220_

static void ads1220_reset(int fd) 
{
	printf("Reset the device\n");
	uint8_t cmd = ADS1220_CMD_RESET;
	spi_transfer(fd, &cmd, NULL, 1);
	usleep(2000);
}

static void ads1220_sync(int fd) 
{
	printf("Start or restart conversions\n");
	uint8_t cmd = ADS1220_CMD_SYNC;
	spi_transfer(fd, &cmd, NULL, 1);
}
static void ads1220_pwrDown(int fd) 
{
	printf("Enter power-down mode\n");
	uint8_t cmd = ADS1220_CMD_SHUTDOWN;
	spi_transfer(fd, &cmd, NULL, 1);
}

static uint8_t ads1220_readReg(int fd, uint8_t reg) 
{
	printf("Read register %02X\n", reg);
	uint8_t cmd = ADS1220_CMD_RREG | (reg<<2);
	uint8_t tx[] = {cmd, 0};
	uint8_t rx[2];
	spi_transfer(fd, tx, rx, 2);
	return rx[1];
}

// TODO: nedd full duplex here
static void ads1220_readAllRegs(int fd, uint8_t *ret) 
{
	printf("Read all 4 registers \n");
	uint8_t cmd = ADS1220_CMD_RREG | 0b11;
	uint8_t tx[] = {cmd, 0, 0, 0, 0};
	spi_transfer(fd, tx, ret, 5);
}
static void ads1220_writeReg(int fd, uint8_t reg, uint8_t val) 
{
	printf("Write registers %02X with value %02X\n", reg, val);
	uint8_t cmd = ADS1220_CMD_WREG | (reg << 2);
	uint8_t tx[] = {cmd, val};
	uint8_t rx[2];
	spi_transfer(fd, tx, rx, 2);
}

static void ads1220_writeAllRegs(int fd, uint8_t *regs) 
{
	printf("Write all 4 regs with %02X %02X %02X %02X\n",
		 reg[0], reg[1], reg[2], reg[3]);
	uint8_t cmd = ADS1220_CMD_WREG | (reg << 2) | 0b11;
	uint8_t tx[] = {cmd, reg[0], reg[1], reg[2], reg[3]};
	spi_transfer(fd, tx, NULL, sizeof(tx));	
}

static void ads1220_init(int fd)
{
	uint8_t regs[5];
	uint8_t test_tx[] = {
		0b00010000, 0, 0, 0
	};
	uint8_t test_rx[5];
	ads1220_reset(fd);

	

	ads1220_writeReg(fd, ADS1220_0_REGISTER, 
		0x43); // 0x43 - 0100 0011
	ads1220_writeReg(fd, ADS1220_1_REGISTER, 0b11000100);
	ads1220_writeReg(fd, ADS1220_2_REGISTER, 0x04);
	ads1220_writeReg(fd, ADS1220_3_REGISTER, 0x10);
	ads1220_readAllRegs(fd, regs);
	ads1220_sync(fd);
}



static void spi_init(int fd) {
	uint8_t mode = SPI_MODE_1;
	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
		perror("Cannot set spi mode");
		return;
	}
	dumpstat(fd);
}

int main(int argc, char **argv)
{
	int fd;
	uint8_t default_tx[] = {0b00010000, 0, 0, 0};
	uint8_t default_rx[sizeof(default_tx)];


	fd = open("/dev/spidev1.0",O_RDWR);
	if (fd < 0)
		pabort("cannot open device");

	spi_init(fd);
	ads1220_init(fd);

	for (int i=0; i<10; i++) {
		spi_transfer(fd, default_tx, default_rx, sizeof(default_tx));
		usleep(50000);
	}
	
	return 0;
}