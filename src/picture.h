#ifndef PICTURE_H
#define PICTURE_H

#include <stdint.h>

/* 图片以 SSD1306 显存的原始排列保存, 128x64 单色共 1024 字节 */
uint8_t *picture_buf(void);
int picture_valid(void);
/* 从 W25Q 读入缓冲, 返回 0 成功, -1 没有芯片或没有有效图片 */
int picture_load(void);
/* 擦除首个扇区后写入文件头与缓冲, 写完读回比较. 0 成功, -1 写入失败, -2 校验失败 */
int picture_save(void);
/* 在缓冲中生成一张示例图, 用于没有上位机时验证存储和显示 */
void picture_make_demo(void);

#endif
