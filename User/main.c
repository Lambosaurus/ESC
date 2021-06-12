
#include "Core.h"
#include "GPIO.h"

#include "MP6532.h"
#include "UART.h"
#include "TIM.h"
#include <stdlib.h>

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

	while(1)
	{
		char bfr[32];
		uint32_t read = UART_Read(COM_UART, (uint8_t*)bfr, sizeof(bfr));
		if (read)
		{
			Line_Parse(&gLine, bfr, read);
		}

		GPIO_Write(LED_GPIO, LED_RED_PIN, MP6532_IsFaulted());
		CORE_Idle();
	}
}

