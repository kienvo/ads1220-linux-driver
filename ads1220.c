/**
 * @file		ads1220.c
 * @author		github.com/kienvo (kienvo@kienlab.com)
 * @brief 		This file contain functions to interact with ads1220
 * @version		0.1
 * @date		Oct-30-2021
 * 
 * @copyright	Copyright (c) 2021 by kienvo@kienlab.com
 * 
 */



static int verbose = 1;

#include "ads1220.h"


static struct spi_device *ads1220;

struct spi_board_info ads1220_info =
{
	.modalias	= "ads1220spi",
	.max_speed_hz = 20000,
	.bus_num	= 1,
	.chip_select = 1,
	.mode		= SPI_MODE_1
};

static int ads1220_spi_txByte(uint8_t data)
{
	int ret;
	uint8_t rx = 0x00;

	if(ads1220) {
		struct spi_transfer tr = {
			.tx_buf = &data,
			.rx_buf = &rx,
			.len	= 1,
		};
		ret = spi_sync_transfer(ads1220, &tr, 1);
	}
	pr_info("ads1220_spi_txByte: rx: 0x%02X\n", rx);
	return ret;
}


static void ads1220_spi_txBuf(uint8_t const *tx, uint8_t const *rx, size_t len) 
{

	if(ads1220) {
		struct spi_transfer tr = {
			.tx_buf = (void *)tx,
			.rx_buf = (void *)rx,
			.len	= len,
		};
		spi_sync_transfer(ads1220, &tr, 1);// TODO: return handle
	}
	
}

static void hex_dump(const void *src, size_t len, size_t line_size, 
	char *prefix) 
// TODO: implement line_size later
{
	const uint8_t *mem;
	if(!src) { // == NULL
		printk("__null__\n");
		return;
	}
	mem = src;

	printk("%s | ", prefix);
	while(len-- >0) {
		printk("%02X ", *mem++);
	}
	printk("\n");
}

void test(void)
{
	uint8_t test_tx[] = {
		0b00010000, 0, 0, 0
	};
	uint8_t test_rx[5];
	ads1220_spi_txBuf(test_tx, test_rx, 4);
	hex_dump(test_rx, 5, 32, "RX");
}

int ads1220_init(void)
{
	struct spi_master *master;
	master = spi_busnum_to_master(ads1220_info.bus_num);
	if(!master) {
		pr_err("SPI Master not found.\n");
		return -ENODEV;
	}
	ads1220 = spi_new_device(master, &ads1220_info);
	if(!ads1220) {
		pr_err("Failed to create slave.\n");
		return -ENODEV;
	}
	ads1220->bits_per_word = 8;
	if(spi_setup(ads1220)) {
		pr_err("Failed to setup slave.\n");
		spi_unregister_device(ads1220);
		return -ENODEV;
	}
	return 0;
}

void ads1220_exit(void) 
{
	spi_unregister_device(ads1220);
}


// static void pabort(const char *s) 
// {
// 	if (errno != 0)
// 		perror(s);
// 	else 
// 		printf("%s\n", s);
// 	abort();
// }

// /**
//  * @brief 		Dump the hex
//  * 
//  * @param		src point to memory address to be dump
//  * @param		len length of bytes to be dump
//  * @param		line_size of console 
//  * @param		prefix of dump bytes
//  */
// static void hex_dump(const void *src, size_t len, size_t line_size, 
// 	char *prefix) 
// // TODO: implement line_size later
// {
// 	if(!src) { // == NULL
// 		printf("__null__\n");
// 		return;
// 	}
// 	const uint8_t *mem = src;

// 	printf("%s | ", prefix);
// 	while(len-- >0) {
// 		printf("%02X ", *mem++);
// 	}
// 	printf("\n");
// }


// static void dumpstat(int fd)
// {
// 	__u8	lsb, bits;
// 	__u32	mode, speed;

// 	if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
// 		perror("SPI rd_mode");
// 		return;
// 	}
// 	if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
// 		perror("SPI rd_lsb_fist");
// 		return;
// 	}
// 	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
// 		perror("SPI bits_per_word");
// 		return;
// 	}
// 	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
// 		perror("SPI max_speed_hz");
// 		return;
// 	}

// 	printf("spi mode 0x%x, %d bits %sper word, %d Hz max\n",
// 		mode, bits, lsb ? "(lsb first) " : "", speed);
// }

// static void spi_transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len) 
// {
// 	struct spi_ioc_transfer tr = {
// 		.tx_buf = (unsigned long)tx,
// 		.rx_buf = (unsigned long)rx,
// 		.len 	= len,
// 		// .delay_usecs
// 		.speed_hz = 10000
// 		// .bits_per_word
// 	};

// 	int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
// 	if (ret < 1)
// 		pabort("can't send spi message");
// 	if (verbose) {
// 		hex_dump(tx, len, 32, "TX");
// 		hex_dump(rx, len, 32, "RX");
// 	}
// }

// // static void ads1220_

// static void ads1220_reset(int fd) 
// {
// 	printf("Reset the device\n");
// 	uint8_t cmd = ADS1220_CMD_RESET;
// 	spi_transfer(fd, &cmd, NULL, 1);
// 	usleep(2000);
// }

// static void ads1220_sync(int fd) 
// {
// 	printf("Start or restart conversions\n");
// 	uint8_t cmd = ADS1220_CMD_SYNC;
// 	spi_transfer(fd, &cmd, NULL, 1);
// }

// static void ads1220_pwrDown(int fd) 
// {
// 	printf("Enter power-down mode\n");
// 	uint8_t cmd = ADS1220_CMD_SHUTDOWN;
// 	spi_transfer(fd, &cmd, NULL, 1);
// }

// static uint8_t ads1220_readReg(int fd, uint8_t reg) 
// {
// 	printf("Read register %02X\n", reg);
// 	uint8_t cmd = ADS1220_CMD_RREG | (reg<<2);
// 	uint8_t tx[] = {cmd, 0};
// 	uint8_t rx[2];
// 	spi_transfer(fd, tx, rx, 2);
// 	return rx[1];
// }

// // TODO: need full duplex here
// static void ads1220_readAllRegs(int fd, uint8_t *ret) 
// {
// 	printf("Read all 4 registers \n");
// 	uint8_t cmd = ADS1220_CMD_RREG | 0b11;
// 	uint8_t tx[] = {cmd, 0, 0, 0, 0};
// 	spi_transfer(fd, tx, ret, 5);
// }

// static void ads1220_writeReg(int fd, uint8_t reg, uint8_t val) 
// {
// 	printf("Write registers %02X with value %02X\n", reg, val);
// 	uint8_t cmd = ADS1220_CMD_WREG | (reg << 2);
// 	uint8_t tx[] = {cmd, val};
// 	uint8_t rx[2];
// 	spi_transfer(fd, tx, rx, 2);
// }

// static void ads1220_writeAllRegs(int fd, uint8_t *regs) 
// {
// 	printf("Write all 4 regs with %02X %02X %02X %02X\n",
// 		 regs[0], regs[1], regs[2], regs[3]);
// 	uint8_t cmd = ADS1220_CMD_WREG | 0b11;
// 	uint8_t tx[] = {cmd, regs[0], regs[1], regs[2], regs[3]};
// 	spi_transfer(fd, tx, NULL, sizeof(tx));	
// }

// static void ads1220_config(int fd) 
// {
// 	uint8_t config[] = {
// 		// p = AIN0, n = AVSS, gain = 1, PGA disabled
// 		ADS1220_MUX_0_G | ADS1220_PGA_BYPASS,   
// 		ADS1220_DR_1000 |ADS1220_MODE_TURBO | ADS1220_CC , 
// 		ADS1220_REJECT_50 | ADS1220_MUX_EX_VREF, // 100
// 		0
// 	};
// 	uint8_t regs_check[5];

// 	ads1220_writeAllRegs(fd, config);
// 	ads1220_readAllRegs(fd, regs_check);
// 	if(memcmp(config, &regs_check[1], 4)) 
// 		pabort("Fail to config the device, or lost connect!\n"
// 		"Please recheck the connection.");
// }


// static int32_t ads1220_humanReadable(uint8_t *data) 
// {
// 	int32_t ret = 0;
// 	ret |= data[0]<<24;
// 	ret |= data[1]<<16;
// 	ret |= data[2]<<8;
// 	return ret/256;
// }

// static int32_t ads1220_get1SingleSample(int fd) {
// 	uint8_t tx[] = {0b00010000, 0, 0, 0};
// 	uint8_t rx[sizeof(tx)];
// 	spi_transfer(fd, tx, rx, sizeof(tx));
// 	// usleep(500);
// 	return ads1220_humanReadable(&rx[1]);
// }

// static int32_t ads1220_getSample(int fd, uint32_t us, FILE *toFile) 
// {
// 	verbose = 0; // turn off verbose
// 	for(int i=0; i < us/500; i++) {
// 		fprintf(toFile, "%i\n", ads1220_get1SingleSample(fd));
// 		usleep(500); // 2000 sps
// 	}
// 	verbose = 1; // turn on verbose
// }

// static void ads1220_init(int fd)
// {
// 	ads1220_reset(fd);
// 	ads1220_config(fd);
// 	ads1220_sync(fd);
// }

// static void spi_init(int fd) 
// {
// 	uint8_t mode = SPI_MODE_1;
// 	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
// 		pabort("Cannot set spi mode");
// 		return;
// 	}
// 	dumpstat(fd);
// }

// static void gpio_init() 
// {
// 	if(gpio_is_valid())
// }
