
#include "MP6532.h"
#include "GPIO.h"
#include "TIM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define MP6532_HX_PINS 		(MP6532_HA_PIN | MP6532_HB_PIN | MP6532_HC_PIN)
#define PWM_FREQ			20000
#define PWM_RES				255

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

void MP6532_EnterPhaseA(void);

/*
 * PRIVATE VARIABLES
 */

static struct
{
	Phase_t phase;
}gState;

/*
 * PUBLIC FUNCTIONS
 */

void MP6532_Init(void)
{
	GPIO_EnableOutput(MP6532_GPIO, MP6532_HX_PINS, GPIO_PIN_RESET);
	GPIO_EnableOutput(MP6532_GPIO, MP6532_SLEEP_PIN, GPIO_PIN_SET);
	GPIO_EnableOutput(MP6532_GPIO, MP6532_DIR_PIN, GPIO_PIN_RESET);
	GPIO_EnableOutput(MP6532_GPIO, MP6532_BRAKE_PIN, GPIO_PIN_RESET);
	GPIO_EnableInput(MP6532_GPIO, MP6532_FAULT_PIN, GPIO_Pull_Up);

	TIM_Init(MP6532_PWM_TIM, PWM_FREQ * PWM_RES, PWM_RES);
	TIM_SetPulse(MP6532_PWM_TIM, MP6532_PWM_CH, 0);
	TIM_EnablePwm(MP6532_PWM_TIM, MP6532_PWM_CH, MP6532_GPIO, MP6532_PWM_PIN, MP6532_PWM_AF);
	TIM_Start(MP6532_PWM_TIM);

	MP6532_EnterPhaseA();
}

void MP6532_Deinit(void)
{
	TIM_Deinit(MP6532_PWM_TIM);
	GPIO_Deinit(MP6532_GPIO, MP6532_PWM_PIN);
	GPIO_Deinit(MP6532_GPIO, MP6532_SLEEP_PIN);
	GPIO_Deinit(MP6532_GPIO, MP6532_HX_PINS);
	GPIO_Deinit(MP6532_GPIO, MP6532_DIR_PIN);
	GPIO_Deinit(MP6532_GPIO, MP6532_BRAKE_PIN);
	GPIO_Deinit(MP6532_GPIO, MP6532_FAULT_PIN);
}

bool MP6532_IsFaulted(void)
{
	return !GPIO_Read(MP6532_GPIO, MP6532_FAULT_PIN);
}

void MP6532_SetDuty(uint8_t duty)
{
	GPIO_Write(MP6532_GPIO, MP6532_BRAKE_PIN, duty != 0);
	TIM_SetPulse(MP6532_PWM_TIM, MP6532_PWM_CH, duty);
}

void MP6532_EnterPhaseA(void)
{
	gState.phase = Phase_A;
	GPIO_Reset(MP6532_GPIO, MP6532_HB_PIN | MP6532_HC_PIN);
	GPIO_Set(MP6532_GPIO, MP6532_HA_PIN);
}

Phase_t MP6532_Step(void)
{
	if (++gState.phase > Phase_CA)
	{
		gState.phase = Phase_A;
	}

	switch (gState.phase)
	{
	case Phase_A:
		GPIO_Reset(MP6532_GPIO, MP6532_HC_PIN);
		break;
	case Phase_AB:
		GPIO_Set(MP6532_GPIO, MP6532_HB_PIN);
		break;
	case Phase_B:
		GPIO_Reset(MP6532_GPIO, MP6532_HA_PIN);
		break;
	case Phase_BC:
		GPIO_Set(MP6532_GPIO, MP6532_HC_PIN);
		break;
	case Phase_C:
		GPIO_Reset(MP6532_GPIO, MP6532_HB_PIN);
		break;
	case Phase_CA:
		GPIO_Set(MP6532_GPIO, MP6532_HA_PIN);
		break;
	}

	return gState.phase;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * INTERRUPT ROUTINES
 */


