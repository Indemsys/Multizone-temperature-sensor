#include "App.h"


#define   ONE_WIRE_RTX_PORT GPIOB
#define   ONE_WIRE_TX_PIN   6

D1W_device d1w_devices[MAX_DEVICES];



uint32_t one_wire_tx_err_cnt;
uint32_t one_wire_rx_err_cnt;
uint32_t one_wire_presense_cnt;
uint8_t  one_wire_recv_buf[2];
uint32_t one_wire_baud_rate;

static _task_id     onewire_task_id;
static uint8_t      curr_id[8];
static uint8_t      prev_id[8];
static TIME_STRUCT  last_id_time_stump;
static uint32_t     same_id_cnt;


static void Task_1WIRE(uint32_t initial_data);

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void OneWire_task_create(void)
{
  _mqx_uint res;


  TASK_TEMPLATE_STRUCT  task_template = {0};
  task_template.TASK_NAME          = "1WIRE";
  task_template.TASK_PRIORITY      = ONEWIRE_PRIO;
  task_template.TASK_STACKSIZE     = 1024;
  task_template.TASK_ADDRESS       = Task_1WIRE;
  task_template.TASK_ATTRIBUTES    = MQX_FLOATING_POINT_TASK;
  task_template.CREATION_PARAMETER = 0;
  onewire_task_id =  _task_create(0, 0, (uint32_t)&task_template);


}

/*-----------------------------------------------------------------------------------------------------


  \param id_arr

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Check_for_zero(uint8_t *id_arr)
{
  uint32_t sum = 0;
  for (uint32_t i=0; i < 8; i++) sum += id_arr[i];
  return sum;
}

/*-----------------------------------------------------------------------------------------------------


  \param id_arr
-----------------------------------------------------------------------------------------------------*/
static void _Log_1Wire_id(uint8_t *id_arr)
{
  LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT,"1-WIRE DEV ID: %02X %02X %02X %02X %02X %02X %02X %02X",
       id_arr[0],
       id_arr[1],
       id_arr[2],
       id_arr[3],
       id_arr[4],
       id_arr[5],
       id_arr[6],
       id_arr[7]);
}


/*-----------------------------------------------------------------------------------------------------
  Прочитать идентификатор единственного устройства на линии 1Wire

  \param id_arr

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
static uint32_t DS1W_read_id(uint8_t *id_arr)
{
  if (D1W_DetectPresence() != RES_OK)
  {
    return RES_ERROR;
  }
  D1W_ReadRom(id_arr);
  if (D1W_CheckRomCRC(id_arr) != D1W_CRC_OK)
  {
    return RES_ERROR;
  }
  return RES_OK;
}



/*! \brief  Perform a 1-Wire search
*
*  This function shows how the D1W_SearchRom function can be used to
*  discover all slaves on the bus. It will also CRC check the 64 bit
*  identifiers.
*
*  \param  devices Pointer to an array of type D1W_device. The discovered
*                  devices will be placed from the beginning of this array.
*
*  \param  len     The length of the device array. (Max. number of elements).
*
*
*  \retval SEARCH_SUCCESSFUL   Search completed successfully.
*  \retval SEARCH_CRC_ERROR    A CRC error occured. Probably because of noise
*                              during transmission.
*/
uint8_t DS1W_SearchBuses(D1W_device *devices, uint8_t max_devices, uint32_t *devices_num)
{
  uint8_t i, j;
  uint32_t      presence;
  uint8_t *newID;
  uint8_t *currentID;
  uint8_t lastDeviation;
  uint8_t numDevices;

  // Initialize all addresses as zero, on bus 0 (does not exist).
  for (i = 0; i < max_devices; i++)
  {
    for (j = 0; j < 8; j++)
    {
      devices[i].id_arr[j] = 0x00;
    }
  }
  *devices_num = 0;

  // Find the buses with slave devices.
  if (D1W_DetectPresence() != RES_OK)
  {
    return SEARCH_FAILED;
  }
  presence = 1;

  numDevices = 0;
  newID = devices[0].id_arr;

  // Go through all buses with slave devices.
  lastDeviation = 0;
  currentID = newID;
  if (presence) // Devices available on this bus.
  {
    // Do slave search  and place identifiers in the array.
    do
    {
      memcpy(newID, currentID, 8);
      D1W_DetectPresence();
      lastDeviation = D1W_SearchRom(newID, lastDeviation);
      currentID = newID;
      numDevices++;
      if (numDevices >= max_devices) break;
      newID=devices[numDevices].id_arr;
    }while (lastDeviation != D1W_ROM_SEARCH_FINISHED);
  }

  *devices_num = numDevices;
  // Go through all the devices and do CRC check.
  for (i = 0; i < numDevices; i++)
  {
    // If any id has a crc error, return error.
    if (D1W_CheckRomCRC(devices[i].id_arr) != D1W_CRC_OK)
    {
      *devices_num = 0;
      return SEARCH_CRC_ERROR;
    }
  }
  // Else, return Successful.
  return SEARCH_SUCCESSFUL;
}

/*! \brief  Find the first device of a family based on the family id
*
*  This function returns a pointer to a device in the device array
*  that matches the specified family.
*
*  \param  familyID    The 8 bit family ID to search for.
*
*  \param  devices     An array of devices to search through.
*
*  \param  size        The size of the array 'devices'
*
*  \return A pointer to a device of the family.
*  \retval NULL    if no device of the family was found.
*/
D1W_device* DS1W_FindFamily(uint8_t familyID, D1W_device *devices, uint8_t size)
{
  uint8_t i = 0;

  // Search through the array.
  while (i < size)
  {
    // Return the pointer if there is a family id match.
    if ((*devices).id_arr[0] == familyID)
    {
      return devices;
    }
    devices++;
    i++;
  }
  // Else, return NULL.
  return NULL;
}

D1W_device* D1W_Get_dev(uint32_t n)
{
  if (n < MAX_DEVICES) return &d1w_devices[n];
  else return 0;
}



/*! \brief  Set the wiper position of a DS2890.
*
*  This function initializes the DS2890 by enabling the charge pump. It then
*  changes the wiper position.
*
*  \param  position    The new wiper position.
*
*  \param  id          The 64 bit identifier of the DS2890.
*/
void DS2890_SetWiperPosition(uint8_t position, uint8_t *id)
{
  // Reset, presence.
  if (D1W_DetectPresence() != RES_OK)
  {
    return;
  }
  //Match id.
  D1W_MatchRom(id);

  // Send Write control register command.
  D1W_SendByte(DS2890_WRITE_CONTROL_REGISTER);

  // Write 0x4c to control register to enable charge pump.
  D1W_SendByte(0x4c);

  // Check that the value returned matches the value sent.
  if (D1W_ReceiveByte() != 0x4c)
  {
    return;
  }

  // Send release code to update control register.
  D1W_SendByte(DS2890_RELEASE_CODE);

  // Check that zeros are returned to ensure that the operation was
  // successful.
  if (D1W_ReceiveByte() == 0xff)
  {
    return;
  }

  // Reset, presence.
  if (D1W_DetectPresence() != RES_OK)
  {
    return;
  }

  // Match id.
  D1W_MatchRom(id);

  // Send the Write Position command.
  D1W_SendByte(DS2890_WRITE_POSITION);

  // Send the new position.
  D1W_SendByte(position);

  // Check that the value returned matches the value sent.
  if (D1W_ReceiveByte() != position)
  {
    return;
  }

  // Send release code to update wiper position.
  D1W_SendByte(DS2890_RELEASE_CODE);

  // Check that zeros are returned to ensure that the operation was
  // successful.
  if (D1W_ReceiveByte() == 0xff)
  {
    return;
  }
}

/*! \brief  Sends one byte of data on the 1-Wire(R) bus(es).
*
*  This function automates the task of sending a complete byte
*  of data on the 1-Wire bus(es).
*
*  \param  data    The data to send on the bus(es).
*
*/
void D1W_SendByte(uint8_t data)
{
  uint8_t temp;
  uint8_t i;

  // Do once for each bit
  for (i = 0; i < 8; i++)
  {
    // Determine if lsb is '0' or '1' and transmit corresponding
    // waveform on the bus.
    temp = data & 0x01;
    if (temp)
    {
      D1W_WriteBit1();
    }
    else
    {
      D1W_WriteBit0();
    }
    // Right shift the data to get next bit.
    data >>= 1;
  }
}


/*! \brief  Receives one byte of data from the 1-Wire(R) bus.
*
*  This function automates the task of receiving a complete byte
*  of data from the 1-Wire bus.
*
*  \return     The byte read from the bus.
*/
uint8_t D1W_ReceiveByte(void)
{
  uint8_t data;
  uint8_t i;

  // Clear the temporary input variable.
  data = 0x00;

  // Do once for each bit
  for (i = 0; i < 8; i++)
  {
    // Shift temporary input variable right.
    data >>= 1;
    // Set the msb if a '1' value is read from the bus.
    // Leave as it is ('0') else.
    if (D1W_ReadBit())
    {
      // Set msb
      data |= 0x80;
    }
  }
  return data;
}


/*! \brief  Sends the SKIP ROM command to the 1-Wire bus(es).
*
*/
void D1W_SkipRom(void)
{
  // Send the SKIP ROM command on the bus.
  D1W_SendByte(D1W_ROM_SKIP);
}


/*! \brief  Sends the READ ROM command and reads back the ROM id.
*
*  \param  romValue    A pointer where the id will be placed.
*
*/
void D1W_ReadRom(uint8_t *romValue)
{
  uint8_t bytesLeft = 8;

  // Send the READ ROM command on the bus.
  D1W_SendByte(D1W_ROM_READ);

  // Do 8 times.
  while (bytesLeft > 0)
  {
    // Place the received data in memory.
    *romValue++= D1W_ReceiveByte();
    bytesLeft--;
  }
}


/*! \brief  Sends the MATCH ROM command and the ROM id to match against.
*
*  \param  romValue    A pointer to the ID to match against.
*
*/
void D1W_MatchRom(uint8_t *romValue)
{
  uint8_t bytesLeft = 8;

  // Send the MATCH ROM command.
  D1W_SendByte(D1W_ROM_MATCH);

  // Do once for each byte.
  while (bytesLeft > 0)
  {
    // Transmit 1 byte of the ID to match.
    D1W_SendByte(*romValue++);
    bytesLeft--;
  }
}


/*! \brief  Sends the SEARCH ROM command and returns 1 id found on the
*          1-Wire(R) bus.
*
*  \param  bitPattern      A pointer to an 8 byte char array where the
*                          discovered identifier will be placed. When
*                          searching for several slaves, a copy of the
*                          last found identifier should be supplied in
*                          the array, or the search will fail.
*
*  \param  lastDeviation   The bit position where the algorithm made a
*                          choice the last time it was run. This argument
*                          should be 0 when a search is initiated. Supplying
*                          the return argument of this function when calling
*                          repeatedly will go through the complete slave
*                          search.
*
*  \return The last bit position where there was a discrepancy between slave addresses the last time this function was run. Returns D1W_ROM_SEARCH_FAILED if an error was detected (e.g. a device was connected to the bus during the search), or D1W_ROM_SEARCH_FINISHED when there are no more devices to be discovered.
*
*  \note   See main.c for an example of how to utilize this function.
*/
uint8_t D1W_SearchRom(uint8_t *bitPattern, uint8_t lastDeviation)
{
  uint8_t currentBit = 1;
  uint8_t newDeviation = 0;
  uint8_t bitMask = 0x01;
  uint8_t bitA;
  uint8_t bitB;

  // Send SEARCH ROM command on the bus.
  D1W_SendByte(D1W_ROM_SEARCH);

  // Walk through all 64 bits.
  while (currentBit <= 64)
  {
    // Read bit from bus twice.
    bitA = D1W_ReadBit();
    bitB = D1W_ReadBit();

    if (bitA && bitB)
    {
      // Both bits 1 (Error).
      newDeviation = D1W_ROM_SEARCH_FAILED;
      return newDeviation;
    }
    else if (bitA ^ bitB)
    {
      // Bits A and B are different. All devices have the same bit here.
      // Set the bit in bitPattern to this value.
      if (bitA)
      {
        (*bitPattern)|= bitMask;
      }
      else
      {
        (*bitPattern)&= ~bitMask;
      }
    }
    else // Both bits 0
    {
      // If this is where a choice was made the last time,
      // a '1' bit is selected this time.
      if (currentBit == lastDeviation)
      {
        (*bitPattern)|= bitMask;
      }
      // For the rest of the id, '0' bits are selected when
      // discrepancies occur.
      else if (currentBit > lastDeviation)
      {
        (*bitPattern)&= ~bitMask;
        newDeviation = currentBit;
      }
      // If current bit in bit pattern = 0, then this is
      // out new deviation.
      else if (!(*bitPattern & bitMask))
      {
        newDeviation = currentBit;
      }
      // IF the bit is already 1, do nothing.
      else
      {

      }
    }


    // Send the selected bit to the bus.
    if ((*bitPattern) & bitMask)
    {
      D1W_WriteBit1();
    }
    else
    {
      D1W_WriteBit0();
    }

    // Increment current bit.
    currentBit++;

    // Adjust bitMask and bitPattern pointer.
    bitMask <<= 1;
    if (!bitMask)
    {
      bitMask = 0x01;
      bitPattern++;
    }
  }
  return newDeviation;
}

/*-----------------------------------------------------------------------------------------------------


  \param inData
  \param seed

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t D1W_ComputeCRC8(uint8_t inData, uint8_t seed)
{
  uint8_t bitsLeft;
  uint8_t temp;

  for (bitsLeft = 8; bitsLeft > 0; bitsLeft--)
  {
    temp =((seed ^ inData) & 0x01);
    if (temp == 0)
    {
      seed >>= 1;
    }
    else
    {
      seed ^= 0x18;
      seed >>= 1;
      seed |= 0x80;
    }
    inData >>= 1;
  }
  return seed;
}

/*-----------------------------------------------------------------------------------------------------


  \param romValue

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t D1W_CheckRomCRC(uint8_t *romValue)
{
  uint8_t i;
  uint8_t crc8 = 0;

  for (i = 0; i < 7; i++)
  {
    crc8 = D1W_ComputeCRC8(*romValue, crc8);
    romValue++;
  }
  if (crc8 == (*romValue))
  {
    return D1W_CRC_OK;
  }
  return D1W_CRC_ERROR;
}


/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t D1W_WriteBit1(void)
{
  uint32_t status;
  //ITM_EVENT8(1,1);
  OneWire_UART_send_byte(0xFF);
  status = OneWire_UART_wait_transmit_complete(10);
  if (status == MQX_OK)
  {
    //ITM_EVENT8(1,2);
    return  status;
  }
  //ITM_EVENT8(2,1);
  DELAY_32us;
  return  status;
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
uint32_t D1W_WriteBit0(void)
{
  uint32_t status;
  //ITM_EVENT8(1,0);
  OneWire_UART_send_byte(0x00);
  status = OneWire_UART_wait_transmit_complete(10);
  if (status == MQX_OK)
  {
    //ITM_EVENT8(1,2);
    return  status;
  }
  //ITM_EVENT8(2,0);
  DELAY_32us;
  return  status;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t D1W_ReadBit(void)
{
  uint32_t status;
  OneWire_UART_assign_rx_data_wait(one_wire_recv_buf, 1);
  OneWire_UART_send_byte(0xFF);
  status = OneWire_UART_wait_transmit_complete(10);
  if (status == MQX_OK)
  {
    if (OneWire_UART_wait_data(10) == RES_OK)
    {
      if  (one_wire_recv_buf[0] != 0xFF)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }
  return  0;
}


/*-----------------------------------------------------------------------------------------------------
  Обнаружение импульса присутствия

  По спецификации 1Wire
  Импульс передачи 1 должен быть в пределах от 1  до 15 мкс
  Импульс передачи 0 должен быть в пределах от 60 до 120 мкс

  Длительность ипмульса присутствия может быть в пределах от 60 до 240 мкс

  При использовании UART значение 1 представляется одним старт битом, занчение 0 представляется 9-ю битами (1 старт бит и 8 бит в занчении 0)
  В этом случае соотношения длительностей импульсов 0 и 1 для двух крайних случаев будет:
  60  мкс /  6  мкс
  120 мкс /  13 мкс

  \param void

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint32_t  D1W_DetectPresence(void)
{
  uint32_t dt;

  Wait_ms(1);
  OneWire_UART_init(18000);                               //  Настраиваем скрость для отсылки сигнала presense
  one_wire_recv_buf[0] = 0xFF;
  one_wire_recv_buf[1] = 0xFF;

  OneWire_UART_send_byte(0);                              //  Посылаем импульс длинной 9/18000 = 500 мкс и паузой после импульса 1/18000 = 55 мкс
  dt = OneWire_wait_presence_pulse();
  if ((dt > 60) && (dt < 240))
  {
    if (dt > 120) dt = 120;
    one_wire_baud_rate =(9 * 1000000ul) / dt;
    OneWire_UART_init(one_wire_baud_rate);
    return RES_OK;
  }

  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------


  \param id

  \return signed int
-----------------------------------------------------------------------------------------------------*/
int32_t DS1820_ReadTemperature(uint8_t *rom_id, float *t)
{
  uint8_t   lsb;
  uint8_t   msb;
  int16_t   v;
  uint32_t  wait_cycles_cnt = 0;

  // Reset, presence.
  if (D1W_DetectPresence() != RES_OK)
  {
    return DS1820_ERROR; // Error
  }
  // Match the id found earlier.
  D1W_MatchRom(rom_id);
  // Send start conversion command.
  D1W_SendByte(DS1820_START_CONVERSION);
  // Wait until conversion is finished.
  // Bus line is held low until conversion is finished.
  while (!D1W_ReadBit())
  {
    wait_cycles_cnt++;
  }
  if (wait_cycles_cnt == 0)
  {
    return DS1820_ERROR; // Error
  }
  // Reset, presence.
  if (D1W_DetectPresence() != RES_OK)
  {
    return DS1820_ERROR; // Error
  }
  // Match id again.
  D1W_MatchRom(rom_id);
  // Send READ SCRATCHPAD command.
  D1W_SendByte(DS1820_READ_SCRATCHPAD);
  // Read only two first bytes (temperature low, temperature high)
  // and place them in the 16 bit temperature variable.
  lsb = D1W_ReceiveByte();
  msb = D1W_ReceiveByte();
  v =(msb << 8) | lsb;
  *t = (float)(v) / 2.0f;

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------


  \param initial_input
-----------------------------------------------------------------------------------------------------*/
static void Task_1WIRE(uint32_t initial_data)
{
  int32_t status;
  OneWire_UART_config();

  Wait_ms(10);

  DS1W_SearchBuses(app.onew_devices, TEMP_ZONES_NUM,&app.onew_devices_num);

  // Если количество обнаруженных устройств на шине больше на 1 чем валидных устройств записанных в настройках,
  // то добавляем в настройки обнаруженное устройство
  uint32_t valid_id_num =  Get_valid_sensors_num();
  if (app.onew_devices_num  == (valid_id_num + 1))
  {
    // Добавляем в настройки  обнаруженный id сенсора
    Append_new_sensors_id(app.onew_devices, app.onew_devices_num);
  }
  valid_id_num =  Get_valid_sensors_num();

  for (;;)
  {
    if (valid_id_num > 0)
    {
      float   aver = 0;
      uint32_t cnt = 0;
      for (uint32_t i=0; i < valid_id_num; i++)
      {
        float t;
        app.current_sensor = i;
        status = DS1820_ReadTemperature(sensor_configs[i].sensor_id, &t);
        if (status == RES_OK)
        {
          app.temperatures[i]  = t;
          app.sensors_state[i] = 1;
          aver += t;
          cnt++;
        }
        else
        {
          app.sensors_state[i] = 0;
        }
      }
      app.temperature_aver = aver/cnt;
    }
    App_set_event(EVENT_SENSOR_RESULTS_READY);
    Wait_ms(100);
  }

}

