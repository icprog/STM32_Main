#include "log_data.h"
#include "user_main.h"
#include <string.h>

LogData::LogData() : Log(DataTypeLog)
{

}

LogData::~LogData()
{

}

static void logDataTask(void *p)
{
  (static_cast<LogData*>(p))->task();
}

void LogData::init()
{
  Log::init();

  osThreadDef_t t = {"LogData", logDataTask, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE};
  threadId_ = osThreadCreate(&t, this);
}

void LogData::task()
{
  int normTimeCnt = 0;
  int fastTimeCnt = 0;
  bool startFastMode = false;

  while (1) {
    osDelay(1000);

    int condition = KSU.getValue(CCS_CONDITION);
#if DEBUG
//    condition = CCS_CONDITION_DELAY;
#endif
    if (condition == CCS_CONDITION_DELAY) {
      if (!startFastMode) {
        startFastMode = true;
        add(FastModeCode);
      } else {
        int period = KSU.getValue(CCS_PERIOD_LOG_FAST_MODE);
#if DEBUG
        period = 5;
#endif
        if (++fastTimeCnt >= period) {
          fastTimeCnt = 0;
          add(FastModeCode);
        }
      }
    } else {
      startFastMode = false;
      fastTimeCnt = 0;
    }

    int period = KSU.getValue(CCS_PERIOD_LOG_NOLMAL_MODE);
#if DEBUG
    period = 60;
#endif
    if (++normTimeCnt >= period) {
      normTimeCnt = 0;
      if (condition != CCS_CONDITION_DELAY)
        add(NormModeCode);
    }
  }
}

void LogData::add(uint8_t code)
{
  memset(buffer, 0, sizeof(buffer));

  time_t time = getTime();

  *(uint32_t*)(buffer) = ++id_;
  *(uint32_t*)(buffer+4) = time;
  *(uint8_t*)(buffer+8) = code;
  *(float*)(buffer+9) = KSU.getValue(CCS_NUMBER_WELL);
  *(float*)(buffer+13) = KSU.getValue(VSD_CURRENT_FREQUENCY);
  *(float*)(buffer+17) = KSU.getValue(VSD_OUT_CURRENT_PHASE_1);
  *(float*)(buffer+21) = KSU.getValue(VSD_OUT_CURRENT_PHASE_2);
  *(float*)(buffer+25) = KSU.getValue(VSD_OUT_CURRENT_PHASE_3);
  *(float*)(buffer+29) = KSU.getValue(VSD_VOLTAGE_MOTOR);
  *(float*)(buffer+33) = KSU.getValue(CCS_MOTOR_IMBALANCE_CURRENT);
  *(float*)(buffer+37) = KSU.getValue(VSD_COS_PHI_MOTOR);
  *(float*)(buffer+41) = KSU.getValue(CCS_LOAD_MOTOR);
  *(float*)(buffer+45) = KSU.getValue(VSD_ACTIVE_POWER);

//  *(float*)(buffer+49) = KSU.getValue();
  *(float*)(buffer+53) = KSU.getValue(EM_VOLTAGE_PHASE_1_2);
  *(float*)(buffer+57) = KSU.getValue(EM_VOLTAGE_PHASE_2_3);
  *(float*)(buffer+61) = KSU.getValue(EM_VOLTAGE_PHASE_3_1);
  *(float*)(buffer+65) = KSU.getValue(CCS_SYPPLY_IMBALANCE_VOLTAGE);
  *(float*)(buffer+69) = KSU.getValue(EM_CURRENT_PHASE_1);
  *(float*)(buffer+73) = KSU.getValue(EM_CURRENT_PHASE_2);
  *(float*)(buffer+77) = KSU.getValue(EM_CURRENT_PHASE_3);
//  *(float*)(buffer+81) = KSU.getValue();
  *(float*)(buffer+85) = KSU.getValue(CCS_RESISTANCE_ISOLATION);

//  *(float*)(buffer+89) = KSU.getValue();
  *(float*)(buffer+93) = KSU.getValue(TMS_TEMPERATURE_WINDING);
  *(float*)(buffer+97) = KSU.getValue(TMS_PRESSURE_INTAKE);
  *(float*)(buffer+101) = KSU.getValue(TMS_TEMPERATURE_INTAKE);
  *(float*)(buffer+105) = KSU.getValue(EM_ACTIVE_POWER);
  *(float*)(buffer+109) = KSU.getValue(EM_FULL_POWER);
  *(float*)(buffer+113) = KSU.getValue(VSD_OUT_CURRENT_MOTOR);
  *(float*)(buffer+117) = KSU.getValue(VSD_OUT_VOLTAGE_MOTOR);
//  *(float*)(buffer+121) = KSU.getValue();
//  *(float*)(buffer+125) = KSU.getValue();

//  *(float*)(buffer+129) = KSU.getValue();
  *(float*)(buffer+133) = KSU.getValue(TMS_TEMPERATURE_DISCHARGE);
  *(float*)(buffer+137) = KSU.getValue(TMS_PRESSURE_DISCHARGE);
//  *(float*)(buffer+141) = KSU.getValue();
//  *(float*)(buffer+145) = KSU.getValue();
  *(float*)(buffer+149) = KSU.getValue(TMS_FLOW_DISCHARGE);
  *(float*)(buffer+153) = KSU.getValue(VSD_RADIATOR_TEMPERATURE);
  *(float*)(buffer+157) = KSU.getValue(VSD_CONTROL_TEMPERATURE);
//  *(float*)(buffer+161) = KSU.getValue();
  *(float*)(buffer+165) = KSU.getValue(CCS_CONDITION);

  *(float*)(buffer+169) = KSU.getValue(CCS_WORKING_MODE);
//  *(float*)(buffer+173) = KSU.getValue();
//  *(float*)(buffer+177) = KSU.getValue();
  *(float*)(buffer+181) = KSU.getValue(TMS_PSW_TMS);
  *(float*)(buffer+185) = KSU.getValue(TMS_PSW_TMSN);
  *(float*)(buffer+189) = KSU.getValue(VSD_DC_VOLTAGE);
  *(float*)(buffer+193) = KSU.getValue(VSD_STATUS_WORD);
  *(float*)(buffer+197) = KSU.getValue(VSD_ALARM_WORD_1);
  *(float*)(buffer+201) = KSU.getValue(VSD_ALARM_WORD_2);
  *(float*)(buffer+205) = KSU.getValue(VSD_WARNING_WORD_1);

  *(float*)(buffer+209) = KSU.getValue(VSD_WARNING_WORD_2);
//  *(float*)(buffer+213) = KSU.getValue();
  *(float*)(buffer+217) = KSU.getValue(VSD_LAST_ALARM);
//  *(float*)(buffer+221) = KSU.getValue();
  *(float*)(buffer+225) = KSU.getValue(CCS_RGM_PERIODIC_MODE);
  *(float*)(buffer+229) = KSU.getValue(CCS_RGM_PERIODIC_TIME);
  *(float*)(buffer+233) = KSU.getValue(CCS_RGM_PERIODIC_TIME);
  *(float*)(buffer+237) = KSU.getValue(CCS_RGM_PERIODIC_TIME_END);
  *(float*)(buffer+241) = KSU.getValue(CCS_RGM_PERIODIC_TIME_END);

//  *(float*)(buffer+245) = KSU.getValue();

//  *(float*)(buffer+249) = KSU.getValue();

  write(buffer, 253);
}
