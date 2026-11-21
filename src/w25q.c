#include "w25q.h"
#include "sys.h"

#define SPI2_BASE   0x40003800u
#define SPI_CR1     REG16(SPI2_BASE + 0x00u)
#define SPI_SR      REG16(SPI2_BASE + 0x08u)
#define SPI_DR      REG16(SPI2_BASE + 0x0Cu)

#define SPI_CR1_MSTR (1u << 2)
#define SPI_CR1_BR_DIV8 (2u << 3)   /* BR=010 为 fPCLK/8, RM0008 25.5.1; 36 MHz / 8 = 4.5 MHz */
#define SPI_CR1_SPE  (1u << 6)
#define SPI_CR1_SSI  (1u << 8)
#define SPI_CR1_SSM  (1u << 9)
#define SPI_SR_RXNE  (1u << 0)
#define SPI_SR_TXE   (1u << 1)

/*
 * 命令码见 W25Q80DV 8.2 Instruction Set Table 1. 写使能 8.5.1, 读状态 8.5.4,
 * 读数据 8.5.6, 页编程 8.5.13, 扇区擦除 8.5.15, JEDEC ID 8.5.27.
 */
#define CMD_WRITE_ENABLE 0x06u
#define CMD_READ_STATUS1 0x05u
#define CMD_READ_DATA    0x03u
#define CMD_PAGE_PROGRAM 0x02u
#define CMD_SECTOR_ERASE 0x20u
#define CMD_JEDEC_ID     0x9Fu
#define STATUS_BUSY      0x01u

#define WINBOND_MFR      0xEFu   /* 8.1 Manufacturer and Device Identification */
/* 9.6 AC Electrical Characteristics: tPP 最大 3 ms, tSE (4 KB) 最大 300 ms */
#define PAGE_PROGRAM_TIMEOUT_MS 10u
#define SECTOR_ERASE_TIMEOUT_MS 500u

static uint32_t jedec_id;
static int present;

static void cs(int level)
{
	gpio_write(FLASH_CS_PORT, FLASH_CS_PIN, level);
}

static uint8_t xfer(uint8_t out)
{
	while (!(SPI_SR & SPI_SR_TXE))
		;
	SPI_DR = out;
	while (!(SPI_SR & SPI_SR_RXNE))
		;
	return (uint8_t)SPI_DR;
}

static void send_addr(uint32_t addr)
{
	xfer((uint8_t)(addr >> 16));
	xfer((uint8_t)(addr >> 8));
	xfer((uint8_t)addr);
}

/* 轮询状态寄存器的 BUSY 位 (8.5.4). 芯片不存在时 MISO 被上拉, 读到 0xFF, 以超时返回 */
static int wait_ready(uint32_t timeout_ms)
{
	uint32_t start = g_ms;
	uint8_t status;

	do {
		cs(0);
		xfer(CMD_READ_STATUS1);
		status = xfer(0xFF);
		cs(1);
		if (!(status & STATUS_BUSY))
			return 0;
	} while (g_ms - start < timeout_ms);
	return -1;
}

static void write_enable(void)
{
	cs(0);
	xfer(CMD_WRITE_ENABLE);
	cs(1);
}

uint32_t w25q_init(void)
{
	RCC_APB1ENR |= RCC_APB1_SPI2;
	(void)RCC_APB1ENR;

	gpio_config(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_OUT_PP_50M);
	cs(1);
	gpio_config(SPI2_PORT, SPI2_SCK_PIN, GPIO_AF_PP_50M);
	gpio_config(SPI2_PORT, SPI2_MOSI_PIN, GPIO_AF_PP_50M);
	gpio_config_pullup(SPI2_PORT, SPI2_MISO_PIN);

	/* W25Q80DV 支持 SPI 模式 0 和 3 (6.1.1). 这里用模式 0, 软件管理片选 */
	SPI_CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV8 | SPI_CR1_SSM | SPI_CR1_SSI;
	SPI_CR1 |= SPI_CR1_SPE;

	cs(0);
	xfer(CMD_JEDEC_ID);
	jedec_id = (uint32_t)xfer(0xFF) << 16;
	jedec_id |= (uint32_t)xfer(0xFF) << 8;
	jedec_id |= xfer(0xFF);
	cs(1);

	/*
	 * TODO: 清单写的是 6 引脚 8 Mbit 模块, 按 W25Q80 (ID EF 40 14) 处理, 但没有
	 * 见到实物. 只检查厂商字节, 以便换成同厂其他容量时仍可使用.
	 */
	present = (jedec_id >> 16) == WINBOND_MFR;
	return jedec_id;
}

int w25q_present(void) { return present; }

int w25q_read(uint32_t addr, uint8_t *buf, unsigned len)
{
	unsigned i;

	if (!present)
		return -1;
	cs(0);
	xfer(CMD_READ_DATA);
	send_addr(addr);
	for (i = 0; i < len; i++)
		buf[i] = xfer(0xFF);
	cs(1);
	return 0;
}

int w25q_erase_sector(uint32_t addr)
{
	if (!present)
		return -1;
	write_enable();
	cs(0);
	xfer(CMD_SECTOR_ERASE);
	send_addr(addr);
	cs(1);
	return wait_ready(SECTOR_ERASE_TIMEOUT_MS);
}

int w25q_write(uint32_t addr, const uint8_t *buf, unsigned len)
{
	if (!present)
		return -1;
	while (len) {
		unsigned room = W25Q_PAGE_SIZE - (addr % W25Q_PAGE_SIZE);
		unsigned n = len < room ? len : room;
		unsigned i;

		write_enable();
		cs(0);
		xfer(CMD_PAGE_PROGRAM);
		send_addr(addr);
		for (i = 0; i < n; i++)
			xfer(buf[i]);
		cs(1);
		if (wait_ready(PAGE_PROGRAM_TIMEOUT_MS) != 0)
			return -1;
		addr += n;
		buf += n;
		len -= n;
	}
	return 0;
}
