#ifndef W25Q_H
#define W25Q_H

#include <stdint.h>

#define W25Q_SECTOR_SIZE 4096u
#define W25Q_PAGE_SIZE   256u

/* 返回 JEDEC ID 的 24 位值, 总线上没有芯片时为 0xFFFFFF 或 0 */
uint32_t w25q_init(void);
int w25q_present(void);
int w25q_read(uint32_t addr, uint8_t *buf, unsigned len);
/* 擦除 addr 所在的 4 KB 扇区, 返回 0 成功, -1 超时 */
int w25q_erase_sector(uint32_t addr);
/* 写入前目标区域必须已经擦除, 内部按 256 字节页边界拆分 */
int w25q_write(uint32_t addr, const uint8_t *buf, unsigned len);

#endif
