/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/

#include <msp430.h>
#include<stdio.h>
#include<string.h>
/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"

#define CAL30 *((unsigned int *) 0x1A1A)
#define CAL85 *((unsigned int *) 0x1A1C)

// Function Prototypes
void runtimerA2(void);
void stoptimerA2(int reset);
void configUserLEDs();
void configADCTemp();
void configADCWheel();
void configUserButtons();
int returnState();
char * numberToMonth(int num);
void convertToGlobal();
void displayTime();
void sync();
char * changeType(int num);
char * floatToString(float num, char * ret);
void rev(char * s);


unsigned int in_temp;

enum state{DISPLAY, SETUP};
unsigned long int modifiedMonth, modifiedDate, modifiedHour, modifiedMinutes, modifiedSeconds;
unsigned long int day, hour,min,sec, monthNumber;
char monthDay[10];
char time[10];
bool updateTemp = true;


//unsigned long int timer_cnt = 619200;
unsigned long int timer_cnt = 0;
unsigned long int globalTime = 23562231;
// Declare globals here

// Main
void main(void)

{

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired
    unsigned int in_value;
    _BIS_SR(GIE);
    configUserLEDs();
    runtimerA2();
    configADCTemp();

    // Useful code starts here
    initLeds();

    configDisplay();
    configKeypad();
    configUserButtons();
    //timer_cnt = 691200;

    volatile float tempC, tempF, degC_per_bit;
    enum state currentState;
    int modifyIndex = 0;
    char disp[5];

    float tempCel[10];
    float tempFar[10];
    int tempIndex = 0;
    float sumC = 0;
    float sumF = 0;
    float avgC = 0;
    float avgF = 0;
    int divider = 0;


    int q = 0;
    for(q = 0; q < 10; q++){
        tempCel[q] = 0;
    }
    q = 0;
    for(q = 0; q < 10; q++){
        tempFar[q] = 0;
    }




    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext); // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "ECE2049-D19!", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = {.xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    // We are now done writing to the display.  However, if we stopped here, we would not
    // see any changes on the actual LCD.  This is because we need to send our changes
    // to the LCD, which then refreshes the display.
    // Since this is a slow operation, it is best to refresh (or "flush") only after
    // we are done drawing everything we need.
    Graphics_flushBuffer(&g_sContext);

    degC_per_bit = ((float) 55.0)/((float) CAL85 - CAL30);
    currentState = DISPLAY;
    while (1)    // Forever loop
    {
        int press = returnState();
        if(press == 1){

            if(modifyIndex > 0){
                if(modifyIndex == 5){
                    modifyIndex = 1;
                }
                else{
                    modifyIndex++;
                }
            }
            else{
                currentState = SETUP;
                modifyIndex++;
                sync();
                configADCWheel();
            }
        }
        else if(press == 2){
            currentState = DISPLAY;
            modifyIndex = 0;
            convertToGlobal();
            configADCTemp();
        }
        switch(currentState){
        case DISPLAY:

            displayTime();

            ADC12CTL0 &= ~ADC12SC;
            ADC12CTL0 |= ADC12SC;
            while ((ADC12CTL1 & ADC12BUSY) && updateTemp) {
                __no_operation();
            }
            updateTemp = false;
            in_temp = ADC12MEM0;
            tempC = (float)(((long)in_temp - CAL30)*degC_per_bit) + 30.0; //check this line if problems
            tempF = tempC * 9.0 / 5.0 + 32.0;

            sumC -= tempCel[tempIndex];
            sumF -= tempFar[tempIndex];
            tempCel[tempIndex] = tempC;
            tempFar[tempIndex] = tempF;
            sumC += tempCel[tempIndex];
            sumF += tempFar[tempIndex];

            if(divider < 10){
                divider++;
            }

            avgC = sumC/divider;
            avgF = sumF/divider;

            tempIndex = (tempIndex + 1) % 10;

            char ret[10];




            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, monthDay, AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, time, AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
            floatToString(avgC, ret);
            Graphics_drawStringCentered(&g_sContext, ret, AUTO_STRING_LENGTH, 48, 55, TRANSPARENT_TEXT);
            floatToString(avgF, ret);
            Graphics_drawStringCentered(&g_sContext, ret, AUTO_STRING_LENGTH, 48, 75, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            break;

        case SETUP:


            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, changeType(modifyIndex), AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);



            ADC12CTL0 |= ADC12SC;

            while (ADC12CTL1 & ADC12BUSY) {
                __no_operation();
            }
            in_value = ADC12MEM0 & 0X0FFF;

            if(modifyIndex == 1){
                modifiedMonth = in_value/341 + 1;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                Graphics_drawStringCentered(&g_sContext, numberToMonth(in_value/341 + 1), AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
                Graphics_flushBuffer(&g_sContext);
            }
            else if(modifyIndex == 2){
                modifiedDate = in_value/132 + 1;
                if(modifiedDate < 10){
                    disp[0] = '0';
                    disp[1] = '0' + modifiedDate;
                    disp[2] = '\0';
                }
                else{
                    disp[0] = '0' + modifiedDate/10;
                    disp[1] = '0' + modifiedDate%10;
                    disp[2] = '\0';
                }
                Graphics_clearDisplay(&g_sContext); // Clear the display
                Graphics_drawStringCentered(&g_sContext, disp, AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
                Graphics_flushBuffer(&g_sContext);
            }
            else if(modifyIndex == 3){
                modifiedHour = in_value/170 + 1;
                if(modifiedHour < 10){
                    disp[0] = '0';
                    disp[1] = '0' + modifiedHour;
                    disp[2] = '\0';
                }
                else{
                    disp[0] = '0' + modifiedHour/10;
                    disp[1] = '0' + modifiedHour%10;
                    disp[2] = '\0';
                }

                Graphics_clearDisplay(&g_sContext); // Clear the display
                Graphics_drawStringCentered(&g_sContext, disp, AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
                Graphics_flushBuffer(&g_sContext);
            }
            else if(modifyIndex == 4){
                modifiedMinutes = in_value/68 + 1;
                if(modifiedMinutes < 10){
                    disp[0] = '0';
                    disp[1] = '0' + modifiedMinutes;
                    disp[2] = '\0';
                }
                else{
                    disp[0] = '0' + modifiedMinutes/10;
                    disp[1] = '0' + modifiedMinutes%10;
                    disp[2] = '\0';
                }
                Graphics_clearDisplay(&g_sContext); // Clear the display
                Graphics_drawStringCentered(&g_sContext, disp, AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
                Graphics_flushBuffer(&g_sContext);
            }
            else if(modifyIndex == 5){
                modifiedSeconds = in_value/68 + 1;
                if(modifiedSeconds < 10){
                    disp[0] = '0';
                    disp[1] = '0' + modifiedSeconds;
                    disp[2] = '\0';
                }
                else{
                    disp[0] = '0' + modifiedSeconds/10;
                    disp[1] = '0' + modifiedSeconds%10;
                    disp[2] = '\0';
                }
                Graphics_clearDisplay(&g_sContext); // Clear the display
                Graphics_drawStringCentered(&g_sContext, disp, AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
                Graphics_flushBuffer(&g_sContext);
            }
            break;
        }


        //ADC12CTL0 &= ~ADC12SC;
        ADC12CTL0 |= ADC12SC;

        while (ADC12CTL1 & ADC12BUSY) {
            __no_operation();
        }
        /*in_temp = ADC12MEM0;
        tempC = (float)(((long)in_temp - CAL30)*degC_per_bit) + 30.0; //check this line if problems
        tempF = tempC * 9.0 / 5.0 + 32.0;*/
        in_value = ADC12MEM0 & 0X0FFF;

        __no_operation();



    }  // end while (1)
}


void configUserLEDs() {
    P1SEL = P1SEL & ~(BIT0);
    P4SEL = P4SEL & ~(BIT7);

    P4DIR = P4DIR | (BIT7);
    P1DIR = P1DIR | (BIT0);

}

void configADCTemp() {
    REFCTL0 &= ~REFMSTR;
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;
    __delay_cycles(100);
    ADC12CTL0 |= ADC12ENC;
}

void configUserButtons() {
    P2SEL = P2SEL & ~(BIT1);
    P1SEL = P1SEL & ~(BIT1);

    P2DIR = P2DIR & ~(BIT1);
    P1DIR = P1DIR & ~(BIT1);

    P2REN = P2REN | (BIT1);
    P1REN = P1REN | (BIT1);

    P2OUT = P2OUT | (BIT1);
    P1OUT = P1OUT | (BIT1);

}

int returnState(){
    if(~P2IN & BIT1){
        return 1;
    }
    else if(~P1IN & BIT1){
        return 2;
    }
    return 0;
}

void convertToGlobal(){
    unsigned long int temp_cnt = 0;
    //timer_cnt = 0;
    unsigned long int monthDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int i;
    for(i = 0; i < modifiedMonth - 1;i++){
        temp_cnt += monthDays[i] * 86400;
    }

    temp_cnt += modifiedDate * 24 * 60 * 60;
    temp_cnt += modifiedHour * 60 * 60;
    temp_cnt += modifiedMinutes  * 60;
    temp_cnt += modifiedSeconds;
    globalTime = temp_cnt;
}

char * numberToMonth(int num){
    switch (num){
        case 1: return "JAN";
        case 2: return "FEB";
        case 3: return "MAR";
        case 4: return "APR";
        case 5: return "MAY";
        case 6: return "JUN";
        case 7: return "JUL";
        case 8: return "AUG";
        case 9: return "SEP";
        case 10: return "OCT";
        case 11: return "NOV";
        case 12: return "DEC";
    }
    return "None";
}

char * changeType(int num){
    switch (num){
        case 1: return "Month";
        case 2: return "Date";
        case 3: return "Hour";
        case 4: return "Minute";
        case 5: return "Second";
    }
    return "None";
}

void configADCWheel() {
    REFCTL0 &= ~REFMSTR;
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0;
    __delay_cycles(100);
    ADC12CTL0 |= ADC12ENC;
    P6SEL |= BIT0;
}

void runtimerA2(void) {
    // This function configures and starts Timer A2
    // Timer is counting ~0.01 seconds //
    // Input: none, Output: none //
    // smj, ECE2049, 17 Sep 2013 //
    // Use ACLK, 16 Bit, up mode, 1 divider
    TA2CTL = TASSEL_1 + MC_1 + ID_0;
    TA2CCR0 = 32767;       // 32767+1 = 32768 ACLK tics = 1 seconds
    TA2CCTL0 = CCIE;     // TA2CCR0 interrupt enabled
}


void stoptimerA2(int reset) {
    // This function stops Timer A2 and resets the global time variable
    // if input reset = 1 //
    // Input: reset, Output: none //
    // smj, ECE2049, 17 Sep 2013 //
    TA2CTL = MC_0;        // stop timer
    TA2CCTL0 &= ~CCIE;    // TA2CCR0 interrupt disabled
    if(reset)
        timer_cnt=0;
}

// Timer A2 interrupt service routine
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR (void) {
        timer_cnt++;
        updateTemp = true;
        P1OUT = P1OUT ^ BIT0;
        P4OUT ^= BIT7;
}

void sync(){
    modifiedMonth = monthNumber;
    modifiedDate = day;
    modifiedHour = hour;
    modifiedMinutes = min;
    modifiedSeconds = sec;
}

void rev(char * s){
    char r[6];
    int begin, end,count = 0;
    while(s[count] != '\0'){
        count++;
    }
    end = count - 1;
    for(begin = 0; begin < count; begin++){
        r[begin] = s[end];
        end--;
    }

    r[begin] = '\0';
    strcpy(s, r);
}

char * floatToString(float num, char * ret){
    int x = (int) num;
    float rem = num - x;
    int firstAfterDot = (int)(rem * 10);
    char arr[6];
    int i = 0;
    while (x)
    {
        arr[i] = (x%10) + '0';
        x = x/10;
        i++;
    }
    arr[i] = '\0';
    rev(arr);
    arr[i++] = '.';
    arr[i++] = '0' + firstAfterDot;
    arr[i] = '\0';
    strcpy(ret,arr);
    return ret;
}

void displayTime() {
    // inTime should be passed as a copy becasue the original variable is linked to the timer //             and updates every second;
    unsigned long int tempCnt = globalTime;
    char dayString[4];
    char timeTemp[4];
    int monthDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned long int days;
    days = tempCnt/(86400);
    unsigned long int temp = 0;
    int i;
    for (i = 0; i < 12; i ++){
        temp += monthDays[i];
        if(temp >= days){
            monthNumber = i+1;
            strcpy(monthDay,numberToMonth(monthNumber));
            if(temp == days){
                day = monthDays[i];
            }
            else{
                day = monthDays[i] - (temp - days);
            }
            break;
        }
    }
    temp = tempCnt%(86400);
    hour = temp / (60*60);
    temp = temp % (60*60);
    min = temp / 60;
    sec = temp % 60;

    if(day < 10){
        dayString[0] = ' ';
        dayString[1] = '0';
        dayString[2] = '0' + day;
        dayString[3] = '\0';
    }
    else{
        dayString[0] = ' ';
        dayString[1] = '0' + day/10;
        dayString[2] = '0' + day%10;
        dayString[3] = '\0';
    }
    strcat(monthDay, dayString);

    if(hour < 10){
        timeTemp[0] = '0';
        timeTemp[1] = '0' + hour;
        timeTemp[2] = '\0';
    }
    else{
        timeTemp[0] = '0' + hour/10;
        timeTemp[1] = '0' + hour%10;
        timeTemp[2] = '\0';
    }
    strcpy(time, timeTemp);

    if(min < 10){
        timeTemp[0] = ':';
        timeTemp[1] = '0';
        timeTemp[2] = '0' + min;
        timeTemp[3] = '\0';
    }
    else{
        timeTemp[0] = ':';
        timeTemp[1] = '0' + min/10;
        timeTemp[2] = '0' + min%10;
        timeTemp[3] = '\0';
    }
    strcat(time, timeTemp);

    if(sec < 10){
        timeTemp[0] = ':';
        timeTemp[1] = '0';
        timeTemp[2] = '0' + sec;
        timeTemp[3] = '\0';
    }
    else{
        timeTemp[0] = ':';
        timeTemp[1] = '0' + sec/10;
        timeTemp[2] = '0' + sec%10;
        timeTemp[3] = '\0';
    }
    strcat(time, timeTemp);


    // Doing LCD Display using these strings
}




