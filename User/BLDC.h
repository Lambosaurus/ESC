#ifndef BLCD_H
#define BLDC_H

#include "STM32X.h"

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

typedef enum {
	BLDC_State_Fault,
	BLDC_State_Idle,
	BLDC_State_Starting,
	BLDC_State_Running,
} BLDC_State_t;

/*
 * PUBLIC FUNCTIONS
 */

void BLDC_Init(void);
uint32_t BLDC_GetRPM(void);
void BLDC_SetPower(uint8_t power);
void BLDC_Start(uint32_t freq);
void BLDC_Stop(void);
BLDC_State_t BLDC_GetState(void);
void BLDC_Update(void);

/*
 * EXTERN DECLARATIONS
 */

#endif //BLDC_H
