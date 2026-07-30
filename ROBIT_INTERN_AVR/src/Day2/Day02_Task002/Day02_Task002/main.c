/*
 * Day02_Task002.c
 *
 * Author : chang
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "LCD_Text.h"

void adcInit(void);
unsigned int adcRead(void);

int main(void)
{
    unsigned int adcValue;
    unsigned int voltage;
    unsigned char ledPosition;

    DDRA = 0xFF; // LED 포트를 출력으로 설정
    PORTA = 0xFF; // LED 모두 끄기
    DDRF &= ~(1 << PF0); // PF0를 ADC 입력으로 설정

    adcInit(); // ADC 초기화
    lcdInit(); // LCD 초기화
    lcdClear(); // LCD 화면 지우기

    lcdString(0, 0, "JCG"); // 첫 번째 줄에 이니셜 표시
    lcdString(1, 0, "0      0.0V"); // 두 번째 줄에 초기값 표시

    while (1)
    {
        adcValue = adcRead(); // 가변저항의 ADC 값 읽기
        ledPosition = adcValue / 128; // ADC 값을 0부터 7까지로 변환
        voltage = ((unsigned long)adcValue * 50) / 1023; // 전압을 0.1V 단위로 계산

        PORTA = ~(1 << ledPosition); // ADC 값에 따라 LED 이동

        lcdString(1, 0, "    "); // 이전 ADC 값 지우기
        lcdNumber(1, 0, adcValue); // ADC 값 표시

        lcdNumber(1, 7, voltage / 10); // 전압 정수 부분 표시
        lcdNumber(1, 9, voltage % 10); // 전압 소수 첫째 자리 표시

        _delay_ms(100); // 출력 갱신 간격
    }
}

void adcInit(void)
{
    ADMUX = (1 << REFS0); // AVCC 기준 전압 및 ADC0 선택
    ADCSRA = (1 << ADEN) | (1 << ADPS2) |
             (1 << ADPS1) | (1 << ADPS0); // ADC 활성화 및 분주비 128 설정
}

unsigned int adcRead(void)
{
    ADMUX = (ADMUX & 0xE0) | 0x00; // PF0에 해당하는 ADC0 선택
    ADCSRA |= (1 << ADSC); // ADC 변환 시작

    while (ADCSRA & (1 << ADSC)) // ADC 변환 완료까지 대기
    {
    }

    return ADC; // ADC 값 반환
}