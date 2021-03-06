#ifndef TMSPICV2_H
#define TMSPICV2_H

#include "tms.h"
#include "device_modbus.h"

/*!
 * \brief Класс системы телеметрической "PICv2"
 */
class TmsPicV2 : public Tms
{
public:
  TmsPicV2();
  virtual ~TmsPicV2();

  /*!
   * \brief Функция инициализации ТМС
   * в функции создаётся задача для работы с устройством и создаётся
   * modbus протокол для работы с устройством
   */
  void init();

  /*!
   * \brief Функция заполнения массива параметров modbus карты
   */
  void initModbusParameters();

  /*!
   * \brief Функция заполнения массива параметров по массиву modbus параметров
   */
  void initParameters();

  /*!
   * \brief Функция проверки и обновления параметра устройства
   * \param id - уникальный идентификатор параметра
   */
  void getNewValue(uint16_t id);

  /*!
   * \brief Функция записи нового значения в устройство
   * \param id - уникальный идентификатор параметра
   * \param value - значение параметра
   * \return 0 - выполнено, код ошибки
   */
  uint8_t setNewValue(uint16_t id, float value, EventType eventType = AutoType);

  /*!
   * \brief Метод записи параметра в устройство
   * \param id - индекс параметра
   * \param value - значение
   */
  void writeToDevice(int id, float value);

  bool isConnect();
  void getConnect();
  void resetConnect();

private:
  bool config();

  ModbusParameter modbusParameters_[18];
  DeviceModbus *dm_;
  bool isConfigurated_;

};

#endif // TMSPICV2_H
