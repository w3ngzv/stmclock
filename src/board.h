#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))
#define REG16(addr) (*(volatile uint16_t *)(addr))

/*
 * 没有引入 ST 的头文件, 寄存器地址在这里手写, 依据的文档如下. 其他源文件
 * 的注释用简称引用章节号.
 *
 *   RM0008   STM32F10xxx 参考手册 Rev 15 (st.com)
 *   PM0056   STM32F10xxx Cortex-M3 编程手册 Rev 5 (st.com)
 *   DS5319   STM32F103x8/xB 数据手册 Rev 20 (st.com)
 *   UM10204  NXP I2C 总线规范 Rev. 7.0 (nxp.com)
 *   SSD1306  cdn-shop.adafruit.com/datasheets/SSD1306.pdf
 *   W25Q80DV 数据手册 Rev H 2015-10-02 (winbond.com)
 *   MPU60X0  RM-MPU-6000A 寄存器手册 (cdn.sparkfun.com)
 *   SG90     TowerPro SG90 数据手册
 *
 * 清单里的陀螺仪是 MPU6500, 它的寄存器手册没有取得, 见 mpu.c 开头.
 */
#define RCC_BASE        0x40021000u
#define RCC_CR          REG32(RCC_BASE + 0x00u)
#define RCC_CFGR        REG32(RCC_BASE + 0x04u)
#define RCC_APB2ENR     REG32(RCC_BASE + 0x18u)
#define RCC_APB1ENR     REG32(RCC_BASE + 0x1Cu)
#define FLASH_ACR       REG32(0x40022000u)

#define RCC_CR_HSEON        (1u << 16)
#define RCC_CR_HSERDY       (1u << 17)
#define RCC_CR_PLLON        (1u << 24)
#define RCC_CR_PLLRDY       (1u << 25)
#define RCC_CFGR_SW_PLL     2u
#define RCC_CFGR_SWS_MASK   (3u << 2)
#define RCC_CFGR_SWS_PLL    (2u << 2)
#define RCC_CFGR_PPRE1_DIV2 (4u << 8)
#define RCC_CFGR_ADCPRE_DIV6 (2u << 14)
#define RCC_CFGR_PLLSRC_HSE (1u << 16)
#define RCC_CFGR_PLLMUL(n)  (((n) - 2u) << 18)
#define FLASH_ACR_LATENCY_2 2u
#define FLASH_ACR_PRFTBE    (1u << 4)

#define RCC_APB2_AFIO   (1u << 0)
#define RCC_APB2_IOPA   (1u << 2)
#define RCC_APB2_IOPB   (1u << 3)
#define RCC_APB2_IOPC   (1u << 4)
#define RCC_APB2_ADC1   (1u << 9)
#define RCC_APB1_TIM2   (1u << 0)
#define RCC_APB1_TIM3   (1u << 1)
#define RCC_APB1_SPI2   (1u << 14)
#define RCC_APB1_USART2 (1u << 17)

#define GPIOA_BASE      0x40010800u
#define GPIOB_BASE      0x40010C00u
#define GPIOC_BASE      0x40011000u
#define GPIO_IDR(base)  REG32((base) + 0x08u)
#define GPIO_ODR(base)  REG32((base) + 0x0Cu)
#define GPIO_BSRR(base) REG32((base) + 0x10u)

/* 端口配置寄存器每脚 4 位, 高 2 位 CNF, 低 2 位 MODE (RM0008 9.2.1) */
#define GPIO_ANALOG     0x0u
#define GPIO_IN_FLOAT   0x4u
#define GPIO_IN_PULL    0x8u
#define GPIO_OUT_PP_2M  0x2u
#define GPIO_OUT_PP_50M 0x3u
#define GPIO_OUT_OD_50M 0x7u
#define GPIO_AF_PP_2M   0xAu
#define GPIO_AF_PP_50M  0xBu

#define TIM2_BASE       0x40000000u
#define TIM3_BASE       0x40000400u
#define TIM_CR1(b)      REG16((b) + 0x00u)
#define TIM_SMCR(b)     REG16((b) + 0x08u)
#define TIM_EGR(b)      REG16((b) + 0x14u)
#define TIM_CCMR1(b)    REG16((b) + 0x18u)
#define TIM_CCMR2(b)    REG16((b) + 0x1Cu)
#define TIM_CCER(b)     REG16((b) + 0x20u)
#define TIM_CNT(b)      REG16((b) + 0x24u)
#define TIM_PSC(b)      REG16((b) + 0x28u)
#define TIM_ARR(b)      REG16((b) + 0x2Cu)
#define TIM_CCR3(b)     REG16((b) + 0x3Cu)

/*
 * 引脚分配 (F103C8T6 核心板). 这张表同时是给接线用的清单.
 * 避开了 PA13/PA14 (SWD), PA15/PB3/PB4 (JTAG 复用), PA11/PA12 (USB),
 * PC14/PC15 (32.768 kHz 晶振).
 *
 *   OLED SCL / SDA   PB6 / PB7   与 MPU6500 共用软件 I2C
 *   MPU6500 SCL/SDA  PB6 / PB7   AD0 接地, 地址 0x68
 *   W25Q CS          PB12        SPI2
 *   W25Q CLK         PB13        SPI2_SCK
 *   W25Q DO (MISO)   PB14        SPI2_MISO
 *   W25Q DI (MOSI)   PB15        SPI2_MOSI
 *   蓝牙 TXD -> PA3, 蓝牙 RXD <- PA2   USART2, 9600 8N1
 *   舵机信号         PB0         TIM3_CH3
 *   有源蜂鸣器       PB1         低电平触发
 *   编码器 A / B     PA0 / PA1   TIM2 编码器模式
 *   编码器按键       PA8
 *   按键 BACK/OK/AUX PB5 / PB8 / PB9   另一端接地, 内部上拉
 *   光敏模块 AO      PA4         ADC1_IN4
 *   热敏模块 AO      PA5         ADC1_IN5
 *   反射式红外 DO    PA6
 *   对射式红外 DO    PA7
 *   板载 LED         PC13        每秒翻转一次, 作为主循环存活指示
 */
#define I2C_PORT        GPIOB_BASE
#define I2C_SCL_PIN     6u
#define I2C_SDA_PIN     7u

#define FLASH_CS_PORT   GPIOB_BASE
#define FLASH_CS_PIN    12u
#define SPI2_PORT       GPIOB_BASE
#define SPI2_SCK_PIN    13u
#define SPI2_MISO_PIN   14u
#define SPI2_MOSI_PIN   15u

#define BT_PORT         GPIOA_BASE
#define BT_TX_PIN       2u
#define BT_RX_PIN       3u

#define SERVO_PORT      GPIOB_BASE
#define SERVO_PIN       0u
#define BUZZER_PORT     GPIOB_BASE
#define BUZZER_PIN      1u

#define ENC_PORT        GPIOA_BASE
#define ENC_A_PIN       0u
#define ENC_B_PIN       1u
#define ENC_SW_PORT     GPIOA_BASE
#define ENC_SW_PIN      8u

#define KEY_PORT        GPIOB_BASE
#define KEY_BACK_PIN    5u
#define KEY_OK_PIN      8u
#define KEY_AUX_PIN     9u

#define LIGHT_PORT      GPIOA_BASE
#define LIGHT_PIN       4u
#define LIGHT_ADC_CH    4u
#define THERM_PORT      GPIOA_BASE
#define THERM_PIN       5u
#define THERM_ADC_CH    5u
#define IR_REFLECT_PORT GPIOA_BASE
#define IR_REFLECT_PIN  6u
#define IR_BEAM_PORT    GPIOA_BASE
#define IR_BEAM_PIN     7u

#define LED_PORT        GPIOC_BASE
#define LED_PIN         13u

#endif
