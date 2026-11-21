#ifndef CMD_H
#define CMD_H

/*
 * 蓝牙串口收到的文本命令, 每行一条, 不区分大小写:
 *   TIME HH:MM[:SS]      DATE YYYY-MM-DD
 *   ALARM HH:MM          设置并开启闹钟
 *   ALARM ON | OFF       VIB ON | OFF   闹钟响时是否震动
 *   VIB TEST             BEEP TEST      立即试震动和试蜂鸣
 *   STATUS               IMGW           随后发送 1024 字节图片数据
 */
typedef enum {
	CMD_NONE,       /* 空行, 或模块回显的 OK / ERR, 不需要回复 */
	CMD_BAD,
	CMD_TIME,       /* a 时, b 分, c 秒 */
	CMD_DATE,       /* a 年, b 月, c 日 */
	CMD_ALARM_SET,  /* a 时, b 分 */
	CMD_ALARM_ON,
	CMD_ALARM_OFF,
	CMD_VIB_ON,
	CMD_VIB_OFF,
	CMD_VIB_TEST,
	CMD_BEEP_TEST,
	CMD_STATUS,
	CMD_IMGW,
} cmd_type_t;

typedef struct {
	cmd_type_t type;
	int a, b, c;
} cmd_t;

cmd_t cmd_parse(const char *line);

#endif
