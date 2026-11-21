#include "i2c.h"
#include "sys.h"

/*
 * 软件 I2C. 引脚为开漏输出, 写 1 即释放总线, 由外部上拉拉高 (RM0008 9.1.8,
 * Table 20). 没有处理时钟延展, SSD1306 和 MPU6500 不会拉住 SCL.
 */
#define I2C_TARGET_HZ   400000u   /* 快速模式上限, UM10204 Table 11 */
#define I2C_LOOP_CYCLES 5u        /* 空循环每次约 5 个周期, 估计值 */
#define RECOVER_CLOCKS  9u

static uint32_t half_period_loops = 1;

static void delay_half(void)
{
	volatile uint32_t i;

	for (i = half_period_loops; i; i--)
		;
}

static void scl(int level) { gpio_write(I2C_PORT, I2C_SCL_PIN, level); }
static void sda(int level) { gpio_write(I2C_PORT, I2C_SDA_PIN, level); }
static int sda_read(void)  { return gpio_read(I2C_PORT, I2C_SDA_PIN); }

void i2c_init(void)
{
	unsigned i;

	/* TODO: SCL 频率按循环周期数估算, 还没有用逻辑分析仪或示波器测过 */
	half_period_loops = g_sysclk_hz / (2u * I2C_TARGET_HZ * I2C_LOOP_CYCLES);
	if (half_period_loops == 0)
		half_period_loops = 1;

	gpio_config(I2C_PORT, I2C_SCL_PIN, GPIO_OUT_OD_50M);
	gpio_config(I2C_PORT, I2C_SDA_PIN, GPIO_OUT_OD_50M);
	scl(1);
	sda(1);

	/*
	 * 单片机在从机传输中途复位时, 从机可能一直拉低 SDA. 按 UM10204 3.1.16
	 * (Bus clear) 发最多 9 个时钟让它释放, 再发 STOP.
	 */
	for (i = 0; i < RECOVER_CLOCKS && !sda_read(); i++) {
		scl(0);
		delay_half();
		scl(1);
		delay_half();
	}
	i2c_stop();
}

/* START: SCL 为高时 SDA 由高变低. STOP: SCL 为高时 SDA 由低变高, UM10204 3.1.4 */
void i2c_start(void)
{
	sda(1);
	scl(1);
	delay_half();
	sda(0);
	delay_half();
	scl(0);
}

void i2c_stop(void)
{
	sda(0);
	delay_half();
	scl(1);
	delay_half();
	sda(1);
	delay_half();
}

/*
 * 第 9 个时钟上从机拉低 SDA 表示 ACK (UM10204 3.1.6).
 * 总线浮空 (没接从机, 无上拉) 时 SDA 常读成低, 会被误判为 ACK,
 * 所以调用方不能只靠 ACK 判断从机存在, 还要核对读回的内容.
 */
int i2c_put(uint8_t byte)
{
	int ack;
	int bit;

	for (bit = 7; bit >= 0; bit--) {
		sda((byte >> bit) & 1u);
		delay_half();
		scl(1);
		delay_half();
		scl(0);
	}
	sda(1);
	delay_half();
	scl(1);
	delay_half();
	ack = !sda_read();
	scl(0);
	return ack;
}

uint8_t i2c_get(int ack)
{
	uint8_t byte = 0;
	int bit;

	sda(1);
	for (bit = 0; bit < 8; bit++) {
		delay_half();
		scl(1);
		delay_half();
		byte = (uint8_t)((byte << 1) | (uint8_t)sda_read());
		scl(0);
	}
	sda(ack ? 0 : 1);
	delay_half();
	scl(1);
	delay_half();
	scl(0);
	sda(1);
	return byte;
}

int i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t value)
{
	int ok;

	i2c_start();
	ok = i2c_put((uint8_t)(addr7 << 1)) && i2c_put(reg) && i2c_put(value);
	i2c_stop();
	return ok ? 0 : -1;
}

int i2c_read_regs(uint8_t addr7, uint8_t reg, uint8_t *buf, unsigned len)
{
	unsigned i;

	i2c_start();
	if (!i2c_put((uint8_t)(addr7 << 1)) || !i2c_put(reg)) {
		i2c_stop();
		return -1;
	}
	i2c_start();
	if (!i2c_put((uint8_t)((addr7 << 1) | 1u))) {
		i2c_stop();
		return -1;
	}
	for (i = 0; i < len; i++)
		buf[i] = i2c_get(i + 1 < len);
	i2c_stop();
	return 0;
}
