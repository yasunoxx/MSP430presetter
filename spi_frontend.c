/*
 * spi_frontend.c -- MSP430, and MCP23S17 SPI frontend
 * by yasunoxx
 * ### Use mspgcc(4.6.3 or later) only !!! ###
 */

#include <msp430g2553.h>
#include <stdint.h>
#include "spi_master.h"
#include "spi_frontend.h"

// MCP23S17 Configuration
uint8_t MCP23S17_Init1[ INIT1_LEN ] = { 0x05, 0x28 };
// IOCON |BANK  |MIRROR|SEQOP |DISSLW|HAEN  |ODR   |INTPOL|(null)
//       |     0|     0|     1|     0|     1|     0|     0|     0
uint8_t MCP23S17_Init2[ INIT2_LEN ] = { 0x00, 0x0f0, 0x00 };
// IODIRA|     1|     1|     1|     1|     0|     0|     0|     0
// IODIRB|     0|     0|     0|     0|     0|     0|     0|     0
uint8_t MCP23S17_Init3[ INIT3_LEN ] = { 0x12, 0x08, 0x80 };
// GPIOA |     0|     0|     0|     0|     0|     0|     0|     0
// GPIOB |     0|     0|     0|     0|     0|     0|     0|     0

SPI_Mode SPI_WriteReg( uint8_t chip_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count )
{
  P2OUT |= chip_addr & 0x07;
  SPI_Master_WriteReg( reg_addr, reg_data, count );
  P2OUT &= 0x0F8;
  return MasterMode;
}

SPI_Mode SPI_ReadReg( uint8_t chip_addr, uint8_t reg_addr, uint8_t count )
{
  P2OUT |= chip_addr & 0x07;
  SPI_Master_ReadReg( reg_addr, count );
  P2OUT &= 0x0F8;
  return MasterMode;
}

void InitSPI_FrontEnd_GPIO( void )
{
  P2DIR |= BIT0 | BIT1 | BIT2;
  P2OUT &= 0x0F8;
}
