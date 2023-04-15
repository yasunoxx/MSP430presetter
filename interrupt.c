// interrupt.c -- Interrupt Service Routuies
// (C)2022 yasunoxx
#ifdef GCC_VERSION_463
#include <legacymsp430.h>
#else
#include <msp430g2553.h>
#endif
#include <stdint.h>
#include "io.h"

#define SYSTIMER_COUNTUP	100
extern volatile unsigned int SysTimer_Counter;
#define SYSTIMER2_COUNTUP	10
extern volatile unsigned int SysTimer2_Counter;
#define Low 0
#define High 1
extern volatile unsigned char ClockMode;

#define	SYSTIMER_FLIP_OFF	0
#define	SYSTIMER_FLIP_ON	1
extern volatile unsigned char F_SysTimer_Flipper;
#define	SYSTIMER2_FLIP_OFF	0
#define	SYSTIMER2_FLIP_ON	1
extern volatile unsigned char F_SysTimer2_Flipper;


void __attribute__ (( interrupt TIMER0_A0_VECTOR )) TimerA0_ISR( void )
{
  TACCTL0 &= ~CCIFG;
}

void __attribute__ (( interrupt TIMER0_A1_VECTOR )) TimerA1_ISR( void )
{
  SysTimer_Counter++;
  if( SysTimer_Counter >= SYSTIMER_COUNTUP )
  {
    if( F_SysTimer_Flipper == SYSTIMER_FLIP_OFF )
    {
      F_SysTimer_Flipper = SYSTIMER_FLIP_ON;
      LED_OUT |= LED1;
    }
    else
    {
      F_SysTimer_Flipper = SYSTIMER_FLIP_OFF;
      LED_OUT &= ~LED1;
    }
    SysTimer_Counter = 0;
  }

  SysTimer2_Counter++;
  if( SysTimer2_Counter >= SYSTIMER2_COUNTUP )
  {
    F_SysTimer2_Flipper = SYSTIMER2_FLIP_ON;
    SysTimer2_Counter = 0;
  }

  TACCTL1 &= ~CCIFG;
}

// ADC10 interrupt service routine
void __attribute__ (( interrupt WDT_VECTOR )) WDT_ISR( void )
{
  IE1 &= ~WDTIE;  /* disable interrupt */
  IFG1 &= ~WDTIFG;  /* clear interrupt flag */
  WDTCTL = WDTPW + WDTHOLD;  /* put WDT back in hold state */
}
