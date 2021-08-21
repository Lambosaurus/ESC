
#include "BLDC.h"
#include "MP6532.h"
#include "Core.h"
#include "COMP.h"
#include "TIM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define BLDC_COMP		COMP_2
#define BLDC_COMP_CHA	(COMP_Pos_IO2 | COMP_Neg_IO2)
#define BLDC_COMP_CHB	(COMP_Pos_IO5 | COMP_Neg_IO2)
#define BLDC_COMP_CHC	(COMP_Pos_IO4 | COMP_Neg_IO2)

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
	uint32_t comps;
	uint32_t delay;
} gBLDC;

/*
 * PUBLIC FUNCTIONS
 */

void BLDC_Init(void)
{
	MP6532_Init();
	TIM_Init(BLDC_TIM, BLDC_RLD, BLDC_RLD);
	TIM_OnReload(BLDC_TIM, BLDC_Tick);
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
	BLDC_ExitRunMode();
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

void BLDC_Update(void)
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
			if (now - gBLDC.start.time > 10)
			{
				// Stalled out.
				BLDC_Stop();
			}
			break;
		}
	}
}

/*
 * PRIVATE FUNCTIONS
 */

static void BLDC_Tick(void)
{
	if (gBLDC.state == BLDC_State_Starting)
	{
		COMP_Deinit(BLDC_COMP);
		Phase_t phase = MP6532_Step();
		BLDC_ConfigComp(phase);
		if (gBLDC.start.freq < 1000)
		{
			gBLDC.start.freq += 1;
			TIM_SetFreq(BLDC_TIM, BLDC_RLD * gBLDC.start.freq);
		}
		//else
		//{
		//	gBLDC.state = BLDC_State_Running;
		//	BLDC_EnterRunMode(phase);
		//}
	}
	else
	{
		TIM_Stop(BLDC_TIM);
	}
}

static void BLDC_EnterRunMode(Phase_t phase)
{
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
	//Phase_t phase = MP6532_Step();
	//BLDC_ConfigComp(phase);
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
		COMP_Init(BLDC_COMP, BLDC_COMP_CHA | COMP_Input_Inverted);
		break;
	case Phase_AB: // A -> B
		COMP_Init(BLDC_COMP, BLDC_COMP_CHC);
		break;
	case Phase_B:  // A -> C
		COMP_Init(BLDC_COMP, BLDC_COMP_CHB | COMP_Input_Inverted);
		break;
	case Phase_BC: // B -> C
		COMP_Init(BLDC_COMP, BLDC_COMP_CHA);
		break;
	case Phase_C:  // B -> A
		COMP_Init(BLDC_COMP, BLDC_COMP_CHC | COMP_Input_Inverted);
		break;
	case Phase_CA: // C -> A
		COMP_Init(BLDC_COMP, BLDC_COMP_CHB);
		break;
	}

	/*
	// Wait for comparator to settle.
	/*
	for (int i = 10; i > 0; i--)
	{
		if ( COMP_Read(BLDC_COMP) )
		{
			// Accelerate the count if we stable.
			i -= 2;
		}
	}
	*/

	CORE_DelayUs(5);

	COMP_OnChange(BLDC_COMP, GPIO_IT_Rising, BLDC_CompIRQ);
}

/*
 * INTERRUPT ROUTINES
 */

