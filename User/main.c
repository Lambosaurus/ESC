
#include "Core.h"
#include "GPIO.h"

#include "MP6532.h"


int main(void)
{
	CORE_Init();
	GPIO_EnableOutput(LED_GPIO, LED_GRN_PIN, GPIO_PIN_SET);
	GPIO_EnableOutput(LED_GPIO, LED_RED_PIN, GPIO_PIN_RESET);

	MP6532_Init();

	// MP6532_SetDuty(10);

	while(1)
	{
		//MP6532_Step();
		CORE_Idle();
	}
}

