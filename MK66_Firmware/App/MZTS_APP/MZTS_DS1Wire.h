#ifndef _DS1WIRE
  #define _DS1WIRE

  #define       MAX_DEVICES                   2

  #define       D1W_BUS                       0x10
  #define       D1W_DDR                       DDRD
  #define       D1W_PORT                      PORTD
  #define       D1W_PIN                       PIND

  #define       D1W_RELEASE_BUS               WTX_LOW()
  #define       D1W_PULL_BUS_LOW              WTX_HIGH()
  #define       D1W_LINE_STATE                WRX_state()

  #define       D1W_ROM_READ                  0x33    //!< READ ROM command code.
  #define       D1W_ROM_SKIP                  0xCC    //!< SKIP ROM command code.
  #define       D1W_ROM_MATCH                 0x55    //!< MATCH ROM command code.
  #define       D1W_ROM_SEARCH                0xF0    //!< SEARCH ROM command code.

  #define       D1W_ROM_SEARCH_FINISHED       0x00    //!< Search finished return code.
  #define       D1W_ROM_SEARCH_FAILED         0xFF    //!< Search failed return code.

  #define       D1W_DELAY_OFFSET_CYCLES       13   //!< Timing delay when pulling bus low and releasing bus.

// Bit timing delays in us
  #define       D1W_DELAY_A_STD_MODE_6        6
  #define       D1W_DELAY_B_STD_MODE_64       64
  #define       D1W_DELAY_C_STD_MODE_60       60
  #define       D1W_DELAY_D_STD_MODE_10       10
  #define       D1W_DELAY_E_STD_MODE_9        9
  #define       D1W_DELAY_F_STD_MODE_55       55
  #define       D1W_DELAY_H_STD_MODE_480      480
  #define       D1W_DELAY_I_STD_MODE_70       70
  #define       D1W_DELAY_J_STD_MODE_410      410


// Defines used only in code example.
  #define       DS18B20_FAMILY_ID             0x28
  #define       DS1820_FAMILY_ID              0x10
  #define       DS1820_START_CONVERSION       0x44
  #define       DS1820_READ_SCRATCHPAD        0xBE
  #define       DS1820_ERROR                  -1000   // Return code. Outside temperature range.

  #define       DS2890_FAMILY_ID              0x2C
  #define       DS2890_WRITE_CONTROL_REGISTER 0X55
  #define       DS2890_RELEASE_CODE           0x96
  #define       DS2890_WRITE_POSITION         0x0F

  #define       SEARCH_SUCCESSFUL             0x00
  #define       SEARCH_CRC_ERROR              0x01
  #define       SEARCH_FAILED                 0x02

  #define       D1W_CRC_OK                    1
  #define       D1W_CRC_ERROR                 0


/*! \brief  Data type used to hold information about slave devices.
 *
 *  The OWI_device data type holds information about what bus each device
 *  is connected to, and its 64 bit identifier.
 */
typedef struct
{
    uint8_t id_arr[8];    //!< The 64 bit identifier.
} D1W_device;


void          OneWire_task_create(void);

int32_t       DS1820_ReadTemperature(uint8_t *rom_id, float *t);
void          DS2890_SetWiperPosition(uint8_t position, uint8_t *id);
uint8_t       DS1W_SearchBuses(D1W_device *devices, uint8_t max_devices, uint32_t *devices_num);
D1W_device*   DS1W_FindFamily(uint8_t familyID, D1W_device *devices, uint8_t size);

void          D1W_SendByte(uint8_t data);
uint8_t       D1W_ReceiveByte(void);
void          D1W_SkipRom(void);
void          D1W_ReadRom(uint8_t *romValue);
void          D1W_MatchRom(uint8_t *romValue);
uint8_t       D1W_SearchRom(uint8_t *bitPattern, uint8_t lastDeviation);
uint8_t       D1W_ComputeCRC8(uint8_t inData, uint8_t seed);
uint8_t       D1W_CheckRomCRC(uint8_t *romValue);


uint32_t      D1W_WriteBit1(void);
uint32_t      D1W_WriteBit0(void);
uint8_t       D1W_ReadBit(void);
uint32_t      D1W_DetectPresence(void);

D1W_device* D1W_Get_dev(uint32_t n);


#endif
