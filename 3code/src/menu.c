// balmer@inbox.ru RLC Meter 303
// 2013-2014

#include "hw_config.h"
#include "quadrature_encoder.h"
#include "lcd_interface.h"
#include "menu.h"
#include "dac.h"
#include "adc.h"
#include "process_measure.h"
#include "number_edit.h"
#include "corrector.h"
#include "systick.h"

#define SIZEOF(x) (sizeof(x)/sizeof(x[0]))

typedef enum MenuEnum {
	MENU_NONE = 0, //Несуществующий пункт меню
	MENU_MAIN_RETURN, //Выход из main menu
	MENU_MAIN_FREQUENCY,
	MENU_MAIN_SER_PAR,
	MENU_MAIN_VIEW_PARAM, //Какой из параметров отображается.
	MENU_MAIN_TOGGLE_LIGHT, //Подсветка дисплея
	MENU_MAIN_CORRECTION,
	MENU_RETURN, //Возврат в main menu
	MENU_F_100Hz,
	MENU_F_1KHz,
	MENU_F_10KHz,
	MENU_F_93_75KHz,
	MENU_F_187_5KHz,
	MENU_SP_SERIAL,
	MENU_SP_PARALLEL,
	MENU_V_RIM,
	MENU_V_LC,
	MENU_CORRECTION_SHORT,
	MENU_CORRECTION_1_Om,
	MENU_CORRECTION_100_Om,
	MENU_CORRECTION_1_KOm,
	MENU_CORRECTION_10_KOm,
	MENU_CORRECTION_100_KOm,
	MENU_CORRECTION_OPEN,
	MENU_CORRECTION_SAVE,
	MENU_CORRECTION_CLEAR,
} MenuEnum;

typedef struct MenuElem {
	char* text;
	MenuEnum command;
} MenuElem;

static MenuElem g_main_menu[]={
	{"..", MENU_MAIN_RETURN},
	{"Frequency", MENU_MAIN_FREQUENCY},
	{"SER/PAR", MENU_MAIN_SER_PAR},
	{"View", MENU_MAIN_VIEW_PARAM},
	{"Toggle Light", MENU_MAIN_TOGGLE_LIGHT},
	{"Correction", MENU_MAIN_CORRECTION},
};

static MenuElem g_f_menu[]={
	{"..", MENU_RETURN},
	{"100 Hz", MENU_F_100Hz},
	{"1 KHz", MENU_F_1KHz},
	{"10 KHz", MENU_F_10KHz},
	{"93.75 KHz", MENU_F_93_75KHz},
	{"187.5 KHz", MENU_F_187_5KHz},
};

static MenuElem g_sp_menu[]={
	{"..", MENU_RETURN},
	{"SERIAL", MENU_SP_SERIAL},
	{"PARALLEL", MENU_SP_PARALLEL},
};

static MenuElem g_v_menu[]={
	{"..", MENU_RETURN},
	{"R.imag", MENU_V_RIM},
	{"L/C", MENU_V_LC},
};

static MenuElem g_correction_menu[]={
	{"..", MENU_RETURN},
	{"short", MENU_CORRECTION_SHORT},
	{"open", MENU_CORRECTION_OPEN},
	{"1 Om", MENU_CORRECTION_1_Om},
	{"100 Om", MENU_CORRECTION_100_Om},
	{"1 KOm", MENU_CORRECTION_1_KOm},
	{"10 KOm", MENU_CORRECTION_10_KOm},
	{"100 KOm", MENU_CORRECTION_100_KOm},
	{"SAVE", MENU_CORRECTION_SAVE},
	{"CLEAR", MENU_CORRECTION_CLEAR},
};

static MenuElem* g_cur_menu = NULL;
static uint8_t g_menu_size = 0;
static uint8_t g_menu_pos = 0;
static bool g_update = false;
static char* message_line1 = NULL;
static char* message_line2 = NULL;

static MenuEnum g_last_main_command = MENU_MAIN_FREQUENCY;
static MenuEnum g_last_f_command = MENU_F_100Hz;
static MenuEnum g_last_correction_command = MENU_NONE;

#define MENU_START(menu) \
	g_cur_menu = menu; \
	g_menu_size = SIZEOF(menu); \
	g_menu_pos = 0;

#define MENU_CLEAR() \
	g_cur_menu = NULL; \
	g_menu_size = 0;

void MenuSetF(uint32_t period);
void MenuSetSerial(bool ser);
void MenuSetPos(MenuEnum pos);
void MenuSetPrinRim(bool pr);
void ToggleLight();
void MenuOnCommand(MenuEnum command);
void MenuClearFlash();
void MenuOnCorrection(MenuEnum command);
void OnNumberEditEnd();

void OnButtonPressed()
{
	g_update = true;
	if(NumberEditStarted())
	{
		NumberEditOnButtonPressed();
		return;
	}

	if(message_line1)
	{//MessageBox suport
		message_line1 = NULL;
		return;
	}


	if(g_cur_menu==NULL)
	{
		MENU_START(g_main_menu);
		MenuSetPos(g_last_main_command);
		return;
	}

	if(g_menu_pos>=g_menu_size)
		return;
	MenuEnum command = g_cur_menu[g_menu_pos].command;
	MenuOnCommand(command);
}

void OnWeel(int16_t delta)
{
	if(NumberEditStarted())
	{
		NumberEditOnWeel(delta);
	    g_update = true;
		return;
	}

	if(message_line1)
	{//MessageBox suport
		return;
	}

	if(g_cur_menu==NULL)
		return;

	g_menu_pos = (g_menu_pos+g_menu_size+delta)%g_menu_size;
    g_update = true;
}

void OnTimer()
{
	if(NumberEditCompleted())
	{
		NumberEditEnd();
		OnNumberEditEnd();
		g_update = true;
	}

	if(g_update)
	{
		LcdRepaint();
		g_update = false;		
	}
}

void MenuOnCommand(MenuEnum command)
{
	switch(command)
	{
	case MENU_NONE:
		break;
	case MENU_MAIN_RETURN:
		MENU_CLEAR();
		break;
	case MENU_MAIN_FREQUENCY:
		MENU_START(g_f_menu);
		MenuSetPos(g_last_f_command);
		g_last_main_command = command;
		break;
	case MENU_MAIN_SER_PAR:
		MENU_START(g_sp_menu);
		MenuSetPos(isSerial?MENU_SP_SERIAL:MENU_SP_PARALLEL);
		g_last_main_command = command;
		break;
	case MENU_MAIN_VIEW_PARAM:
		g_last_main_command = command;
		MENU_START(g_v_menu);
		MenuSetPos(printRim?MENU_V_RIM:MENU_V_LC);
		break;
	case MENU_MAIN_TOGGLE_LIGHT:
		ToggleLight();
		MENU_CLEAR();
		break;
	case MENU_MAIN_CORRECTION:
		g_last_main_command = command;
		MENU_START(g_correction_menu);
		break;
	case MENU_RETURN:
		MENU_START(g_main_menu);
		MenuSetPos(g_last_main_command);
		break;
	case MENU_F_100Hz:
		MenuSetF(720000);
		g_last_f_command = command;
		break;
	case MENU_F_1KHz:
		MenuSetF(72000);
		g_last_f_command = command;
		break;
	case MENU_F_10KHz:
		MenuSetF(7200);
		g_last_f_command = command;
		break;
	case MENU_F_93_75KHz:
		MenuSetF(768);
		g_last_f_command = command;
		break;
	case MENU_F_187_5KHz:
		MenuSetF(384);
		g_last_f_command = command;
		break;
	case MENU_SP_SERIAL:
		MenuSetSerial(true);
		break;
	case MENU_SP_PARALLEL:
		MenuSetSerial(false);
		break;
	case MENU_V_RIM:
		MenuSetPrinRim(true);
		break;
	case MENU_V_LC:
		MenuSetPrinRim(false);
		break;
	case MENU_CORRECTION_1_Om:
	case MENU_CORRECTION_100_Om:
	case MENU_CORRECTION_1_KOm:
	case MENU_CORRECTION_10_KOm:
	case MENU_CORRECTION_100_KOm:
	case MENU_CORRECTION_SHORT:
	case MENU_CORRECTION_OPEN:
	case MENU_CORRECTION_SAVE:
	case MENU_CORRECTION_CLEAR:
		MenuOnCorrection(command);
		break;
	}
}

void MenuRepaint()
{
	if(NumberEditStarted())
	{
		NumberEditRepaint();
		return;
	}

	if(message_line1)
	{//MessageBox suport
		LcdGotoXYFont( 1, 3 );
		LcdStr(FONT_1X, message_line1);
		if(message_line2)
		{
			LcdGotoXYFont( 1, 4 );
			LcdStr(FONT_1X, message_line2);
		}
		return;
	}

	if(g_menu_size==0 || g_cur_menu==NULL)
		return;
	const uint8_t height = 6;
	const uint8_t font_height = 8;
	uint8_t ystart = 1;

	if(g_menu_size<height)
	{
		ystart = (height-g_menu_size)/2+1;
	}

	uint8_t istart = 0;
	uint8_t iend = g_menu_size;
	if(g_menu_size>=height)
	{
		if(g_menu_pos<height)
		{
			iend = height;
		} else
		{
			istart = g_menu_pos-height+1;
			iend = g_menu_pos+1;
			if(iend>g_menu_size)
				iend = g_menu_size;
		}
	}


	for(uint8_t i=istart; i<iend; i++)
	{
		LcdGotoXYFont( 2, ystart+i-istart );
		LcdStr(FONT_1X, g_cur_menu[i].text);
	}

	uint8_t higlight_pos = ystart+g_menu_pos-istart;
	if(higlight_pos<=height)
	{
		if(higlight_pos<5)
			LcdSingleBar( 4, higlight_pos*font_height+1, font_height+1, 6*12+4, PIXEL_XOR );
		else
			LcdSingleBar( 4, higlight_pos*font_height, font_height, 6*12+4, PIXEL_XOR );
	}
}

bool MenuIsOpen()
{
	return g_menu_size!=0 && g_cur_menu!=NULL && !bCalibration;
}

void MenuSetPos(MenuEnum command)
{
	if(g_menu_size==0 || g_cur_menu==NULL)
		return;
	for(uint8_t i=0; i<g_menu_size; i++)
	{
		if(g_cur_menu[i].command==command)
		{
			g_menu_pos = i;
			break;
		}
	}
}

void MenuSetF(uint32_t period)
{
	bContinuousMode = true;
	AdcDacStartSynchro(period, DEFAULT_DAC_AMPLITUDE);
	ProcessStartComputeX(0/*count*/, 
			255/*predefinedResistorIdx*/,
			255/*predefinedGainVoltageIdx*/,
			255/*uint8_t predefinedGainCurrentIdx*/,
			true/*useCorrector*/
		);
	MENU_CLEAR();
}

void MenuSetSerial(bool ser)
{
	isSerial = ser;
	MENU_CLEAR();
}

void MenuSetPrinRim(bool pr)
{
	printRim = pr;
	MENU_CLEAR();
}

void MessageBox(char* line1)
{
	message_line1 = line1;
	message_line2 = NULL;
	g_update = true;
}

void MessageBox2(char* line1, char* line2)
{
	message_line1 = line1;
	message_line2 = line2;
	g_update = true;
}

void MenuClearFlash()
{
	if(CorrectorFlashClearCurrent())
		MessageBox("CLEAR COMPLETE");
	else
		MessageBox("CLEAR FAIL");
	ClearCorrector();
}

void MenuSaveFlash()
{
	if(!CorrectorFlashClearCurrent())
		MessageBox("CLEAR FAIL");

	delay_ms(10);

	if(CorrectorFlashCurrentData())
		MessageBox("SAVE COMPLETE");
	else
		MessageBox("SAVE FAIL");
}


typedef struct CalibrationJob
{
	uint8_t resistorIndex;
	uint8_t VIndex;
	uint8_t IIndex;
	uint8_t ampDiv;
} CalibrationJob;

void OnCalibrationStart(CalibrationJob* job, uint8_t jobCount);
void OnOpenFirstPass();


static CalibrationJob* calJob;
static uint8_t calJobCount;
static uint8_t calCurIndex;
static complexf calResult[20];
static bool calFirstPassOpen = false;

static CalibrationJob calibrateShort[]=
{
    {0, 0, 0, 1},
    {0, 1, 0, 1},
    {0, 2, 0, 1},
    {0, 4, 0, 1},
    {0, 6, 0, 1},
    {0, 7, 0, 1},
};

static CalibrationJob calibrate1Om[]=
{
	{0, 7, 0, 1},
};

static CalibrationJob calibrate100Om[]=
{
	{0, 0, 0, 1},
    {0, 0, 1, 2},

    {0, 1, 0, 2},
    {0, 2, 0, 4},
    {0, 4, 0, 8},
    {0, 6, 0, 16},
};

static CalibrationJob calibrate1KOm[]=
{
    {0, 0, 2, 1},
    {1, 0, 0, 1},
    {1, 0, 1, 2},
};

static CalibrationJob calibrate10KOm[]=
{
    {1, 0, 2, 1},
    {2, 0, 0, 1},
    {2, 0, 1, 2},
};

static CalibrationJob calibrate100KOm[]=
{
    {2, 0, 2, 1},

    {3, 0, 0, 1},
    {3, 0, 1, 2},
    {3, 0, 2, 4},
    {3, 0, 4, 8},
    {3, 0, 6, 16},
    {3, 0, 7, 32},
};

static CalibrationJob calibrateOpen[]=
{
    {0, 0, 0, 1},
    {0, 0, 1, 1},
    {0, 0, 2, 1},

    {1, 0, 0, 1},
    {1, 0, 1, 1},
    {1, 0, 2, 1},

    {2, 0, 0, 1},
    {2, 0, 1, 1},
    {2, 0, 2, 1},

    {3, 0, 0, 1},
    {3, 0, 1, 1},
    {3, 0, 2, 1},
    {3, 0, 4, 1},
    {3, 0, 6, 1},
    {3, 0, 7, 1},
};

void MenuOnCorrection(MenuEnum command)
{
	g_last_correction_command = command;

	CoeffCorrector* corr = GetCorrector();
	if(corr->period==0)
	{
		ClearCorrector();
		corr->period = DacPeriod();
	}

	if(PredefinedPeriodIndex()==255)
	{
		MessageBox2("ERROR", "Bad frequency");
		return;
	}

	switch(command)
	{
	case MENU_CORRECTION_1_Om:
		NumberEditSetText("Value 1 Om");
		NumberEditSetValue(corr->cshort.R1,
			-3, -1);
		NumberEditStart();
		break;	
	case MENU_CORRECTION_100_Om:
		NumberEditSetText("Value 100 Om");
		NumberEditSetValue(corr->cshort.R100,
			-1, 1);
		NumberEditStart();
		break;
	case MENU_CORRECTION_1_KOm:
		NumberEditSetText("Value 1 KOm");
		NumberEditSetValue(corr->x2x[1].R[0], 0, 2);
		NumberEditStart();
		break;
	case MENU_CORRECTION_10_KOm:
		NumberEditSetText("Value 10 KOm");
		NumberEditSetValue(corr->x2x[2].R[0], 1, 3);
		NumberEditStart();
		break;
	case MENU_CORRECTION_100_KOm:
		NumberEditSetText("Value 100 KOm");
		NumberEditSetValue(corr->open.R, 2, 4);
		NumberEditStart();
		break;
	case MENU_CORRECTION_SHORT:
		OnCalibrationStart(calibrateShort, sizeof(calibrateShort)/sizeof(calibrateShort[0]));
		break;
	case MENU_CORRECTION_OPEN:
		OnOpenFirstPass();
		break;	
	case MENU_CORRECTION_SAVE:
		MenuSaveFlash();
		break;
	case MENU_CORRECTION_CLEAR:
		MenuClearFlash();
		break;

	default:;
	}
}

void CalNextJob()
{
	uint32_t period = DacPeriod();
	CalibrationJob* job = calJob+calCurIndex;
	AdcDacStartSynchro(period, DEFAULT_DAC_AMPLITUDE/job->ampDiv);
	ProcessStartComputeX(0/*count*/, 
			job->resistorIndex/*predefinedResistorIdx*/,
			job->VIndex/*predefinedGainVoltageIdx*/,
			job->IIndex/*predefinedGainCurrentIdx*/,
			false/*useCorrector*/
		);
}

void OnCalibrationStart(CalibrationJob* job, uint8_t jobCount)
{
	calJob = job;
	calJobCount = jobCount;
	calCurIndex = 0;

	if(jobCount>sizeof(calResult)/sizeof(calResult[0]))
	{
		MessageBox2("OnCalibrationStart", "ERR jobCount");
		return;
	}

	isSerial = true;
	bCalibration = true;
	printRim = true;
	CalNextJob();
}

static bool FindResult(	uint8_t resistorIndex, uint8_t VIndex, uint8_t IIndex, complexf* result)
{
	for(uint8_t i=0; i<calJobCount; i++)
	{
		CalibrationJob* p = calJob+i;
		if(p->resistorIndex==resistorIndex && p->VIndex==VIndex && p->IIndex==IIndex)
		{
			*result = calResult[i];
			return true;
		}
	}

	return false;
}

void OnOpenFirstPass()
{
	calFirstPassOpen = true;
	isSerial = true;
	bCalibration = true;
	printRim = true;

	GetCorrector()->open.maxGainIndex = 7;
	//	OnCalibrationStart(calibrateOpen, sizeof(calibrateOpen)/sizeof(calibrateOpen[0]));	
	AdcDacStartSynchro(GetCorrector()->period, DEFAULT_DAC_AMPLITUDE);
	ProcessStartComputeX(ProcessCalcOptimalCount()*2/*count*/, 
			255/*predefinedResistorIdx*/,
			255/*predefinedGainVoltageIdx*/,
			255/*uint8_t predefinedGainCurrentIdx*/,
			false/*useCorrector*/
		);
}

void OnSaveCalibrationResult()
{
	CoeffCorrector* corr = GetCorrector();
	if(g_last_correction_command==MENU_CORRECTION_SHORT)
	{
		CoeffCorrectorShort* p = &corr->cshort;
		if(!FindResult(0, 0, 0, &p->Zm[0].Zsm))
			MessageBox2("ERROR Cal", "Open 0");
		if(!FindResult(0, 1, 0, &p->Zm[1].Zsm))
			MessageBox2("ERROR Cal", "Open 1");
		if(!FindResult(0, 2, 0, &p->Zm[2].Zsm))
			MessageBox2("ERROR Cal", "Open 2");
		if(!FindResult(0, 4, 0, &p->Zm[3].Zsm))
			MessageBox2("ERROR Cal", "Open 4");
		if(!FindResult(0, 6, 0, &p->Zm[4].Zsm))
			MessageBox2("ERROR Cal", "Open 6");
		if(!FindResult(0, 7, 0, &p->Zm[5].Zsm))
			MessageBox2("ERROR Cal", "Open 7");
		return;
	}

	if(g_last_correction_command==MENU_CORRECTION_1_Om)
	{
		CoeffCorrectorShort* p = &corr->cshort;
		if(!FindResult(0, 7, 0, &p->Zm[5].Zstdm))
			MessageBox2("ERROR Cal", "1Om 7");
		return;
	}

	if(g_last_correction_command==MENU_CORRECTION_100_Om)
	{
		CoeffCorrectorShort* p = &corr->cshort;
		if(!FindResult(0, 0, 0, &p->Zm[0].Zstdm))
			MessageBox2("ERROR Cal", "100Om 0");
		if(!FindResult(0, 1, 0, &p->Zm[1].Zstdm))
			MessageBox2("ERROR Cal", "100Om 1");
		if(!FindResult(0, 2, 0, &p->Zm[2].Zstdm))
			MessageBox2("ERROR Cal", "100Om 2");
		if(!FindResult(0, 4, 0, &p->Zm[3].Zstdm))
			MessageBox2("ERROR Cal", "100Om 4");
		if(!FindResult(0, 6, 0, &p->Zm[4].Zstdm))
			MessageBox2("ERROR Cal", "100Om 6");

		if(!FindResult(0, 0, 0, &corr->x2x[0].Zm[0].Zstdm))
			MessageBox2("ERROR Cal", "100Om 00");
		if(!FindResult(0, 0, 1, &corr->x2x[0].Zm[1].Zstdm))
			MessageBox2("ERROR Cal", "100Om 01");
		return;
	}

	if(g_last_correction_command==MENU_CORRECTION_1_KOm)
	{
		if(!FindResult(0, 0, 2, &corr->x2x[0].Zm[2].Zstdm))
			MessageBox2("ERROR Cal", "1KOm 00");
		if(!FindResult(1, 0, 0, &corr->x2x[1].Zm[0].Zstdm))
			MessageBox2("ERROR Cal", "1KOm 00");
		if(!FindResult(1, 0, 1, &corr->x2x[1].Zm[1].Zstdm))
			MessageBox2("ERROR Cal", "1KOm 01");
	}

	if(g_last_correction_command==MENU_CORRECTION_10_KOm)
	{
		if(!FindResult(1, 0, 2, &corr->x2x[1].Zm[2].Zstdm))
			MessageBox2("ERROR Cal", "10KOm 00");
		if(!FindResult(2, 0, 0, &corr->x2x[2].Zm[0].Zstdm))
			MessageBox2("ERROR Cal", "10KOm 00");
		if(!FindResult(2, 0, 1, &corr->x2x[2].Zm[1].Zstdm))
			MessageBox2("ERROR Cal", "10KOm 01");
	}

	if(g_last_correction_command==MENU_CORRECTION_100_KOm)
	{
		if(!FindResult(2, 0, 2, &corr->x2x[2].Zm[2].Zstdm))
			MessageBox2("ERROR Cal", "100KOm 00");

		for(uint8_t IIndex=0; IIndex<8; IIndex++)
		{
			int8_t idx = GetGainValidIdx(IIndex);
			if(idx<0)
				continue;
			if(!FindResult(3, 0, IIndex, &corr->open.Zm[idx].Zstdm))
				MessageBox2("ERROR Cal", "100 KOm 1");
		}
	}

	if(g_last_correction_command==MENU_CORRECTION_OPEN)
	{
		for(uint8_t resistorIndex=0; resistorIndex<3; resistorIndex++)
		for(uint8_t IIndex=0; IIndex<3; IIndex++)
		{
			if(!FindResult(resistorIndex, 0, IIndex, &corr->x2x[resistorIndex].Zm[IIndex].Zom))
				MessageBox2("ERROR Cal", "Open");
		}

		for(uint8_t IIndex=0; IIndex<8; IIndex++)
		{
			int8_t idx = GetGainValidIdx(IIndex);
			if(idx<0)
				continue;
			if(!FindResult(3, 0, IIndex, &corr->open.Zm[idx].Zom))
				MessageBox2("ERROR Cal", "Open 1");
		}
	}
	
}

void OnCalibrationComplete()
{
	if(calFirstPassOpen)
	{
		calFirstPassOpen = false;
		GetCorrector()->open.maxGainIndex = gainCurrentIdx;

		if(false)
		{//debug code
			static char buf[]="0";
			buf[0]='0'+gainCurrentIdx;
			MessageBox2("OPEN", buf);
			bCalibration = false;
		} else
		{
			OnCalibrationStart(calibrateOpen, sizeof(calibrateOpen)/sizeof(calibrateOpen[0]));
		}
		return;
	}

	calResult[calCurIndex] = Rre + Rim*I;

	calCurIndex++;
	if(calCurIndex<calJobCount)
	{
		CalNextJob();
	} else
	{
		OnSaveCalibrationResult();
		bCalibration = false;
		MessageBox("COMPLETE");
	}
}

void OnNumberEditEnd()
{
	CoeffCorrector* corr = GetCorrector();
	switch(g_last_correction_command)
	{
	case MENU_CORRECTION_1_Om:
		corr->cshort.R1 = NumberEditGetValue();
		OnCalibrationStart(calibrate1Om, sizeof(calibrate1Om)/sizeof(calibrate1Om[0]));
		break;	
	case MENU_CORRECTION_100_Om:
		corr->cshort.R100 =
		corr->x2x[0].R[0] =
		corr->x2x[0].R[1] =
			NumberEditGetValue();
		OnCalibrationStart(calibrate100Om, sizeof(calibrate100Om)/sizeof(calibrate100Om[0]));
		break;
	case MENU_CORRECTION_1_KOm:
		corr->x2x[0].R[2] =
		corr->x2x[1].R[0] =
		corr->x2x[1].R[1] =
			NumberEditGetValue();
		OnCalibrationStart(calibrate1KOm, sizeof(calibrate1KOm)/sizeof(calibrate1KOm[0]));
		break;
	case MENU_CORRECTION_10_KOm:
		corr->x2x[1].R[2] =
		corr->x2x[2].R[0] =
		corr->x2x[2].R[1] =
			NumberEditGetValue();
		OnCalibrationStart(calibrate10KOm, sizeof(calibrate10KOm)/sizeof(calibrate10KOm[0]));
		break;
	case MENU_CORRECTION_100_KOm:
		corr->x2x[2].R[2] =
		corr->open.R = 
			NumberEditGetValue();
		OnCalibrationStart(calibrate100KOm, sizeof(calibrate100KOm)/sizeof(calibrate100KOm[0]));
		break;
	default:
		MessageBox2("NumberEditEnd", "Bad command");	
		break;
	}
}