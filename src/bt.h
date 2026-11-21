#ifndef BT_H
#define BT_H

#include <stdint.h>

void bt_init(void);
/* 每个主循环调用一次, 取出接收缓冲中的字节, 组成命令并执行 */
void bt_poll(void);

#endif
