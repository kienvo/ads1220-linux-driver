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
		spi_sync_transfer(ads1220, &tr, 1);// TODO: handle return value
	}
	
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
	const uint8_t *mem;
	if(!src) { // == NULL
		printk("__null__\n");
		return;
	}
	mem = src;

	pr_info("%s | ", prefix);
	while(len-- >0) {
		printk(KERN_CONT"%02X ", *mem++);
	}
}

void ads1220_exit(void) 
{
	spi_unregister_device(ads1220);
}



static void ads1220_reset(void) 
{
	uint8_t cmd = ADS1220_CMD_RESET;

	pr_info("Reset the device\n");
	ads1220_spi_txBuf(&cmd, NULL, 1); // TODO: wrap to 1 byte
	mdelay(2);
}

static void ads1220_sync(void)
{
	uint8_t cmd = ADS1220_CMD_SYNC;

	pr_info("Restart conversions");
	ads1220_spi_txBuf(&cmd, NULL, 1); // TODO: wrap to 1 byte
}

static void ads1220_pwrDown(void)
{
	uint8_t cmd = ADS1220_CMD_SHUTDOWN;

	pr_info("Enter power-down mode\n");
	ads1220_spi_txBuf(&cmd, NULL, 1);
}

static uint8_t ads1220_readReg(uint8_t reg) 
{
	uint8_t cmd = ADS1220_CMD_RREG | (reg<<2);
	uint8_t tx[] = {cmd, 0};
	uint8_t rx[2];

	pr_info("Read register %02X\n", reg);
	ads1220_spi_txBuf( tx, rx, 2);
	return rx[1];
}

// TODO: need full duplex here
static void ads1220_readAllRegs(uint8_t *ret) 
{
	uint8_t cmd = ADS1220_CMD_RREG | 0b11;
	uint8_t tx[] = {cmd, 0, 0, 0, 0};

	pr_info("Read all 4 registers");
	ads1220_spi_txBuf(tx, ret, 5);
}

static void ads1220_writeReg(uint8_t reg, uint8_t val) 
{
	uint8_t cmd = ADS1220_CMD_WREG | (reg << 2);
	uint8_t tx[] = {cmd, val};
	uint8_t rx[2];

	pr_info("Write registers %02X with value %02X", reg, val);
	ads1220_spi_txBuf(tx, rx, 2);
}

static void ads1220_writeAllRegs(uint8_t *regs) 
{
	uint8_t cmd = ADS1220_CMD_WREG | 0b11;
	uint8_t tx[5];

	pr_info("Write all 4 regs with %02X %02X %02X %02X",
		 regs[0], regs[1], regs[2], regs[3]);
	tx[0] = cmd;
	tx[1] = regs[0];
	tx[2] = regs[1];
	tx[3] = regs[2];
	tx[4] = regs[3];

	ads1220_spi_txBuf( tx, NULL, sizeof(tx));	
}

static void ads1220_config(void)
{
	uint8_t config[] = {
		// p = AIN0, n = AVSS, gain = 1, PGA disabled
		ADS1220_MUX_0_G | ADS1220_PGA_BYPASS,   
		ADS1220_DR_1000 |ADS1220_MODE_TURBO | ADS1220_CC , 
		ADS1220_REJECT_50 | ADS1220_MUX_EX_VREF, // 100
		0
	};
	uint8_t regs_check[5];

	ads1220_writeAllRegs(config);
	ads1220_readAllRegs(regs_check);
	if(memcmp(config, &regs_check[1], 4)) 
		pr_err("Fail to config the device, or lose connection!\n"
		"Please recheck the connection.");
}


static int32_t ads1220_humanReadable(uint8_t *data) 
{
	int32_t ret = 0;
	ret |= data[0]<<24;
	ret |= data[1]<<16;
	ret |= data[2]<<8;
	return ret/256;
}

static int32_t ads1220_get1SingleSample(void){
	uint8_t tx[] = {0b00010000, 0, 0, 0};
	uint8_t rx[sizeof(tx)];

	ads1220_spi_txBuf(tx, rx, sizeof(tx));
	// usleep(500);
	return ads1220_humanReadable(&rx[1]);
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

	ads1220_reset();
	ads1220_config();
	ads1220_sync();

	return 0;
}


void ads1220_test(void)
{
	uint8_t test_tx[] = {
		0b00010000, 0, 0, 0
	};
	uint8_t test_rx[5];
	ads1220_spi_txBuf(test_tx, test_rx, 4);
	hex_dump(test_rx, 5, 32, "RX");

	pr_info("Last value: %d", ads1220_get1SingleSample());
}