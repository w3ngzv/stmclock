#include "bt.h"
#include "actuators.h"
#include "alarm.h"
#include "cmd.h"
#include "fmt.h"
#include "oled.h"
#include "picture.h"
#include "sys.h"
#include "watch.h"

#define USART2_BASE   0x40004400u
#define USART_SR      REG16(USART2_BASE + 0x00u)
#define USART_DR      REG16(USART2_BASE + 0x04u)
#define USART_BRR     REG16(USART2_BASE + 0x08u)
#define USART_CR1     REG16(USART2_BASE + 0x0Cu)
#define USART_SR_ORE  (1u << 3)
#define USART_SR_RXNE (1u << 5)
#define USART_SR_TXE  (1u << 7)
#define USART_CR1_RE      (1u << 2)
#define USART_CR1_TE      (1u << 3)
#define USART_CR1_RXNEIE  (1u << 5)
#define USART_CR1_UE      (1u << 13)

/* RM0008 Table 63: USART2 的中断号是 38 */
#define USART2_IRQ    38u

/*
 * HC-05 / HC-06 出厂波特率 9600 (模块没有拿到, 凭常见默认值).
 * TODO: 模块型号还没有确认, 如果是 BLE 模块要核对它的串口参数.
 */
#define BT_BAUD       9600u

#define LINE_MAX      64
#define RX_RING       256u    /* 必须是 2 的幂, 下面用掩码取模 */
#define IMG_IDLE_TIMEOUT_MS 2000u

static uint8_t rx_ring[RX_RING];
static volatile uint16_t rx_head;   /* 只由中断写 */
static uint16_t rx_tail;            /* 只由主循环写 */

static char line[LINE_MAX];
static unsigned line_len;
static int line_overflow;

static unsigned img_received;
static int img_active;
static uint32_t img_last_byte_ms;

void usart2_irq_handler(void)
{
	/*
	 * 先读 SR 再读 DR 会同时清除 RXNE 和 ORE (RM0008 27.6.1). 发生溢出时
	 * DR 里的字节仍然有效, 一起存入.
	 */
	uint16_t sr = USART_SR;

	if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
		uint8_t b = (uint8_t)USART_DR;
		uint16_t next = (uint16_t)((rx_head + 1u) & (RX_RING - 1u));

		if (next != rx_tail) {
			rx_ring[rx_head] = b;
			rx_head = next;
		}
	}
}

void bt_init(void)
{
	gpio_config(BT_PORT, BT_TX_PIN, GPIO_AF_PP_2M);
	gpio_config_pullup(BT_PORT, BT_RX_PIN); /* 模块没接时线路空闲为高, 不会收到乱码 */

	RCC_APB1ENR |= RCC_APB1_USART2;
	/* BRR 的值就是 fPCLK1 / 波特率, 整数部分和 4 位小数部分已经拼好, RM0008 27.3.4 */
	USART_BRR = (uint16_t)((g_sysclk_hz / 2u) / BT_BAUD);
	USART_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
	nvic_enable(USART2_IRQ);
}

static void put_str(const char *s)
{
	for (; *s; s++) {
		while (!(USART_SR & USART_SR_TXE))
			;
		USART_DR = (uint8_t)*s;
	}
}

static int rx_pop(void)
{
	uint8_t b;

	if (rx_tail == rx_head)
		return -1;
	b = rx_ring[rx_tail];
	rx_tail = (uint16_t)((rx_tail + 1u) & (RX_RING - 1u));
	return b;
}

static void reply_status(void)
{
	char out[48];
	char *p;

	p = fmt_str(out, "TIME ");
	p = fmt_dec(p, g_now.year, 4);
	*p++ = '-';
	p = fmt_dec(p, g_now.month, 2);
	*p++ = '-';
	p = fmt_dec(p, g_now.day, 2);
	*p++ = ' ';
	p = fmt_dec(p, g_now.hour, 2);
	*p++ = ':';
	p = fmt_dec(p, g_now.min, 2);
	*p++ = ':';
	p = fmt_dec(p, g_now.sec, 2);
	p = fmt_str(p, "\r\n");
	*p = 0;
	put_str(out);

	p = fmt_str(out, "ALARM ");
	p = fmt_dec(p, g_alarm.hour, 2);
	*p++ = ':';
	p = fmt_dec(p, g_alarm.min, 2);
	p = fmt_str(p, g_alarm.enabled ? " ON" : " OFF");
	p = fmt_str(p, g_alarm.vibrate ? " VIB ON" : " VIB OFF");
	p = fmt_str(p, "\r\n");
	*p = 0;
	put_str(out);
}

static void execute(const char *text)
{
	cmd_t cmd = cmd_parse(text);
	datetime_t t = g_now;

	switch (cmd.type) {
	case CMD_NONE:
		return;
	case CMD_BAD:
		put_str("ERR\r\n");
		return;
	case CMD_TIME:
		t.hour = (uint8_t)cmd.a;
		t.min = (uint8_t)cmd.b;
		t.sec = (uint8_t)cmd.c;
		watch_set(&t);
		break;
	case CMD_DATE:
		t.year = (uint16_t)cmd.a;
		t.month = (uint8_t)cmd.b;
		t.day = (uint8_t)cmd.c;
		watch_set(&t);
		break;
	case CMD_ALARM_SET:
		g_alarm.hour = (uint8_t)cmd.a;
		g_alarm.min = (uint8_t)cmd.b;
		g_alarm.enabled = 1;
		break;
	case CMD_ALARM_ON:
		g_alarm.enabled = 1;
		break;
	case CMD_ALARM_OFF:
		g_alarm.enabled = 0;
		break;
	case CMD_VIB_ON:
		g_alarm.vibrate = 1;
		break;
	case CMD_VIB_OFF:
		g_alarm.vibrate = 0;
		break;
	case CMD_VIB_TEST:
		actuators_test_vibrate();
		break;
	case CMD_BEEP_TEST:
		actuators_test_beep();
		break;
	case CMD_STATUS:
		reply_status();
		return;
	case CMD_IMGW:
		img_active = 1;
		img_received = 0;
		img_last_byte_ms = g_ms;
		put_str("READY\r\n");
		return;
	}
	put_str("OK\r\n");
}

static void image_byte(uint8_t b)
{
	picture_buf()[img_received++] = b;
	img_last_byte_ms = g_ms;
	if (img_received == OLED_BUF_SIZE) {
		img_active = 0;
		put_str(picture_save() == 0 ? "OK\r\n" : "ERR\r\n");
	}
}

void bt_poll(void)
{
	int c;

	while ((c = rx_pop()) >= 0) {
		if (img_active) {
			image_byte((uint8_t)c);
			continue;
		}
		if (c == '\r' || c == '\n') {
			if (line_overflow) {
				put_str("ERR\r\n");
			} else if (line_len) {
				line[line_len] = 0;
				execute(line);
			}
			line_len = 0;
			line_overflow = 0;
		} else if (line_len < LINE_MAX - 1u) {
			line[line_len++] = (char)c;
		} else {
			line_overflow = 1;
		}
	}

	if (img_active && g_ms - img_last_byte_ms >= IMG_IDLE_TIMEOUT_MS) {
		img_active = 0;
		put_str("ERR\r\n");
	}
}
