# ESP8266-PingPong-Collection
嵌入式WiFi芯片 ESP8266 项目运用实例——室内乒乓球收集项目
------------------------------------------------------------------------------------------
- 本程序STM32ZET6串口通讯例程基础上，使用两个串口（USART1、USART2）

  USART1：PA9(TX)，PA10(RX)用CH340通过USB与电脑连接

  USART2：PA2(TX)，PA3(RX)与WiFi模块连接，使用AT指令控制

- 使用TIM3_CH4通道，PB1，产生PWM信号，控制步进电机转动；PF11控制电机方向，PF13控制电机使能

- PA5采集左限位开关，PA7采集中间限位开关，PC5采集右限位开关

- PB0控制状态灯1，PC4控制状态灯2，PA6控制状态灯3，PA2控制状态灯4

- PF12采集按键1，PB2采集按键2

- PC3，PA1控制继电器，从而控制风机启停

- PF14，pF15，PG0保留

