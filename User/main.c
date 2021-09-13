
#include "Core.h"
#include "GPIO.h"

#include "UART.h"
#include <stdlib.h>
#include <stdio.h>

#include "Line.h"
#include "BLDC.h"

#include "COMP.h"

#define CLAMP(v, low, high) (v < low ? low : (v > high ? high : v))

#define HALL_TIM	CTL_TIM
#define HALL_RLD	256

static LineParser_t gLine;
static char gLineBuffer[64];

void Main_HandleLine(char * line)
{
	int pwm = atoi(line);
	while (*line != ',' && *line != 0)
	{
		line++;
	}

	pwm = CLAMP(pwm, 0, 256);

	if (pwm == 0)
	{
		BLDC_Stop();
	}
	else
	{
		BLDC_SetPower(pwm);
		BLDC_Start(100);
	}
}

int main(void)
{
	CORE_Init();
	GPIO_EnableOutput(LED_GPIO, LED_GRN_PIN, GPIO_PIN_SET);
	GPIO_EnableOutput(LED_GPIO, LED_RED_PIN, GPIO_PIN_RESET);

	UART_Init(COM_UART, COM_BAUD, UART_Mode_Default);
	Line_Init(&gLine, gLineBuffer, sizeof(gLineBuffer), Main_HandleLine);

	BLDC_Init();

	uint32_t tide = CORE_GetTick();

	while(1)
	{
		uint32_t now = CORE_GetTick();

		char bfr[32];
		uint32_t read = UART_Read(COM_UART, (uint8_t*)bfr, sizeof(bfr));
		if (read)
		{
			Line_Parse(&gLine, bfr, read);
		}
		if (now - tide > 500)
		{
			uint32_t rate = BLDC_GetRPM();
			tide = now;
			snprintf(bfr, sizeof(bfr), "Rate: %d\r\n", (int)rate);
			UART_WriteStr(COM_UART, bfr);
		}

		bool slp = BLDC_Update();
		BLDC_State_t state = BLDC_GetState();

		GPIO_Write(LED_GPIO, LED_RED_PIN, state == BLDC_State_Fault);
		if (slp)
		{
			CORE_Idle();
		}
	}
}

