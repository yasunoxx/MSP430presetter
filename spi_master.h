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

//******************************************************************************
// Example Commands ************************************************************
//******************************************************************************

#define DUMMY   0xFF

#define SLAVE_CS_OUT    P2OUT
#define SLAVE_CS_DIR    P2DIR
#define SLAVE_CS_PIN    BIT0

/* CMD_TYPE_X_SLAVE are example commands the master sends to the slave.
 * The slave will send example SlaveTypeX buffers in response.
 *
 * CMD_TYPE_X_MASTER are example commands the master sends to the slave.
 * The slave will initialize itself to receive MasterTypeX example buffers.
 * */

#define CMD_TYPE_0_SLAVE              0
#define CMD_TYPE_1_SLAVE              1
#define CMD_TYPE_2_SLAVE              2

#define CMD_TYPE_0_MASTER              3
#define CMD_TYPE_1_MASTER              4
#define CMD_TYPE_2_MASTER              5

#define TYPE_0_LENGTH              1
#define TYPE_1_LENGTH              2
#define TYPE_2_LENGTH              6

#define MAX_BUFFER_SIZE     20

/* MasterTypeX are example buffers initialized in the master, they will be
 * sent by the master to the slave.
 * SlaveTypeX are example buffers initialized in the slave, they will be
 * sent by the slave to the master.
 * */

extern uint8_t MasterType0 [TYPE_0_LENGTH];
extern uint8_t MasterType1 [TYPE_1_LENGTH];
extern uint8_t MasterType2 [TYPE_2_LENGTH];

extern uint8_t SlaveType2 [TYPE_2_LENGTH];
extern uint8_t SlaveType1 [TYPE_1_LENGTH];
extern uint8_t SlaveType0 [TYPE_0_LENGTH];

//******************************************************************************
// General SPI State Machine ***************************************************
//******************************************************************************

typedef enum SPI_ModeEnum{
    IDLE_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    TIMEOUT_MODE
} SPI_Mode;

/* Used to track the state of the software state machine*/
extern SPI_Mode MasterMode;

/* The Register Address/Command to use*/
extern uint8_t TransmitRegAddr;

/* ReceiveBuffer: Buffer used to receive data in the ISR
 * RXByteCtr: Number of bytes left to receive
 * ReceiveIndex: The index of the next byte to be received in ReceiveBuffer
 * TransmitBuffer: Buffer used to transmit data in the ISR
 * TXByteCtr: Number of bytes left to transfer
 * TransmitIndex: The index of the next byte to be transmitted in TransmitBuffer
 * */
extern uint8_t ReceiveBuffer[MAX_BUFFER_SIZE];
extern uint8_t RXByteCtr;
extern uint8_t ReceiveIndex;
extern uint8_t TransmitBuffer[MAX_BUFFER_SIZE];
extern uint8_t TXByteCtr;
extern uint8_t TransmitIndex;

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
extern SPI_Mode SPI_Master_WriteReg(uint8_t reg_addr, uint8_t *reg_data, uint8_t count);

/* For slave device, read the data specified in slaves reg_addr.
 * The received data is available in ReceiveBuffer
 *
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_SLAVE
 * count: The length of data to read
 *           Example: TYPE_0_LENGTH
 *  */
extern SPI_Mode SPI_Master_ReadReg(uint8_t reg_addr, uint8_t count);
extern void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count);
extern void SendUCB0Data(uint8_t val);

extern void InitClockTo16MHz( void );
extern void InitSPI_GPIO( void );
extern void InitSPI( void );
