#include "host.h"
#include "gpio.h"
#include "string.h"

#define TIMEOUT_RX 100

SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_tx;

static osSemaphoreId hostSemaphoreId;
static uint8_t txBuffer[2*HOST_BUF_SIZE];
static uint8_t rxBuffer[HOST_BUF_SIZE];

static int rxCount = 0;
static int rxTimeout = 0;
static int rxActive = 0;
static int unstuff = 0;

static void hostTimer(const void *argument);

void hostInit()
{
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_SLAVE;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi4.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi4.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLED;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
  HAL_SPI_Init(&hspi4);

  /* Peripheral DMA init*/

  hdma_spi4_tx.Instance = DMA2_Stream1;
  hdma_spi4_tx.Init.Channel = DMA_CHANNEL_4;
  hdma_spi4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi4_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi4_tx.Init.Mode = DMA_NORMAL;
  hdma_spi4_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_spi4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  HAL_DMA_Init(&hdma_spi4_tx);

  __HAL_LINKDMA(&hspi4, hdmatx, hdma_spi4_tx);

  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

  HAL_NVIC_SetPriority(SPI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(SPI4_IRQn);

  __HAL_SPI_ENABLE_IT(&hspi4, SPI_IT_RXNE);
  __HAL_SPI_ENABLE(&hspi4);

  osTimerDef_t timerDef = {hostTimer};
  osTimerId timerId = osTimerCreate(&timerDef, osTimerPeriodic, 0);
  osTimerStart(timerId, 1);
}

osSemaphoreId hostSemaphoreCreate()
{
  hostSemaphoreId = osSemaphoreCreate(NULL, 1);
  osSemaphoreWait(hostSemaphoreId, 0);
  return hostSemaphoreId;
}

static void hostTimer(const void * argument)
{
  (void)argument;

  if (rxActive) {
    if (rxTimeout) {
      rxTimeout--;
    }
    else {
      memset(rxBuffer, 0, sizeof(rxBuffer));
      rxCount = 0;
      rxActive = 0;
      osSemaphoreRelease(hostSemaphoreId);
    }
  }
}

void hostRxIRQHandler(void)
{
  if ((__HAL_SPI_GET_FLAG(&hspi4, SPI_FLAG_RXNE) != RESET) &&
      (__HAL_SPI_GET_IT_SOURCE(&hspi4, SPI_IT_RXNE) != RESET) &&
      (__HAL_SPI_GET_FLAG(&hspi4, SPI_FLAG_OVR) == RESET)) {
    uint8_t data = hspi4.Instance->DR;
    if (data == 0x7E) {
      //! Конец пакета
      if (rxActive) {
        rxTimeout = 0;
        rxActive = 0;

        osSemaphoreRelease(hostSemaphoreId);
      }
      //! Начало пакета
      else {
        rxActive = 1;
        rxCount = 0;
        rxTimeout = TIMEOUT_RX;
      }
    }
    //! Прием данных пакета
    else {
      if (rxActive) {
        rxTimeout = TIMEOUT_RX;
        if (data == 0x7D) {
          unstuff = 1;
          return;
        }
        //! Выкидываем байтстаффинг
        if (unstuff) {
          data |= (1 << 5);
          unstuff = 0;
        }
        if (rxCount < HOST_BUF_SIZE)
          rxBuffer[rxCount++] = data;
      }
    }
  } else {
    if(__HAL_SPI_GET_FLAG(&hspi4, SPI_FLAG_OVR) != RESET) {
      __HAL_SPI_CLEAR_OVRFLAG(&hspi4);
    }
  }
}

int hostReadData(uint8_t *data)
{
  const int count = rxCount;
  memcpy(data, rxBuffer, count);
  rxCount = 0;

  return count;
}

/*!
 \brief Ожидание установки флага статусного регистра SPI

 \param flag - флаг
 \param status - необходимое состояние флага
 \param timeout - таймаут
 \return HAL_StatusTypeDef
*/
static HAL_StatusTypeDef spiWaitOnFlagUntilTimeout(uint32_t flag, FlagStatus status,
                                                   uint32_t timeout)
{
  uint32_t tickstart = HAL_GetTick();

  while(__HAL_SPI_GET_FLAG(&hspi4, flag) == status) {
    if(timeout != HAL_MAX_DELAY) {
      if((timeout == 0) || ((HAL_GetTick() - tickstart) > timeout))
        return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

//static int txCount = 0;
//static void spiDmaTransmitCplt(DMA_HandleTypeDef *hdma)
//{
//  StatusType status = StatusOk;
//  SPI_HandleTypeDef* hspi = ( SPI_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

//  if((hdma->Instance->CR & DMA_SxCR_CIRC) == 0) {
//    if(spiWaitOnFlagUntilTimeout(hspi, SPI_FLAG_TXE, RESET, 10) != HAL_OK)
//      status = StatusError;
//    hspi->Instance->CR2 &= (uint32_t)(~SPI_CR2_TXDMAEN);

//    if(spiWaitOnFlagUntilTimeout(hspi, SPI_FLAG_BSY, SET, 10) != HAL_OK)
//      status = StatusError;
//  }

//  if(hspi->Init.Direction == SPI_DIRECTION_2LINES)
//    __HAL_SPI_CLEAR_OVRFLAG(hspi);

//  if(status == StatusOk) {
//    txCount++;
//    asm("nop");
//  }
//}

StatusType hostWriteData(uint8_t *data, int size, uint32_t timeout)
{
  int idx = 0;
  txBuffer[idx++] = 0x7E;

  for (int i = 0; i < size; i++) {
    if ((data[i] == 0x7E) || (data[i] == 0x7D)) {
      txBuffer[idx++] = 0x7D;
      txBuffer[idx++] = data[i] ^ (1<<5);
    } else {
      txBuffer[idx++] = data[i];
    }
  }

  txBuffer[idx++] = 0x7E;

//  hspi4.hdmatx->XferCpltCallback = spiDmaTransmitCplt;
//  hspi4.Instance->CR2 |= SPI_CR2_TXDMAEN;
//  HAL_DMA_Start_IT(hspi4.hdmatx, (uint32_t)txBuffer, (uint32_t)&hspi4.Instance->DR, count);

  uint16_t xferCount = idx;
  uint8_t *pTxBuffPtr = txBuffer;
  while (xferCount > 0) {
    if(spiWaitOnFlagUntilTimeout(SPI_FLAG_TXE, RESET, timeout) != HAL_OK)
      return StatusError;

    hspi4.Instance->DR = (*pTxBuffPtr++);
    xferCount--;
  }
  if (spiWaitOnFlagUntilTimeout(SPI_FLAG_TXE, RESET, 10) != HAL_OK)
    return StatusError;

  if (spiWaitOnFlagUntilTimeout(SPI_FLAG_BSY, SET, 10) != HAL_OK)
    return StatusError;

  if (hspi4.Init.Direction == SPI_DIRECTION_2LINES)
    __HAL_SPI_CLEAR_OVRFLAG(&hspi4);

  return StatusOk;
}