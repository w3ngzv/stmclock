#include "mpu.h"
#include "fastmath.h"
#include "i2c.h"
#include "sys.h"

/*
 * 寄存器地址和量程系数取自 MPU-6000/6050 寄存器手册: CONFIG 4.3,
 * GYRO_CONFIG 4.4, ACCEL_CONFIG 4.5, 加速度数据 4.18 (4 g 为 8192 LSB/g),
 * 陀螺仪数据 4.20 (500 dps 为 65.5 LSB/dps), PWR_MGMT_1 4.30,
 * WHO_AM_I 4.34.
 *
 * 清单里的是 MPU6500, 它的寄存器手册没有取得: TDK 官网上该 PDF 的地址现在
 * 返回的是网页. 6050 的 WHO_AM_I 默认值为 0x68 (手册 4.34). 6500 为 0x70
 * 来自 TDK 论坛的回复, 不是手册, 没有核对. 所以驱动不按 ID 拒绝, 只把读到
 * 的值显示在系统页.
 */
#define MPU_ADDR        0x68u
#define REG_CONFIG      0x1Au
#define REG_GYRO_CFG    0x1Bu
#define REG_ACCEL_CFG   0x1Cu
#define REG_ACCEL_XOUT  0x3Bu
#define REG_PWR_MGMT_1  0x6Bu
#define REG_WHO_AM_I    0x75u

#define PWR_RESET       0x80u
#define PWR_CLK_PLL_X   0x01u
#define DLPF_44HZ       0x03u
#define GYRO_FS_500DPS  0x08u
#define ACCEL_FS_4G     0x08u

#define ACCEL_LSB_PER_G 8192.0f   /* +-4 g */
#define GYRO_LSB_PER_DPS 65.5f    /* +-500 dps */

#define SAMPLE_MIN_MS   10u
#define DT_MAX_S        0.2f
#define FILTER_TAU_S    0.5f      /* 互补滤波的时间常数 */
#define CAL_SAMPLES     64
#define CAL_DELAY_MS    5u

#define STEP_ARM_G      1.05f
#define STEP_TRIGGER_G  1.20f
#define STEP_MIN_GAP_MS 250u
#define SHAKE_DELTA_G   0.8f
#define SHAKE_HOLDOFF_MS 1000u

static int present;
static int who_am_i = -1;
static mpu_state_t st;
static float gyro_bias[3];
static uint32_t last_ms;
static int step_armed = 1;
static uint32_t last_step_ms;
static uint32_t last_shake_ms;
static volatile int shake_flag;

static int16_t be16(const uint8_t *p)
{
	return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static int read_raw(float acc[3], float gyro[3])
{
	uint8_t b[14];
	int i;

	if (i2c_read_regs(MPU_ADDR, REG_ACCEL_XOUT, b, sizeof(b)) != 0)
		return -1;
	for (i = 0; i < 3; i++) {
		acc[i] = (float)be16(&b[2 * i]) / ACCEL_LSB_PER_G;
		gyro[i] = (float)be16(&b[8 + 2 * i]) / GYRO_LSB_PER_DPS;
	}
	return 0;
}

static float wrap180(float a)
{
	while (a > 180.0f)
		a -= 360.0f;
	while (a < -180.0f)
		a += 360.0f;
	return a;
}

int mpu_init(void)
{
	uint8_t id;

	present = 0;
	if (i2c_write_reg(MPU_ADDR, REG_PWR_MGMT_1, PWR_RESET) != 0)
		return -1;
	delay_ms(100); /* 复位后需要等待寄存器恢复默认值 */
	i2c_write_reg(MPU_ADDR, REG_PWR_MGMT_1, PWR_CLK_PLL_X);
	delay_ms(10);
	i2c_write_reg(MPU_ADDR, REG_CONFIG, DLPF_44HZ);
	i2c_write_reg(MPU_ADDR, REG_GYRO_CFG, GYRO_FS_500DPS);
	i2c_write_reg(MPU_ADDR, REG_ACCEL_CFG, ACCEL_FS_4G);

	if (i2c_read_regs(MPU_ADDR, REG_WHO_AM_I, &id, 1) != 0)
		return -1;
	/* 总线浮空时 SDA 读成全 0 或全 1, 会被软件 I2C 当成应答, 这里再排除一次 */
	if (id == 0x00 || id == 0xFF)
		return -1;
	who_am_i = id;
	present = 1;
	mpu_calibrate_gyro();
	last_ms = g_ms;
	return who_am_i;
}

int mpu_present(void) { return present; }
int mpu_who_am_i(void) { return who_am_i; }

/* TODO: 校准要求模块静止, 没有检测校准过程中是否被移动 */
void mpu_calibrate_gyro(void)
{
	float acc[3], gyro[3], sum[3] = { 0, 0, 0 };
	int n = 0, i, k;

	if (!present)
		return;
	for (i = 0; i < CAL_SAMPLES; i++) {
		if (read_raw(acc, gyro) == 0) {
			for (k = 0; k < 3; k++)
				sum[k] += gyro[k];
			n++;
		}
		delay_ms(CAL_DELAY_MS);
	}
	if (n == 0)
		return;
	for (k = 0; k < 3; k++)
		gyro_bias[k] = sum[k] / (float)n;
}

void mpu_zero(void)
{
	st.yaw = 0.0f;
	st.steps = 0;
}

void mpu_update(void)
{
	float acc[3], gyro[3];
	float dt, roll_acc, pitch_acc, k, mag;
	uint32_t now = g_ms;

	if (!present || now - last_ms < SAMPLE_MIN_MS)
		return;
	if (read_raw(acc, gyro) != 0)
		return;

	dt = (float)(now - last_ms) / 1000.0f;
	last_ms = now;
	if (dt > DT_MAX_S)
		dt = DT_MAX_S;

	/*
	 * 轴向约定: x 向前, y 向左, z 向上, 模块水平放置时 az 约为 +1 g.
	 * TODO: 模块在面包板上的朝向没有确认, 角度正负与实际倾斜方向相反时
	 * 调换这里的符号. 本机没有模块, 未验证.
	 */
	roll_acc = fm_atan2_deg(acc[1], acc[2]);
	pitch_acc = fm_atan2_deg(-acc[0], fm_sqrt(acc[1] * acc[1] + acc[2] * acc[2]));

	st.roll += (gyro[0] - gyro_bias[0]) * dt;
	st.pitch += (gyro[1] - gyro_bias[1]) * dt;
	st.yaw = wrap180(st.yaw + (gyro[2] - gyro_bias[2]) * dt);

	/* 互补滤波: 短时信陀螺仪, 长时信加速度计, 误差先折到 +-180 度再修正 */
	k = dt / (FILTER_TAU_S + dt);
	st.roll = wrap180(st.roll + k * wrap180(roll_acc - st.roll));
	st.pitch = wrap180(st.pitch + k * wrap180(pitch_acc - st.pitch));

	mag = fm_sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
	st.acc_g = mag;

	/*
	 * 计步: 合加速度先降到 STEP_ARM_G 以下才允许触发, 再升过 STEP_TRIGGER_G
	 * 计一步, 两步之间至少间隔 STEP_MIN_GAP_MS.
	 * TODO: 阈值是估计值, 没有佩戴行走的数据, 需要朋友实测后调整.
	 */
	if (mag < STEP_ARM_G)
		step_armed = 1;
	if (step_armed && mag > STEP_TRIGGER_G && now - last_step_ms >= STEP_MIN_GAP_MS) {
		st.steps++;
		step_armed = 0;
		last_step_ms = now;
	}

	if (fm_abs(mag - 1.0f) > SHAKE_DELTA_G && now - last_shake_ms >= SHAKE_HOLDOFF_MS) {
		last_shake_ms = now;
		shake_flag = 1;
	}
}

int mpu_take_shake(void)
{
	int v = shake_flag;

	shake_flag = 0;
	return v;
}

const mpu_state_t *mpu_state(void)
{
	return &st;
}
