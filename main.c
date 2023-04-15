/*
 * main.c -- MSP430 PSG/OPLL Register Presetter
 * by yasunoxx
 * ### Use TI-MSPGCC6.1.1.0 or above version ###
 */

/* target type are define in Makefile */

// Original Copyright:
/*
 * main.c
 *
 * MSP-EXP430G2-LaunchPad User Experience Application
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*
 *   1. System timer 5[msec] at 12[MHz], not WDT
 *   2. peripheral initialize
 *   3. main loop
 *   4. signal handling
 */

#include <msp430g2553.h>
#include <stdint.h>
#include "io.h"
#include "tlv.h"
#include "spi_master.h"
#include "spi_frontend.h"
#include "uart_115k.h"

const char *FW_Version = "0X01";

#ifdef TARGET_XT1
#define TIMER_PWM_PERIOD 22
// 21.97 = 60 * 12000[Hz] / 32768[Hz]
#define TIMER_PWM_CENTER 11
// 10.98 = 30 * 12000[Hz] / 32768[Hz]
#else
#define	TIMER_PWM_PERIOD 60
#endif
#define TIMER_PWM_PERIOD_DCO 60000

void ConfigureTimerPwm( void );
void PreApplicationMode( void );
void InitializeClocks( void );
void initializeDCO( void );
void InitializeLeds( void );

#define SYSTIMER_COUNTUP	100
volatile unsigned int SysTimer_Counter;
#define SYSTIMER2_COUNTUP	10
volatile unsigned int SysTimer2_Counter;
#define Low 0
#define High 1
volatile unsigned char ClockMode;

#define	SYSTIMER_FLIP_OFF	0
#define	SYSTIMER_FLIP_ON	1
volatile unsigned char F_SysTimer_Flipper;
#define	SYSTIMER2_FLIP_OFF	0
#define	SYSTIMER2_FLIP_ON	1
volatile unsigned char F_SysTimer2_Flipper;
volatile unsigned short London;

int main( void )
{
	WDTCTL = WDTPW + WDTHOLD;  // Stop WDT

	InitializeClocks();
	InitializeButton();
	InitializeLeds();
	PreApplicationMode();
	ConfigureTimerPwm();

	__enable_interrupt();
	ConfigureAdcTempSensor();
	GetTLV();
	__disable_interrupt();
	initializeDCO(); // on this file
	ConfigureTimerPwm();
	__enable_interrupt();

  InitSPI_GPIO();
	InitSPI_FrontEnd_GPIO();
	InitSPI();

	InitClockTo16MHz();
	InitUart();

	LED_OUT |= LED1;  // Runup

	SPI_WriteReg( 3, DEVADDR_TARGET1_WRITE, MCP23S17_Init1, INIT1_LEN );
	SPI_WriteReg( 1, DEVADDR_TARGET1_WRITE, MCP23S17_Init1, INIT1_LEN );
	SPI_WriteReg( 1, DEVADDR_TARGET1_WRITE, MCP23S17_Init2, INIT2_LEN );
	SPI_WriteReg( 1, DEVADDR_TARGET1_WRITE, MCP23S17_Init3, INIT3_LEN );
	SPI_ReadReg( 1, DEVADDR_TARGET1_READ, TYPE_2_LENGTH );
  CopyArray(ReceiveBuffer, SlaveType2, TYPE_2_LENGTH );

	/* Main Application Loop */
	while( 1 )
	{
		// demonstraation: London
		if( F_SysTimer2_Flipper == SYSTIMER2_FLIP_ON )
	  {
	    F_SysTimer2_Flipper = SYSTIMER2_FLIP_OFF;
			{
				unsigned char f_carry = Low;
				unsigned char london_data[ 2 ];

				if( ( London & 0x8000 ) != 0 )
				{
					f_carry = High;
				}
				London <<= 1;
				if( f_carry == High )
				{
					London |= 0x0001;
				}
				london_data[ 0 ] = ( ( London & 0x0FF00 ) >> 8 );
				london_data[ 1 ] = ( London & 0x00FF );
				SPI_WriteReg( 3, 0x00, london_data, 2 );
			}
	  }
	}
}


void PreApplicationMode( void )
{
	SysTimer_Counter = 0;
  F_SysTimer_Flipper = SYSTIMER_FLIP_OFF;
	SysTimer2_Counter = 0;
	F_SysTimer2_Flipper = SYSTIMER2_FLIP_OFF;
  ClockMode = Low;

	London = 0x8888;
//  __bis_SR_register( LPM3_bits + GIE );  // LPM0 with
//  __bis_SR_register( LPM3_bits );
}

void ConfigureTimerPwm( void )
{
  if( ClockMode == Low )
  {
    TACCR0 = TIMER_PWM_PERIOD;			// Compare Maxim value
#ifdef TARGET_XT1
    TACTL = TASSEL_2 | MC_1;				// TACLK = SMCLK, Up mode.
#else
    TACTL = TASSEL_2 | MC_1;				// TACLK = ACLK, Up mode.
#endif
    TACCTL0 = CCIE;									// TACCTL0 output OUT bit(not used)
    TACCTL1 = CCIE + OUTMOD_3;			// TACCTL1 Capture Compare, Set/reset
  }
  else // ClockMode Low to High
  {
    TACCR0 = TIMER_PWM_PERIOD_DCO;	// Compare Maxim value
		TACTL = TASSEL_2 | MC_1;				// TACLK = SMCLK, Up mode.
		TACCTL0 = CCIE;									// TACCTL0 output OUT bit(not used)
		TACCTL1 = CCIE + OUTMOD_3;			// TACCTL1 Capture Compare, Set/reset
	}
}

void InitializeClocks( void )
{
  /* FIXME: look LFXT1OF, and choose one */
#ifdef TARGET_XT1
  {
    // see F2xx manual section 5.2.7.1
    unsigned char loop;

//    __bic_SR_register( OSCOFF ); // Oscillator ON
    P2DIR &= ~( BIT6 );
		P2DIR |= BIT7;
		P2SEL |= BIT7 | BIT6;
		P2SEL2 |= BIT7 | BIT6;

  	BCSCTL1 &= ~( XTS ); // Set low frequencty mode
//  BCSCTL3 = LFXT1S_0; // Use LFXT1(32768Hz type)
		BCSCTL3 |= XT2S_3; // Use LFXT1(external High OSC type)
		BCSCTL3 |= LFXT1S_3; // Use LFXT1(external High OSC type)
		BCSCTL3 |= XCAP_3; // Use LFXT1(external High OSC type)
    while( 1 )
    {
      IFG1 &= ~( OFIFG );
      for( loop = 0; loop < 254; loop++ );
      if( ( IFG1 & OFIFG ) == 0 ) break;
    }
    BCSCTL2 |= SELM_3;
  }
#else
  DCOCTL = 0;
  BCSCTL1 = 0;
  BCSCTL1 = XT2OFF | DIVA_0 | RSEL0;
                          // Set ACLK / 1
  BCSCTL2 = 0;
  BCSCTL2 = SELM_3 | DIVM_0 | SELS | DIVS_0;
                          // SMCLK = MCLK = ACLK = 12kHz
  BCSCTL3 = 0;
  BCSCTL3 = LFXT1S_2; // use VLO
#endif
}

void initializeDCO( void )
{
  DCOCTL = Var_CALDCO_12MHz;
  DCOCTL |= DCO2;
  BCSCTL1 = Var_CALBC1_12MHz;
  BCSCTL1 |= DIVA_0;
  BCSCTL2 &= ~( DIVS_3 );
  BCSCTL2 = SELM_0 | DIVM_0; // DCO = MCLK = SMCLK = ACLK(12MHZ, VLO * 1000)
  TACTL = TASSEL_2 | MC_2 | TACLR;
      // Select SMCKL as source, no divider, Continuous mode and reset timer
  BCSCTL1 |= XT2OFF; // XT2 off, and Set ACLK to /1 divider

  ClockMode = High;
}
