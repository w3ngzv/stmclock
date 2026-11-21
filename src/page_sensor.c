#include "app.h"
#include "input.h"
#include "oled.h"

/*
 * TODO(练习: 传感器, 考核第 11 项): 本页目前只占了菜单里的位置, 读数和显示
 * 都要你来写. 引脚在 board.h 末尾: 光敏 PA4 (ADC1_IN4), 热敏 PA5 (ADC1_IN5),
 * 反射式红外 PA6, 对射式红外 PA7. ADC1 的时钟已在 sys.c 打开, 经 ADCPRE 六
 * 分频为 12 MHz.
 *
 * 建议的顺序, 每一步都先在屏幕上显示原始值, 确认对了再往下做:
 *
 * 1. 红外 (最简单, 只是读 GPIO). 用 gpio_config() 把 PA6/PA7 设成
 *    GPIO_IN_FLOAT, 用 gpio_read() 读. 这类模块的 DO 常见是检测到物体时为
 *    低电平, 模块上的指示灯同时亮, 先用手挡一下对照指示灯确认.
 *
 * 2. ADC 驱动 (新建 adc.c / adc.h). 寄存器全在 RM0008 第 11 章,
 *    ADC1 基地址 0x40012400 (RM0008 3.3 的存储器映射表).
 *    - 初始化: PA4/PA5 设成 GPIO_ANALOG. 置 ADC_CR2 的 ADON 上电, 等至少
 *      两个 ADC 时钟, 再置 CAL 做校准, 等 CAL 被硬件清零 (11.4).
 *    - 采样时间: ADC_SMPR2 (11.12.5). 传感器模块的输出阻抗不低, 取最长的
 *      239.5 周期, 太短读数会偏低且跳动.
 *    - 读一个通道: 通道号写进 ADC_SQR3 的 SQ1 (11.12.11), 再写一次 ADON
 *      启动单次转换 (11.3.4), 等 ADC_SR 的 EOC (11.12.1), 读 ADC_DR
 *      (11.12.14), 结果是 0 到 4095.
 *
 * 3. 光敏. 先把 PA4 的原始值显示出来, 分别遮住和照亮光敏电阻, 记下两种情况
 *    的读数, 中间取一个阈值. 读数随变暗是增大还是减小取决于模块的分压接法,
 *    要实测, 不要照抄别人的代码.
 *
 * 4. 热敏 (温度). 模块上是 NTC 热敏电阻和一个固定电阻分压. 准确换算要知道
 *    NTC 的标称阻值和 B 值, 写在模块或卖家页面上, 常见是 10k 和 3950, 以你
 *    手上的为准. 换算公式 1/T = 1/T0 + ln(R/R0)/B 需要 ln, 而工程没有链接
 *    libm (见 Makefile). 更简单的做法是两点标定: 记下室温和手捏住时的读数
 *    与温度计读数, 中间按直线插值, 精度对考核够用.
 *
 * 5. 光感亮屏: 和 main.c 里自动熄屏的 TODO 一起做.
 *
 * 验收: 本页同时显示光敏和热敏的原始值、换算后的温度、两个红外的状态,
 * 遮挡或加热时数值随之变化.
 */
static void sensor_update(const input_state_t *in)
{
	(void)in;

	gfx_clear();
	gfx_text(0, 2, "SENSOR", 1, 1);
	gfx_hline(0, 11, OLED_WIDTH, 1);
	gfx_text(0, 24, "NOT IMPLEMENTED", 1, 1);
	/* 字库只到 0x5A, 下划线显示不出来, 所以不写文件名 */
	gfx_text(0, 36, "SEE TODO IN SOURCE", 1, 1);
}

const page_t page_sensor = {
	.name = "SENSOR", .update = sensor_update,
	.keep_awake = 1,
};
