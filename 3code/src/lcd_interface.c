// balmer@inbox.ru RLC Meter 303
// 2013-2014

#include "hw_config.h"
#include <math.h>
#include "lcd_interface.h"
#include "format_print.h"
#include "calc_rc.h"
#include "dac.h"
#include "menu.h"
#include "vbat.h"

int printD = 0; //debug
float printGradus = -1000;

float Rre = 0;
float Rim = 0;

bool printRim = false; 
bool isSerial = true;
bool valueIsC = true;
bool calculatedValues = false;
float valueL = 0;
float valueC = 0;


void printLcdFrequency()
{
    float f = DacFrequency();
    if(f<999.5f)
    {
        printInt(round(f), FONT_1X);
        LcdStr(FONT_1X, "Hz");
        return;
    }

    if(f<1e6f)
    {
        int32_t fi = round(f*1e-3f);
        if(fi*1000==f)
        {
            printInt(fi, FONT_1X);
        } else
        {
            printIntFixed(round(f*1e-2f), FONT_1X, 2, 1);
        }

        LcdStr(FONT_1X, "KHz");
        return;
    }

    printF(f);
}

void printLcdGradus()
{
    float f = printGradus;
    LcdStr(FONT_1X, "D=");
    if(f>-100 && f<+100)
    {
        printIntFixed(round(f*10), FONT_1X, 2, 1);
        LcdStr(FONT_1X, "{");
        return;
    } else
    {
        LcdStr(FONT_1X, "---");
    }
}

void LcdRepaint()
{
    LcdClear();
    if(MenuIsOpen())
    {
        MenuRepaint();
        LcdUpdate();
        return;
    }

    LcdGotoXYFont(1,1);
    printLcdFrequency();

    LcdGotoXYFont(8, 1);
    LcdStr(FONT_1X, isSerial?"SER":"PAR");


    if(calculatedValues)
    {
        printRX2(Rre, 2);
        LcdGotoXYFont(12,2);
        LcdStr(FONT_1X, "Rre");
        
        if(printRim)
        {
            printRX2(Rim, 4);
            LcdGotoXYFont(12,4);
            LcdStr(FONT_1X, "Rim");
        } else
        {
            if(valueIsC)
            {
                printCX2(valueC, 4);
                LcdGotoXYFont(12,4);
                LcdStr(FONT_1X, "C");
            } else
            {
                printLX2(valueL, 4);
                LcdGotoXYFont(12,4);
                LcdStr(FONT_1X, "L");
            }
        }
    }

    VBatQuant();

    LcdGotoXYFont(1,6);
    //LcdStr(FONT_1X, "D="); printInt(printD, FONT_1X);
    printLcdGradus();

    LcdGotoXYFont(9,6);
    LcdStr(FONT_1X, "R");
    printInt(resistorIdx, FONT_1X);
    LcdStr(FONT_1X, "V");
    printInt(gainVoltageIdx, FONT_1X);  
    LcdStr(FONT_1X, "I");
    printInt(gainCurrentIdx, FONT_1X);

    LcdUpdate();
}
