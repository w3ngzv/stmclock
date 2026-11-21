#ifndef ACTUATORS_H
#define ACTUATORS_H

void actuators_init(void);
/* 每个主循环调用一次, 根据当前状态驱动蜂鸣器和舵机 */
void actuators_update(void);
/* 闹钟响铃: 蜂鸣器按节奏响, vibrate 为 1 时舵机来回摆动 */
void actuators_ring(int on, int vibrate);
void actuators_test_beep(void);
void actuators_test_vibrate(void);

#endif
