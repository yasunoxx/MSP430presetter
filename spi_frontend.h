/*
 * spi_frontend.h -- MSP430, and MCP23S17 SPI frontend
 * by yasunoxx
 * ### Use mspgcc(4.6.3 or later) only !!! ###
 */

extern SPI_Mode SPI_WriteReg( uint8_t chip_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count );
extern SPI_Mode SPI_ReadReg( uint8_t chip_addr, uint8_t reg_addr, uint8_t count );
extern void InitSPI_FrontEnd_GPIO( void );

// MCP23S17 Configuration
#define DEVADDR_TARGET1_WRITE 0x4a
#define DEVADDR_TARGET1_READ ( DEVADDR_TARGET1_WRITE | 0x01 )

#define INIT1_LEN 2
extern uint8_t MCP23S17_Init1[ INIT1_LEN ];
#define INIT2_LEN 3
extern uint8_t MCP23S17_Init2[ INIT2_LEN ];
#define INIT3_LEN 3
extern uint8_t MCP23S17_Init3[ INIT3_LEN ];
