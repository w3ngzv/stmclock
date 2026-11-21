stmclock: STM32F103C8T6 面包板智能手表

范围

本文件列出各组件与核心板的接线, 以及编译和烧录的命令. 引脚定义的来源是
src/board.h, 两处不一致时以 board.h 为准.

所有模块的地线与核心板共地. 不使用 PA13/PA14 (SWD), PA15/PB3/PB4 (JTAG 复用),
PA11/PA12 (USB), PC14/PC15 (32.768 kHz 晶振).

接线

  组件                 模块引脚      核心板引脚   说明
  OLED SSD1306 (I2C)   SCL           PB6          软件 I2C, 地址 0x3C (未验证)
                       SDA           PB7
  MPU6500              SCL           PB6          与 OLED 共用同一条 I2C
                       SDA           PB7
                       AD0           GND          地址 0x68
  W25Q (SPI2)          CS            PB12
                       CLK           PB13
                       DO            PB14         接核心板的 MISO
                       DI            PB15         接核心板的 MOSI
  蓝牙串口             TXD           PA3          交叉连接: 模块发, 核心板收
                       RXD           PA2          USART2, 9600 8N1
  舵机 SG90            信号线        PB0          TIM3_CH3, 电源见下
  有源蜂鸣器           I/O           PB1          低电平响
  旋转编码器           A             PA0          TIM2 编码器模式
                       B             PA1
                       按键          PA8
  按键 BACK            一端          PB5          另一端接 GND, 使用内部上拉
  按键 OK              一端          PB8
  按键 AUX             一端          PB9
  光敏模块             AO            PA4          ADC1_IN4
  热敏模块             AO            PA5          ADC1_IN5
  反射式红外           DO            PA6
  对射式红外           DO            PA7
  板载 LED             (无需接线)    PC13         每秒翻转一次, 表示主循环在运行

舵机电源: SG90 的工作电压为 4.8 V 到 6.0 V (SG90 数据手册), 不能接 3.3 V.
使用核心板的 5 V 引脚或外部 5 V 电源, 并与核心板共地.

软件状态

  已在板上验证: 时钟、PC13 闪烁、SysTick 计时、USART2 寄存器配置、TIM3 PWM、
  PB1 电平.
  未验证, 没有实物: OLED, MPU6500, W25Q, 编码器, 按键, 舵机, 蜂鸣器, 蓝牙模块.
  PA4 到 PA7 的传感器尚未实现, 接线表里只是预留.

编译和烧录

  make            生成 build/stmclock.elf, .hex, .bin
  make test       在主机上运行不依赖寄存器的单元测试
  make flash      通过 ST-Link 和 OpenOCD 烧录并校验

在 Windows 上使用 Keil 的人直接烧录 build/stmclock.hex.

蓝牙命令

每行一条, 以换行结束, 不区分大小写.

  TIME HH:MM[:SS]     设置时间
  DATE YYYY-MM-DD     设置日期
  ALARM HH:MM         设置闹钟并开启
  ALARM ON | OFF      开关闹钟
  VIB ON | OFF        响铃时是否震动
  VIB TEST            舵机震动 1 s
  BEEP TEST           蜂鸣 0.4 s
  STATUS              返回当前时间和闹钟设置
  IMGW                回复 READY 后发送 1024 字节图片数据, 收完回复 OK
