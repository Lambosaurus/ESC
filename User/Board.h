#ifndef BOARD_H
#define BOARD_H

#define STM32L0
//#define STM32F0

// Core config
//#define CORE_USE_HSE
//#define CORE_USE_TICK_IRQ

// ADC config
#define ADC_VREF	        3300

// GPIO config
//#define GPIO_USE_IRQS
//#define GPIO_IRQ0_ENABLE

// TIM config
#define TIM_USE_IRQS
#define TIM21_ENABLE
#define TIM22_ENABLE

// UART config
#define UART1_GPIO		GPIOA
#define UART1_PINS		(GPIO_PIN_9 | GPIO_PIN_10)
#define UART1_AF		GPIO_AF4_USART1
#define COM_UART		UART_1
#define COM_BAUD		115200

// MP6532
#define MP6532_GPIO			GPIOA
#define MP6532_HA_PIN		GPIO_PIN_0
#define MP6532_HB_PIN		GPIO_PIN_1
#define MP6532_HC_PIN		GPIO_PIN_2
#define MP6532_SLEEP_PIN	GPIO_PIN_3
#define MP6532_DIR_PIN		GPIO_PIN_4
#define MP6532_BRAKE_PIN	GPIO_PIN_5
#define MP6532_PWM_PIN		GPIO_PIN_6
#define MP6532_PWM_TIM		TIM_22
#define MP6532_PWM_CH		0
#define MP6532_PWM_AF		GPIO_AF5_TIM22
#define MP6532_FAULT_PIN	GPIO_PIN_7

// Current sense
#define ISNS_AIN			ADC_CHANNEL_8
#define ISNS_GAIN			20

// COMP config
#define COMP2_ENABLE
#define COMP_USE_IRQS

// LEDs
#define LED_GPIO			GPIOC
#define LED_GRN_PIN			GPIO_PIN_14
#define LED_RED_PIN			GPIO_PIN_15

#define CTL_TIM				TIM_22

#endif /* BOARD_H */
