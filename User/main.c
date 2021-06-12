
#include "Core.h"
#include "GPIO.h"

#include "MP6532.h"
#include "UART.h"
#include "TIM.h"
#include "COMP.h"
#include <stdlib.h>
#include <stdio.h>

#include "Line.h"

#define CLAMP(v, low, high) (v < low ? low : (v > high ? high : v))

#define HALL_TIM	CTL_TIM
#define HALL_RLD	256

static LineParser_t gLine;
static char gLineBuffer[64];

void Main_HandleLine(char * line)
{
	int pwm = atoi(line);
	int freq = 0;
	while (*line != ',' && *line != 0)
	{
		line++;
	}
	if (*line != 0)
	{
		line++;
		freq = atoi(line);
	}

	pwm = CLAMP(pwm, 0, 255);
	freq = CLAMP(freq, 1, 30000);

	MP6532_SetDuty(pwm);
	TIM_SetFreq(HALL_TIM, freq * HALL_RLD * 6);
}

static uint32_t gLast = 0;
static uint32_t gDelta = 0;

static void Main_OnComp(void)
{
	uint32_t now = CORE_GetTick();
	gDelta = now - gLast;
	gLast = now;
}

int main(void)
{
	CORE_Init();
	GPIO_EnableOutput(LED_GPIO, LED_GRN_PIN, GPIO_PIN_SET);
	GPIO_EnableOutput(LED_GPIO, LED_RED_PIN, GPIO_PIN_RESET);

	UART_Init(COM_UART, COM_BAUD);
	Line_Init(&gLine, gLineBuffer, sizeof(gLineBuffer), Main_HandleLine);

	MP6532_Init();

	TIM_Init(HALL_TIM, HALL_RLD, HALL_RLD);
	TIM_OnReload(HALL_TIM, MP6532_Step);
	TIM_Start(HALL_TIM);

	COMP_Init(COMP_2, COMP_Pos_IO2 | COMP_Neg_IO2);
	COMP_OnChange(COMP_2, GPIO_IT_Rising, Main_OnComp);

	while(1)
	{
		char bfr[32];
		uint32_t read = UART_Read(COM_UART, (uint8_t*)bfr, sizeof(bfr));
		if (read)
		{
			Line_Parse(&gLine, bfr, read);
		}

		GPIO_Write(LED_GPIO, LED_RED_PIN, MP6532_IsFaulted());
		//CORE_Idle();

		CORE_Delay(250);

		uint32_t size = snprintf(bfr, sizeof(bfr), "Delta = %d\r\n", (int)gDelta);
		UART_Write(COM_UART, (uint8_t *)bfr, size);
	}
}

