#include "test.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "uart.h"
#include "gpio.h"
#include "rtc.h"
#include "fram.h"
#include "flash_ext.h"
#include "adc.h"

static void testThread(void const * argument);
static void testUartThread(void const * argument);
static void testRtc();
static void testAdc();
static void testFram();
static void testFlash1();
static void testFlash2();

#define UPLOAD_FILENAME "0:test.tmp"

void testInit()
{
  /* Create Test thread */
  osThreadDef(Test_Thread, testThread, osPriorityNormal, 0, 8 * configMINIMAL_STACK_SIZE);
  osThreadCreate(osThread(Test_Thread), NULL);

  osThreadDef(Test_Uart_Thread, testUartThread, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE);
  osThreadCreate(osThread(Test_Uart_Thread), NULL);
}

static void testThread(void const * argument)
{
  (void)argument;

#if (TEST_USB_FAT == 1)
  FATFS fatfs;
  FIL file;
  FIL fileR;
  uint8_t ramBuf[512] = { 0x00 };
  uint8_t startTestUsb = 1;
#endif

  BlinkLed blinkLed;
  blinkLed.prvSetupHardware();

#if (TEST_UART == 1)
  int sizePkt;
  uint8_t buffer[UART_BUF_SIZE];
  bool turn = false;

  uart_init(uart2, 9600);
  osSemaphoreId semaphoreUart = osSemaphoreCreate(NULL, 1);
  osSemaphoreWait(semaphoreUart, 0);
  uart_setSemaphoreId(uart2, semaphoreUart);
#endif

  testRtc();
  testAdc();
  testFram();
  testFlash1();
  testFlash2();

  while(1) {
#if (TEST_USB_FAT == 1)
    if ((disk_status(0) == RES_OK) && (startTestUsb == 1)) {
      FRESULT result = f_mount(&fatfs, "0", 1);
      if (result == FR_OK) {
        UINT bytesWritten = 0;
        UINT BytesRead;

        ramBuf[0] = 0x55;
        ramBuf[511] = 0xAA;
        f_unlink(UPLOAD_FILENAME);
        if (f_open(&file, UPLOAD_FILENAME, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
          f_write(&file, ramBuf, 512, &bytesWritten);
          f_close(&file);
        }
        ramBuf[0] = 0x00;
        ramBuf[511] = 0x00;
        if (f_open(&fileR, UPLOAD_FILENAME, FA_READ) == FR_OK) {
          f_read(&fileR, ramBuf, 512, &BytesRead);
          f_close(&fileR);
        }
        startTestUsb = 0;
      }
    }
#endif

#if (TEST_LED == 1)
    blinkLed.toggle();
    osDelay(500);
#endif

#if (TEST_UART == 1)
    buffer[0] = 0x55;
    buffer[1] = 0xAA;
    buffer[2] = 0x00;
    uart_writeData(uart2, buffer, 3);

    osSemaphoreWait(semaphoreUart, osWaitForever);
    while (1) {
      if (osSemaphoreWait(semaphoreUart, 5) == osEventTimeout) {
        sizePkt = uart_readData(uart2, buffer);
        if ((buffer[0] != 0xFE) || (buffer[1] != 0x55) ||
            (buffer[2] != 0x01) || (sizePkt != 3))
          asm("nop");

        if (turn)
          onLed(FanLed);
        else
          offLed(FanLed);
        turn = !turn;
        break;
      }
    }
#endif
  }
}

static void testUartThread(void const * argument)
{
  (void)argument;
  int countByte = 0;
  int sizePkt;
  uint8_t buffer[UART_BUF_SIZE];

  uart_init(uart4, 9600);

  osSemaphoreId semaphoreUart = osSemaphoreCreate(NULL, 1);
  osSemaphoreWait(semaphoreUart, 0);
  uart_setSemaphoreId(uart4, semaphoreUart);

  while(1) {
    osSemaphoreWait(semaphoreUart, osWaitForever);
    while (1) {
      if (osSemaphoreWait(semaphoreUart, 5) == osEventTimeout) {
        buffer[0] = 0xFE;
        buffer[1] = 0x55;
        buffer[2] = 0x01;
        uart_writeData(uart4, buffer, 3);

        sizePkt = uart_readData(uart4, buffer);
        if ((buffer[0] != 0x55) || (buffer[1] != 0xAA) ||
            (buffer[2] != 0x00) || (sizePkt != 3))
          asm("nop");
        countByte = 0;
        break;
      } else {
        countByte++;
      }
    }
  }
}

static void testRtc()
{
#if (TEST_RTC == 1)
  DateTimeTypeDef dateTime;
  //! Установка даты и времени
  dateTime.month = 11;
  dateTime.date = 6;
  dateTime.year = 14;
  dateTime.hours = 17;
  dateTime.minutes = 21;
  dateTime.seconds = 10;
  dateTime.mseconds = 0;
  setDateTime(dateTime);
  osDelay(1005);
  getDateTime(&dateTime);
  if ((dateTime.month != 11) || (dateTime.date != 6) || (dateTime.year != 14) ||
      (dateTime.hours != 17) || (dateTime.minutes != 21) ||
      (dateTime.seconds != 11)) {
    asm("nop");
  }
#endif
}

static void testAdc()
{
#if (TEST_ADC == 1)
  StatusType status;
  static float coreTemperature;
  static float coreVbattery;
  static float coreVref;

  status = getCoreVref(&coreVref);
  if (status == StatusOk)
    status = getCoreTemperature(&coreTemperature);
  if (status == StatusOk)
    status = getCoreVbattery(&coreVbattery);

  if (status != StatusOk)
    asm("nop");
  asm("nop");
#endif
}

static void testFram()
{
#if (TEST_FRAM == 1)
  uint8_t bufferTx[10];
  uint8_t bufferRx[10];

  bufferTx[0] = 0x21;
  bufferTx[1] = 0xAA;
  bufferTx[2] = 0x00;
  bufferTx[3] = 0x00;
  bufferTx[4] = 0x00;
  bufferTx[5] = 0x33;
  bufferTx[6] = 0x00;
  bufferTx[7] = 0x00;
  bufferTx[8] = 0x55;
  bufferTx[9] = 0x12;
  framWriteData(10, bufferTx, 10);
  framReadData(10, bufferRx, 10);

  if (memcmp(bufferTx, bufferRx, 10)) {
    asm("nop"); // Ошибка
  }
  asm("nop");
#endif
}

static void testFlash1()
{
#if (TEST_FLASH1 == 1)
  uint8_t bufferTx[10];
  uint8_t bufferRx[10];

  bufferTx[0] = 0x21;
  bufferTx[1] = 0xAA;
  bufferTx[2] = 0x01;
  bufferTx[3] = 0x12;
  bufferTx[4] = 0x03;
  bufferTx[5] = 0x33;
  bufferTx[6] = 0x01;
  bufferTx[7] = 0x20;
  bufferTx[8] = 0x55;
  bufferTx[9] = 0x12;
  flashExtWrite(FlashSpi1, 0, &bufferTx[0], 10);
  flashExtRead(FlashSpi1, 0, &bufferRx[0], 10);

  if (memcmp(bufferTx, bufferRx, 10)) {
    asm("nop"); // Ошибка
  }
  asm("nop");
#endif
}

static void testFlash2()
{
#if (TEST_FLASH2 == 1)
  uint8_t bufferTx[10];
  uint8_t bufferRx[10];

  bufferTx[0] = 0x21;
  bufferTx[1] = 0xAA;
  bufferTx[2] = 0x01;
  bufferTx[3] = 0x12;
  bufferTx[4] = 0x03;
  bufferTx[5] = 0x33;
  bufferTx[6] = 0x01;
  bufferTx[7] = 0x20;
  bufferTx[8] = 0x55;
  bufferTx[9] = 0x12;
  flashExtWrite(FlashSpi5, 0, &bufferTx[0], 10);
  flashExtRead(FlashSpi5, 0, &bufferRx[0], 10);

  if (memcmp(bufferTx, bufferRx, 10)) {
    asm("nop"); // Ошибка
  }
  asm("nop");
#endif
}