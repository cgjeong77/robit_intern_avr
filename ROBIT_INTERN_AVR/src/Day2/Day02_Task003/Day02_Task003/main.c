/*
 * Day02_Task003.c
 *
 * Author : chang
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "LCD_Text.h"

void lcdPrint(int a, int b, char op, int result);

int main(void)
{
    int a = 1;
    int b = 1;
    int result = 0;
    unsigned char opIndex = 0;
    char opList[4] = {'+', '-', '*', '/'};

    DDRE &= ~((1 << PE4) | (1 << PE5)); // 1번과 2번 스위치를 입력으로 설정
    PORTE |= (1 << PE4) | (1 << PE5); // 1번과 2번 스위치 풀업 설정

    DDRD &= ~((1 << PD2) | (1 << PD3)); // 3번과 4번 스위치를 입력으로 설정
    PORTD |= (1 << PD2) | (1 << PD3); // 3번과 4번 스위치 풀업 설정

    lcdInit(); // LCD 초기화
    lcdClear(); // LCD 화면 지우기

    lcdPrint(a, b, opList[opIndex], result); // 초기 계산식 표시

    while (1)
    {
        if (!(PINE & (1 << PE4))) // 1번 스위치 확인
        {
            _delay_ms(20); // 스위치 채터링 방지

            if (!(PINE & (1 << PE4)))
            {
                a++; // A값 1 증가
                result = 0; // 결과값 초기화
                lcdPrint(a, b, opList[opIndex], result);

                while (!(PINE & (1 << PE4)))
                {
                }

                _delay_ms(20); // 스위치 채터링 방지
            }
        }

        if (!(PINE & (1 << PE5))) // 2번 스위치 확인
        {
            _delay_ms(20); // 스위치 채터링 방지

            if (!(PINE & (1 << PE5)))
            {
                opIndex++; // 다음 연산자로 변경

                if (opIndex >= 4)
                {
                    opIndex = 0; // 나눗셈 다음에는 덧셈으로 변경
                }

                result = 0; // 결과값 초기화
                lcdPrint(a, b, opList[opIndex], result);

                while (!(PINE & (1 << PE5)))
                {
                }

                _delay_ms(20); // 스위치 채터링 방지
            }
        }

        if (!(PIND & (1 << PD2))) // 3번 스위치 확인
        {
            _delay_ms(20); // 스위치 채터링 방지

            if (!(PIND & (1 << PD2)))
            {
                b++; // B값 1 증가
                result = 0; // 결과값 초기화
                lcdPrint(a, b, opList[opIndex], result);

                while (!(PIND & (1 << PD2)))
                {
                }

                _delay_ms(20); // 스위치 채터링 방지
            }
        }

        if (!(PIND & (1 << PD3))) // 4번 스위치 확인
        {
            _delay_ms(20); // 스위치 채터링 방지

            if (!(PIND & (1 << PD3)))
            {
                if (opList[opIndex] == '+')
                {
                    result = a + b; // 덧셈 계산
                }
                else if (opList[opIndex] == '-')
                {
                    result = a - b; // 뺄셈 계산
                }
                else if (opList[opIndex] == '*')
                {
                    result = a * b; // 곱셈 계산
                }
                else
                {
                    if (b != 0)
                    {
                        result = a / b; // 나눗셈 계산
                    }
                    else
                    {
                        result = 0; // 0으로 나누는 경우
                    }
                }

                lcdPrint(a, b, opList[opIndex], result); // 계산 결과 표시

                while (!(PIND & (1 << PD3)))
                {
                }

                _delay_ms(20); // 스위치 채터링 방지
            }
        }
    }
}

void lcdPrint(int a, int b, char op, int result)
{
    char text[17];

    snprintf(text, sizeof(text), "%03d %c %03d = %04d",
             a, op, b, result); // 16칸 계산식 만들기

    lcdString(0, 0, text); // LCD 첫 번째 줄에 계산식 표시
}