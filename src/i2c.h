#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
/* 发送一个字节, 返回 1 表示从机应答 */
int i2c_put(uint8_t byte);
uint8_t i2c_get(int ack);

/* 以下两个封装一次完整事务, 返回 0 成功, -1 为地址或数据无应答 */
int i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t value);
int i2c_read_regs(uint8_t addr7, uint8_t reg, uint8_t *buf, unsigned len);

#endif
