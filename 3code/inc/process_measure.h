// balmer@inbox.ru RLC Meter 303
// 2013-2014

#ifndef _PROCESS_MEASURE_H_
#define _PROCESS_MEASURE_H_

#include "adc.h"

typedef enum STATES
{
	STATE_NOP=0,
	STATE_INIT_WAIT,
	STATE_RESISTOR_INDEX,
	STATE_RESISTOR_INDEX_WAIT,
	STATE_GAIN_INDEX,
	STATE_GAIN_INDEX_WAIT,
	STATE_MEASURE,
	STATE_MEASURE_WAIT,
} STATES;

void ProcessData();
void ProcessStartComputeX(
			uint8_t count, uint8_t predefinedResistorIdx_,
			uint8_t predefinedGainVoltageIdx_,
			uint8_t predefinedGainCurrentIdx_,
			bool useCorrector_
	);
STATES ProcessGetState();
void SendRVI();

extern uint8_t resistorIdx;
extern uint8_t gainVoltageIdx;
extern uint8_t gainCurrentIdx;
extern bool isSerial;
extern bool bContinuousMode; //Заполнять перед ProcessStartComputeX
extern bool bCalibration;

float getGainValue(uint8_t idx);
float getGainValueV();
float getGainValueI();
float getResistorOm();

#endif//_PROCESS_MEASURE_H_