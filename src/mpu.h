#ifndef MPU_H
#define MPU_H

#include <stdint.h>

typedef struct {
	float pitch;      /* 度 */
	float roll;       /* 度 */
	float yaw;        /* 度, 只靠陀螺仪积分, 会漂移 */
	float acc_g;      /* 合加速度, 单位 g */
	uint32_t steps;
} mpu_state_t;

/* 返回 WHO_AM_I 的值, 总线无应答时返回 -1 */
int mpu_init(void);
int mpu_present(void);
int mpu_who_am_i(void);
/* 至少每 10 ms 调用一次, 内部按间隔限速 */
void mpu_update(void);
void mpu_calibrate_gyro(void);
/* 偏航角和步数清零 */
void mpu_zero(void);
/* 检测到剧烈晃动时返回 1, 读取后清除 */
int mpu_take_shake(void);
const mpu_state_t *mpu_state(void);

#endif
