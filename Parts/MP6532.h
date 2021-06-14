#ifndef MP6532_H
#define MP6532_H

#include "STM32X.h"

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

typedef enum {
	Phase_A,
	Phase_AB,
	Phase_B,
	Phase_BC,
	Phase_C,
	Phase_CA,
} Phase_t;

/*
 * PUBLIC FUNCTIONS
 */

void MP6532_Init(void);
void MP6532_Deinit(void);
bool MP6532_IsFaulted(void);

void MP6532_SetDuty(uint8_t duty);
Phase_t MP6532_Step(void);

/*
 * EXTERN DECLARATIONS
 */

#endif //MP6532_H
