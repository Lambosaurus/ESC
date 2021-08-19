
#include "BLDC.h"
#include "MP6532.h"
#include "Core.h"
#include "COMP.h"

/*
 * PRIVATE DEFINITIONS
 */

#define BLDC_COMP		COMP_2
#define BLDC_COMP_CHA	(COMP_Pos_IO2 | COMP_Neg_IO2)
#define BLDC_COMP_CHB	(COMP_Pos_IO5 | COMP_Neg_IO2)
#define BLDC_COMP_CHC	(COMP_Pos_IO4 | COMP_Neg_IO2)

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

/*
 * PRIVATE VARIABLES
 */

static struct {
	BLDC_State_t state;
	uint8_t power;
	struct {
		uint32_t time;
		uint32_t period;
		uint32_t steps;
	} start;
} gBLDC;

/*
 * PUBLIC FUNCTIONS
 */

void BLDC_Init(void)
{
	MP6532_Init();
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
		gBLDC.start.period = 6000 / freq;
		gBLDC.start.steps = 12;
		gBLDC.state = BLDC_State_Starting;
		MP6532_SetDuty(gBLDC.power);
	}
}

void BLDC_Stop(void)
{
	if (gBLDC.state != BLDC_State_Fault)
	{
		gBLDC.state = BLDC_State_Idle;
	}
	BLDC_ExitRunMode();
	MP6532_SetDuty(0);
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
			if (now - gBLDC.start.time >= gBLDC.start.period)
			{
				gBLDC.start.time = now;
				Phase_t phase = MP6532_Step();

				gBLDC.start.period -= 1;
				if (gBLDC.start.period == 0)
				{
					gBLDC.state = BLDC_State_Running;
					BLDC_EnterRunMode(phase);
				}
			}
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
	Phase_t phase = MP6532_Step();
	BLDC_ConfigComp(phase);
}

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
	for (int i = 20; i > 0; i--)
	{
		if ( !COMP_Read(BLDC_COMP) )
		{
			// Accelerate the count if we stable.
			i -= 2;
		}
	}
	*/

	CORE_DelayUs(10);

	COMP_OnChange(BLDC_COMP, GPIO_IT_Rising, BLDC_CompIRQ);
}

/*
 * INTERRUPT ROUTINES
 */

