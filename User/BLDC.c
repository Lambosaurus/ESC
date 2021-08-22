
#include "BLDC.h"
#include "MP6532.h"
#include "Core.h"
#include "COMP.h"
#include "TIM.h"

/*
 * PRIVATE DEFINITIONS
 */

//#define BLDC_TEST_CH		BLDC_COMP_CHC
#define BLDC_TEST_GPIO		GPIOA
#define BLDC_TEST_PIN		GPIO_PIN_8

#define BLDC_EN_CLOSED_LOOP
#define BLDC_OPEN_LOOP_FREQ		500
#define BLDC_START_RAMP			2
#define BLDC_STALL_MS		50

//#define BLDC_COMP_INV
#define BLDC_COMP			COMP_2
#define BLDC_COMP_COM		COMP_Neg_IO2
#define BLDC_COMP_CHA		COMP_Pos_IO4
#define BLDC_COMP_CHB		COMP_Pos_IO3
#define BLDC_COMP_CHC		COMP_Pos_IO2


#ifdef BLDC_COMP_INV
#define BLDC_COMP_RISING	COMP_Input_Inverted
#define BLDC_COMP_FALLING	0
#else
#define BLDC_COMP_RISING	0
#define BLDC_COMP_FALLING	COMP_Input_Inverted
#endif

#define BLDC_TIM		TIM_22
#define BLDC_RLD		256

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

static void BLDC_EnterRunMode(Phase_t phase);
static void BLDC_ExitRunMode(void);
static void BLDC_CompIRQ(void);
static void BLDC_ConfigComp(Phase_t phase);

static void BLDC_Tick(void);

/*
 * PRIVATE VARIABLES
 */

static struct {
	BLDC_State_t state;
	uint8_t power;
	struct {
		uint32_t time;
		uint32_t steps;
		uint32_t freq;
	} start;
} gBLDC;

/*
 * PUBLIC FUNCTIONS
 */

void BLDC_Init(void)
{
	MP6532_Init();
	TIM_Init(BLDC_TIM, BLDC_RLD, BLDC_RLD);
	TIM_OnReload(BLDC_TIM, BLDC_Tick);

#ifdef BLDC_TEST_CH
	GPIO_EnableOutput(BLDC_TEST_GPIO, BLDC_TEST_PIN, GPIO_PIN_RESET);
	COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_TEST_CH);
#endif
}

uint32_t BLDC_GetRPM(void)
{
	return 0;
}

void BLDC_SetPower(uint8_t power)
{
	gBLDC.power = power;
	if (gBLDC.state >= BLDC_State_Starting)
	{
		MP6532_SetDuty(power);
	}
}

void BLDC_Start(uint32_t freq)
{
	if (gBLDC.state == BLDC_State_Idle)
	{
		gBLDC.start.time = CORE_GetTick();
		gBLDC.start.steps = 12;
		gBLDC.start.freq = freq;
		gBLDC.state = BLDC_State_Starting;
		MP6532_SetDuty(gBLDC.power);
		TIM_SetFreq(BLDC_TIM, BLDC_RLD * freq);
		TIM_Start(BLDC_TIM);
	}
}

void BLDC_Stop(void)
{
	TIM_Stop(BLDC_TIM);
#ifdef BLDC_EN_CLOSED_LOOP
	BLDC_ExitRunMode();
#endif
	MP6532_SetDuty(0);

	if (gBLDC.state != BLDC_State_Fault)
	{
		gBLDC.state = BLDC_State_Idle;
	}
}

BLDC_State_t BLDC_GetState(void)
{
	return gBLDC.state;
}

bool BLDC_Update(void)
{
	uint32_t now = CORE_GetTick();
	bool faulted = MP6532_IsFaulted();
	if (faulted)
	{
		switch (gBLDC.state)
		{
		case BLDC_State_Fault:
		case BLDC_State_Idle:
			break;
		case BLDC_State_Starting:
		case BLDC_State_Running:
			BLDC_Stop();
			break;
		}
		gBLDC.state = BLDC_State_Fault;
	}
	else
	{
		switch (gBLDC.state)
		{
		case BLDC_State_Fault:
			gBLDC.state = BLDC_State_Idle;
			break;
		case BLDC_State_Idle:
			break;
		case BLDC_State_Starting:
			break;
		case BLDC_State_Running:
			if (now - gBLDC.start.time > BLDC_STALL_MS)
			{
				// Stalled out.
				BLDC_Stop();
			}
			break;
		}
	}

#ifdef BLDC_TEST_CH
	for (uint32_t i = 0; i < 100; i++)
	{
		bool set = COMP_Read(BLDC_COMP);
		GPIO_Write(BLDC_TEST_GPIO, BLDC_TEST_PIN, set);
	}
	return false;
#else
	return true;
#endif
}

/*
 * PRIVATE FUNCTIONS
 */

static void BLDC_Tick(void)
{
	if (gBLDC.state == BLDC_State_Starting)
	{
		Phase_t phase = MP6532_Step();

		if (gBLDC.start.freq < BLDC_OPEN_LOOP_FREQ)
		{
			gBLDC.start.freq += BLDC_START_RAMP;
			TIM_SetFreq(BLDC_TIM, BLDC_RLD * gBLDC.start.freq);
		}
#ifdef BLDC_EN_CLOSED_LOOP
		else
		{
			gBLDC.state = BLDC_State_Running;
			BLDC_EnterRunMode(phase);
		}
#endif
	}
	else
	{
		TIM_Stop(BLDC_TIM);
	}
}

static void BLDC_EnterRunMode(Phase_t phase)
{
	gBLDC.start.time = CORE_GetTick();
	BLDC_ConfigComp(phase);
}

static void BLDC_ExitRunMode(void)
{
	COMP_Deinit(BLDC_COMP);
}

static void BLDC_CompIRQ(void)
{
	gBLDC.start.time = CORE_GetTick();
	COMP_Deinit(BLDC_COMP);
	Phase_t phase = MP6532_Step();
	BLDC_ConfigComp(phase);
}


// HALL | A B C
//------|------
//A     |   - +
//A B   | + -
//  B   | +   -
//  B C |   + -
//    C | - +
//A   C | -   +

static void BLDC_ConfigComp(Phase_t phase)
{
	switch (phase)
	{
	case Phase_A:  // C -> B
		COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_COMP_CHA | BLDC_COMP_FALLING);
		break;
	case Phase_AB: // A -> B
		COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_COMP_CHC | BLDC_COMP_RISING);
		break;
	case Phase_B:  // A -> C
		COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_COMP_CHB | BLDC_COMP_FALLING);
		break;
	case Phase_BC: // B -> C
		COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_COMP_CHA | BLDC_COMP_RISING);
		break;
	case Phase_C:  // B -> A
		COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_COMP_CHC | BLDC_COMP_FALLING);
		break;
	case Phase_CA: // C -> A
		COMP_Init(BLDC_COMP, BLDC_COMP_COM | BLDC_COMP_CHB | BLDC_COMP_RISING);
		break;
	}

	// Wait for comparator to settle.
	for (int i = 20; i > 0; i--)
	{
		if ( COMP_Read(BLDC_COMP) )
		{
			// Accelerate the count if we stable.
			i -= 5;
		}
		CORE_DelayUs(1);
	}

	COMP_OnChange(BLDC_COMP, GPIO_IT_Rising, BLDC_CompIRQ);
}

/*
 * INTERRUPT ROUTINES
 */

