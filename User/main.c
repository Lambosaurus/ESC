
#include "Core.h"
#include "GPIO.h"

#include "MP6532.h"
#include "UART.h"
#include "TIM.h"
#include <stdlib.h>

#define CLAMP(v, low, high) (v < low ? low : (v > high ? high : v))

#define HALL_TIM	TIM_22
#define HALL_RLD	256

typedef struct {
	char * buffer;
	uint32_t size;
	uint32_t index;
	void (*callback)(char * line);
} LineParser_t;

void Line_Init(LineParser_t * line, void * buffer, uint32_t size, void (*callback)(char * line))
{
	line->buffer = (char *)buffer;
	line->size = size;
	line->index = 0;
	line->callback = callback;
}

void Line_Parse(LineParser_t * line, char * read, uint32_t count)
{
	while(count--)
	{
		char ch = *read--;
		switch (ch)
		{
		case '\n':
		case '\r':
		case 0:
			if (line->index)
			{
				line->buffer[line->index] = 0;
				line->callback(line->buffer);
				line->index = 0;
			}
			break;
		default:
			// Leave room for null terminator
			if (line->index >= line->size - 1)
			{
				// Discard the entire line
				line->index = 0;
			}
			else
			{
				line->buffer[line->index++] = ch;
			}
			break;
		}
	}
}

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

	UART_Init(COM_UART, 115200);
	char bfr[64];
	LineParser_t line;
	Line_Init(&line, bfr, sizeof(bfr), Main_HandleLine);

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
			Line_Parse(&line, bfr, read);
		}

		GPIO_Write(LED_GPIO, LED_RED_PIN, MP6532_IsFaulted());
		CORE_Idle();
	}
}

