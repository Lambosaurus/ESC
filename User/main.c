
#include "Core.h"
#include "GPIO.h"



int main(void)
{
	CORE_Init();
	GPIO_EnableOutput(LED_GPIO, LED_GRN_PIN, GPIO_PIN_SET);
	GPIO_EnableOutput(LED_GPIO, LED_RED_PIN, GPIO_PIN_RESET);

	while(1)
	{
		CORE_Idle();
	}
}

