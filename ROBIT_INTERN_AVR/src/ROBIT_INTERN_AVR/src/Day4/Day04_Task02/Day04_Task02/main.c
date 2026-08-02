/*
 * Day04_Task02.c
 *
 * 가변저항으로 날짜와 시간을 설정하고 LCD에 표시한다.
 * SW1은 다음 설정 항목으로 넘어가고, SW2는 설정이 끝난 뒤 시계를 시작한다.
 *
 * 가변저항 : PF0
 * SW1      : PE4
 * SW2      : PE5
 * LCD      : PD0(SCL), PD1(SDA)
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "LCD_Text.h"

typedef enum
{
    SET_YEAR = 0,
    SET_MONTH,
    SET_DAY,
    SET_HOUR,
    SET_MINUTE,
    SET_SECOND,
    SET_MILLISECOND,
    SET_FINISHED
} SettingStep;

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    /* LCD 마지막 두 자리 값 */
    uint8_t centisecond;
} DateTime;

volatile DateTime clock_time =
{
    2026, 1, 1,
    0, 0, 0,
    0
};

volatile uint8_t clock_running = 0;
volatile uint8_t lcd_update = 1;

SettingStep setting_step = SET_YEAR;

static void switchInit(void);
static uint8_t switchPressed(uint8_t pin);

static void adcInit(void);
static uint16_t adcRead(void);
static uint16_t adcToRange(uint16_t adc_value,
                           uint16_t minimum,
                           uint16_t maximum);

static void timer0Init(void);

static uint8_t isLeapYear(uint16_t year);
static uint8_t getLastDay(uint16_t year, uint8_t month);
static void clampDay(void);
static void increaseOneSecond(void);

static void updateSettingValue(void);
static void confirmSetting(void);

static void copyClockTime(DateTime *destination);
static void displayClock(void);
static void fillLine(char *line);
static void putTwoDigits(char *line, uint8_t position, uint8_t value);
static const char *getStepText(void);

int main(void)
{
    switchInit();
    adcInit();
    timer0Init();
    lcdInit();

    lcdClear();
    sei();

    while (1)
    {
        if (setting_step != SET_FINISHED)
        {
            updateSettingValue();

            if (switchPressed(PE4))
            {
                confirmSetting();
                lcd_update = 1;
            }
        }
        else
        {
            if (!clock_running && switchPressed(PE5))
            {
                uint8_t old_sreg = SREG;

                cli();
                clock_running = 1;
                SREG = old_sreg;

                lcd_update = 1;
            }
        }

        if (lcd_update)
        {
            lcd_update = 0;
            displayClock();
        }

        _delay_ms(1);
    }
}

static void switchInit(void)
{
    /* 스위치 입력과 내부 풀업 설정 */
    DDRE &= ~((1 << PE4) | (1 << PE5));
    PORTE |= (1 << PE4) | (1 << PE5);
}

static uint8_t switchPressed(uint8_t pin)
{
    /* 눌림 한 번만 처리하도록 디바운싱 */
    if (!(PINE & (1 << pin)))
    {
        _delay_ms(20);

        if (!(PINE & (1 << pin)))
        {
            while (!(PINE & (1 << pin)))
            {
            }

            _delay_ms(20);
            return 1;
        }
    }

    return 0;
}

static void adcInit(void)
{
    /* PF0을 ADC 입력으로 사용 */
    DDRF &= ~(1 << PF0);
    PORTF &= ~(1 << PF0);

    ADMUX = (1 << REFS0);

    ADCSRA =
        (1 << ADEN) |
        (1 << ADPS2) |
        (1 << ADPS1) |
        (1 << ADPS0);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }
}

static uint16_t adcRead(void)
{
    /* ADC0 값 한 번 읽기 */
    ADMUX &= 0xE0;

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}

static uint16_t adcToRange(uint16_t adc_value,
                           uint16_t minimum,
                           uint16_t maximum)
{
    uint32_t count = (uint32_t)maximum - minimum + 1;

    return minimum + ((uint32_t)adc_value * count / 1024UL);
}

static void timer0Init(void)
{
    /* 1ms마다 비교일치 인터럽트가 발생하도록 설정 */
    TCCR0 = 0x00;
    TCNT0 = 0x00;
    OCR0 = 249;

    TIFR = (1 << OCF0);
    TIMSK |= (1 << OCIE0);

    TCCR0 = (1 << WGM01) | (1 << CS01) | (1 << CS00);
}

static uint8_t isLeapYear(uint16_t year)
{
    /* 윤년 여부 확인 */
    if ((year % 400) == 0)
    {
        return 1;
    }

    if ((year % 100) == 0)
    {
        return 0;
    }

    return ((year % 4) == 0);
}

static uint8_t getLastDay(uint16_t year, uint8_t month)
{
    /* 해당 달의 마지막 날짜 반환 */
    switch (month)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;

        case 4:
        case 6:
        case 9:
        case 11:
            return 30;

        case 2:
            return isLeapYear(year) ? 29 : 28;

        default:
            return 31;
    }
}

static void clampDay(void)
{
    /* 월이 바뀌었을 때 날짜 범위를 맞춘다. */
    uint8_t last_day = getLastDay(clock_time.year, clock_time.month);

    if (clock_time.day > last_day)
    {
        clock_time.day = last_day;
    }

    if (clock_time.day < 1)
    {
        clock_time.day = 1;
    }
}

static void increaseOneSecond(void)
{
    /* 1초 증가 후 날짜까지 올림 처리 */
    clock_time.second++;

    if (clock_time.second < 60)
    {
        return;
    }

    clock_time.second = 0;
    clock_time.minute++;

    if (clock_time.minute < 60)
    {
        return;
    }

    clock_time.minute = 0;
    clock_time.hour++;

    if (clock_time.hour < 24)
    {
        return;
    }

    clock_time.hour = 0;
    clock_time.day++;

    if (clock_time.day <= getLastDay(clock_time.year, clock_time.month))
    {
        return;
    }

    clock_time.day = 1;
    clock_time.month++;

    if (clock_time.month <= 12)
    {
        return;
    }

    clock_time.month = 1;
    clock_time.year++;

    if (clock_time.year > 2099)
    {
        clock_time.year = 2000;
    }
}

static void updateSettingValue(void)
{
    /* 현재 설정 항목을 가변저항 값에 맞춰 갱신 */
    static SettingStep previous_step = SET_FINISHED;
    static uint16_t previous_value = 0xFFFF;

    uint16_t adc_value;
    uint16_t selected_value;
    uint8_t last_day;

    if (previous_step != setting_step)
    {
        previous_step = setting_step;
        previous_value = 0xFFFF;
    }

    adc_value = adcRead();

    switch (setting_step)
    {
        case SET_YEAR:
            selected_value = adcToRange(adc_value, 2000, 2099);

            if (selected_value != previous_value)
            {
                clock_time.year = selected_value;
                clampDay();
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_MONTH:
            selected_value = adcToRange(adc_value, 1, 12);

            if (selected_value != previous_value)
            {
                clock_time.month = (uint8_t)selected_value;
                clampDay();
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_DAY:
            last_day = getLastDay(clock_time.year, clock_time.month);
            selected_value = adcToRange(adc_value, 1, last_day);

            if (selected_value != previous_value)
            {
                clock_time.day = (uint8_t)selected_value;
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_HOUR:
            selected_value = adcToRange(adc_value, 0, 23);

            if (selected_value != previous_value)
            {
                clock_time.hour = (uint8_t)selected_value;
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_MINUTE:
            selected_value = adcToRange(adc_value, 0, 59);

            if (selected_value != previous_value)
            {
                clock_time.minute = (uint8_t)selected_value;
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_SECOND:
            selected_value = adcToRange(adc_value, 0, 59);

            if (selected_value != previous_value)
            {
                clock_time.second = (uint8_t)selected_value;
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_MILLISECOND:
            /* 소수점 뒤 두 자리 값을 설정 */
            selected_value = adcToRange(adc_value, 0, 99);

            if (selected_value != previous_value)
            {
                clock_time.centisecond = (uint8_t)selected_value;
                previous_value = selected_value;
                lcd_update = 1;
            }
            break;

        case SET_FINISHED:
        default:
            break;
    }
}

static void confirmSetting(void)
{
    if (setting_step < SET_FINISHED)
    {
        setting_step++;
    }

    if (setting_step == SET_FINISHED)
    {
        /* 모든 설정이 끝났으므로 SW2 입력을 기다린다. */
        clock_running = 0;
    }
}

static void copyClockTime(DateTime *destination)
{
    /* 인터럽트 중 값이 바뀌지 않도록 복사 */
    uint8_t old_sreg = SREG;

    cli();

    destination->year = clock_time.year;
    destination->month = clock_time.month;
    destination->day = clock_time.day;

    destination->hour = clock_time.hour;
    destination->minute = clock_time.minute;
    destination->second = clock_time.second;
    destination->centisecond = clock_time.centisecond;

    SREG = old_sreg;
}

static void fillLine(char *line)
{
    /* LCD 한 줄을 공백으로 채운다. */
    uint8_t i;

    for (i = 0; i < 16; i++)
    {
        line[i] = ' ';
    }

    line[16] = '\0';
}

static void putTwoDigits(char *line, uint8_t position, uint8_t value)
{
    /* 숫자를 항상 두 자리로 넣는다. */
    line[position] = (value / 10) + '0';
    line[position + 1] = (value % 10) + '0';
}

static const char *getStepText(void)
{
    /* 현재 설정 상태에 맞는 문구 반환 */
    if (clock_running)
    {
        return "RUN ";
    }

    switch (setting_step)
    {
        case SET_YEAR:
            return "YEAR";

        case SET_MONTH:
            return "MON ";

        case SET_DAY:
            return "DAY ";

        case SET_HOUR:
            return "HOUR";

        case SET_MINUTE:
            return "MIN ";

        case SET_SECOND:
            return "SEC ";

        case SET_MILLISECOND:
            return "MSEC";

        case SET_FINISHED:
            return "WAIT";

        default:
            return "    ";
    }
}

static void displayClock(void)
{
    /* 날짜와 시간을 LCD 형식에 맞춰 출력 */
    DateTime now;
    char first_line[17];
    char second_line[17];
    const char *step_text;

    copyClockTime(&now);

    fillLine(first_line);
    fillLine(second_line);

    putTwoDigits(first_line, 0, (uint8_t)(now.year % 100));
    putTwoDigits(first_line, 2, now.month);
    putTwoDigits(first_line, 4, now.day);

    step_text = getStepText();

    first_line[7] = step_text[0];
    first_line[8] = step_text[1];
    first_line[9] = step_text[2];
    first_line[10] = step_text[3];

    putTwoDigits(second_line, 0, now.hour);
    second_line[2] = ':';

    putTwoDigits(second_line, 3, now.minute);
    second_line[5] = ':';

    putTwoDigits(second_line, 6, now.second);
    second_line[8] = '.';

    putTwoDigits(second_line, 9, now.centisecond);

    lcdString(0, 0, first_line);
    lcdString(1, 0, second_line);
}

ISR(TIMER0_COMP_vect)
{
    /* 10ms마다 소수점 뒤 값을 증가시킨다. */
    static uint8_t one_ms_count = 0;

    if (!clock_running)
    {
        one_ms_count = 0;
        return;
    }

    one_ms_count++;

    if (one_ms_count >= 10)
    {
        one_ms_count = 0;

        clock_time.centisecond++;
        lcd_update = 1;

        if (clock_time.centisecond >= 100)
        {
            clock_time.centisecond = 0;
            increaseOneSecond();
        }
    }
}