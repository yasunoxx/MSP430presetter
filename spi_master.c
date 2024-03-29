//******************************************************************************
//   MSP430G2xx3 Demo - USCI_B0, SPI 3-Wire Master multiple byte RX/TX
//
//   Description: SPI master communicates to SPI slave sending and receiving
//   3 different messages of different length. SPI master will enter LPM0 mode
//   while waiting for the messages to be sent/receiving using SPI interrupt.
//   SPI Master will initially wait for a port interrupt in LPM0 mode before
//   starting the SPI communication.
//   ACLK = NA, MCLK = SMCLK = DCO 16MHz.
//
//   Nima Eskandari
//   Texas Instruments Inc.
//   April 2017
//   Built with CCS V7.0
//
// and Modified by yasunoxx 2021/2022
//
//                   MSP430G2553
//                 -----------------
//            /|\ |                 |
//             |  |                 |
//             ---|RST          *RST|-> Slave Reset (*RST)
//                |                 |
//                |             P1.7|-> Data Out (UCB0SIMO)
//                |                 |
//       Button ->|P1.3         P1.6|<- Data In (UCB0SOMI)
//                |                 |
//                |             P1.5|-> Serial Clock Out (UCB0CLK)
//
//******************************************************************************

#include <msp430g2553.h>
#include <stdint.h>
#include "spi_master.h"

/* MasterTypeX are example buffers initialized in the master, they will be
 * sent by the master to the slave.
 * SlaveTypeX are example buffers initialized in the slave, they will be
 * sent by the slave to the master.
 * */

uint8_t MasterType0 [TYPE_0_LENGTH] = {0x11};
uint8_t MasterType1 [TYPE_1_LENGTH] = {8, 9};
uint8_t MasterType2 [TYPE_2_LENGTH] = {'F', '4' , '1' , '9', '2', 'B'};

uint8_t SlaveType2 [TYPE_2_LENGTH] = {0};
uint8_t SlaveType1 [TYPE_1_LENGTH] = {0};
uint8_t SlaveType0 [TYPE_0_LENGTH] = {0};

//******************************************************************************
// General SPI State Machine ***************************************************
//******************************************************************************

// typedef enum SPI_ModeEnum{
//    IDLE_MODE,
//    TX_REG_ADDRESS_MODE,
//    RX_REG_ADDRESS_MODE,
//    TX_DATA_MODE,
//    RX_DATA_MODE,
//    TIMEOUT_MODE
// } SPI_Mode;

/* Used to track the state of the software state machine*/
SPI_Mode MasterMode = IDLE_MODE;

/* The Register Address/Command to use*/
uint8_t TransmitRegAddr = 0;

/* ReceiveBuffer: Buffer used to receive data in the ISR
 * RXByteCtr: Number of bytes left to receive
 * ReceiveIndex: The index of the next byte to be received in ReceiveBuffer
 * TransmitBuffer: Buffer used to transmit data in the ISR
 * TXByteCtr: Number of bytes left to transfer
 * TransmitIndex: The index of the next byte to be transmitted in TransmitBuffer
 * */
uint8_t ReceiveBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t RXByteCtr = 0;
uint8_t ReceiveIndex = 0;
uint8_t TransmitBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t TXByteCtr = 0;
uint8_t TransmitIndex = 0;

/* SPI Write and Read Functions */

/* For slave device, writes the data specified in *reg_data
 *
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_MASTER
 * *reg_data: The buffer to write
 *           Example: MasterType0
 * count: The length of *reg_data
 *           Example: TYPE_0_LENGTH
 *  */
SPI_Mode SPI_Master_WriteReg(uint8_t reg_addr, uint8_t *reg_data, uint8_t count);

/* For slave device, read the data specified in slaves reg_addr.
 * The received data is available in ReceiveBuffer
 *
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_SLAVE
 * count: The length of data to read
 *           Example: TYPE_0_LENGTH
 *  */
SPI_Mode SPI_Master_ReadReg(uint8_t reg_addr, uint8_t count);
void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count);
void SendUCB0Data(uint8_t val);

void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count)
{
    uint8_t copyIndex = 0;
    for (copyIndex = 0; copyIndex < count; copyIndex++)
    {
        dest[copyIndex] = source[copyIndex];
    }
}


SPI_Mode SPI_Master_WriteReg(uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;

    //Copy register data to TransmitBuffer
    CopyArray(reg_data, TransmitBuffer, count);

    TXByteCtr = count;
    RXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

//    SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
    SendUCB0Data(TransmitRegAddr);

    __bis_SR_register(CPUOFF + GIE);              // Enter LPM0 w/ interrupts

//    SLAVE_CS_OUT |= SLAVE_CS_PIN;
    return MasterMode;
}

SPI_Mode SPI_Master_ReadReg(uint8_t reg_addr, uint8_t count)
{
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;
    RXByteCtr = count;
    TXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

//    SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
    SendUCB0Data(TransmitRegAddr);

    __bis_SR_register(CPUOFF + GIE);              // Enter LPM0 w/ interrupts

//    SLAVE_CS_OUT |= SLAVE_CS_PIN;
    return MasterMode;
}

void SendUCB0Data(uint8_t val)
{
    while (!(IFG2 & UCB0TXIFG));              // USCI_A0 TX buffer ready?
    UCB0TXBUF = val;
}

//******************************************************************************
// Device Initialization *******************************************************
//******************************************************************************

void InitClockTo16MHz()
{
    if (CALBC1_16MHZ==0xFF)                  // If calibration constant erased
    {
        while(1);                               // do not load, trap CPU!!
    }
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_16MHZ;                    // Set DCO
    DCOCTL = CALDCO_16MHZ;
}

void InitSPI_GPIO()
{
  //LEDs
//  P1OUT = 0x00;                             // P1 setup for LED & reset output
//  P1DIR |= BIT0 | BIT5 | BIT6;

  //SPI Pins
  P1SEL |= BIT5 | BIT6 | BIT7;
  P1SEL2 |= BIT5 | BIT6 | BIT7;

  P1DIR |= BIT5 | BIT7;

#ifdef IGNOREIT
  //Button to initiate transfer
  P1DIR &= ~(BIT3);
  P1OUT |= BIT3;                            // P1.3 pull up
  P1REN |= BIT3;                            // P1.3 pull up/down resistor enable
  P1IE  |= BIT3;                            // P1.3 interrupt enabled
  P1IES |= BIT3;                            // P1.3 Hi/lo edge
  P1IFG &= ~BIT3;                           // P1.3 IFG cleared
#endif // IGNOREIT
}

void InitSPI()
{
  //Clock Polarity: The inactive state is high
  //MSB First, 8-bit, Master, 3-pin mode, Synchronous
  UCB0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;
  UCB0CTL1 |= UCSSEL_2;                     // SMCLK
  UCB0BR0 |= 0x20;                          // /2
  UCB0BR1 = 0;                              //
  UCA0MCTL = 0;                             // No modulation must be cleared for SPI
  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  IE2 |= UCB0RXIE;                          // Enable USCI0 RX interrupt

//  SLAVE_CS_DIR |= SLAVE_CS_PIN;
//  SLAVE_CS_OUT |= SLAVE_CS_PIN;

}

//******************************************************************************
// Main ************************************************************************
// Send and receive three messages containing the example commands *************
//******************************************************************************

#ifdef DEBUG
int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer

  initClockTo16MHz();
  initGPIO();
  initSPI();

  P1OUT &= ~BIT5;                           // Now with SPI signals initialized,
  __delay_cycles(100000);
  P1OUT |= BIT5;                            // reset slave
  __delay_cycles(100000);                   // Wait for slave to initialize

  P1OUT |= BIT0;

  __bis_SR_register(LPM0_bits + GIE);       // CPU off, enable interrupts

  SPI_Master_ReadReg(CMD_TYPE_2_SLAVE, TYPE_2_LENGTH);
  CopyArray(ReceiveBuffer, SlaveType2, TYPE_2_LENGTH);

  SPI_Master_ReadReg(CMD_TYPE_1_SLAVE, TYPE_1_LENGTH);
  CopyArray(ReceiveBuffer, SlaveType1, TYPE_1_LENGTH);

  SPI_Master_ReadReg(CMD_TYPE_0_SLAVE, TYPE_0_LENGTH);
  CopyArray(ReceiveBuffer, SlaveType0, TYPE_0_LENGTH);

  SPI_Master_WriteReg(CMD_TYPE_2_MASTER, MasterType2, TYPE_2_LENGTH);
  SPI_Master_WriteReg(CMD_TYPE_1_MASTER, MasterType1, TYPE_1_LENGTH);
  SPI_Master_WriteReg(CMD_TYPE_0_MASTER, MasterType0, TYPE_0_LENGTH);
  __bis_SR_register(LPM0_bits + GIE);
}
#endif // DEBUG

//******************************************************************************
// SPI Interrupt ***************************************************************
//******************************************************************************

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIB0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCIB0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if (IFG2 & UCB0RXIFG)
    {
        uint8_t ucb0_rx_val = UCB0RXBUF;
        switch (MasterMode)
        {
            case TX_REG_ADDRESS_MODE:
                if (RXByteCtr)
                {
                    MasterMode = RX_DATA_MODE;   // Need to start receiving now
                    //Send Dummy To Start
                    __delay_cycles(2000);
                    SendUCB0Data(DUMMY);
                }
                else
                {
                    MasterMode = TX_DATA_MODE;        // Continue to transmision with the data in Transmit Buffer
                    //Send First
                    SendUCB0Data(TransmitBuffer[TransmitIndex++]);
                    TXByteCtr--;
                }
                break;

            case TX_DATA_MODE:
                if (TXByteCtr)
                {
                  SendUCB0Data(TransmitBuffer[TransmitIndex++]);
                  TXByteCtr--;
                }
                else
                {
                  //Done with transmission
                  MasterMode = IDLE_MODE;
                  __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
                }
                break;

            case RX_DATA_MODE:
                if (RXByteCtr)
                {
                    ReceiveBuffer[ReceiveIndex++] = ucb0_rx_val;
                    //Transmit a dummy
                    RXByteCtr--;
                }
                if (RXByteCtr == 0)
                {
                    MasterMode = IDLE_MODE;
                    __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
                }
                else
                {
                    SendUCB0Data(DUMMY);
                }
                break;

            default:
                __no_operation();
                break;
        }
        __delay_cycles(50);
    }
}

#ifdef IGNOREIT
//******************************************************************************
// PORT1 Interrupt *************************************************************
// Interrupt occurs on button press and initiates the SPI data transfer ********
//******************************************************************************

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
#else
#error Compiler not supported!
#endif
{
//    P1OUT |= BIT6;
    P1IFG &= ~BIT3;                           // P1.3 IFG cleared
    P1IE &= ~BIT3;
    //Initiate
    __bic_SR_register_on_exit(LPM0_bits);      // Exit LPM0
}
#endif // IGNORET
