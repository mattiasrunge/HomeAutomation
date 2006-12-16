#ifndef MCU_H_
#define MCU_H_


/*-----------------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------------*/
#include <mcu_cfg.h>
#include <avr/interrupt.h>


/*-----------------------------------------------------------------------------
 * Defines
 *---------------------------------------------------------------------------*/
/* supported MCUs */
#define MCU_ATMEGA8		8


/*-----------------------------------------------------------------------------
 * Public Function Prototypes
 *---------------------------------------------------------------------------*/
void Mcu_EnableIRQ(void);
void Mcu_DisableIRQ(void);


#endif /*MCU_H_*/