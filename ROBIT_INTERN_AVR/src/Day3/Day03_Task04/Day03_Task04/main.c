/*
 * Day03_Task04.c
 *
 * ATmega128의 UART1을 이용해서
 * 1초마다 "HelloWorld!" 문자열을 전송한다.
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/* 함수 선언 */
static void UART1_Init(void);
static void UART1_Transmit(uint8_t data);
static void UART1_Print(const char *text);

/* UART1 초기화 */
static void UART1_Init(void)
{
    /* UART1 핀 방향 설정 */
    DDRD &= ~(1 << PD2);      // PD2는 수신 입력
    DDRD |=  (1 << PD3);      // PD3는 송신 출력

    /*
     * 16MHz, 9600bps, 일반 속도 모드
     * UBRR1 값은 103을 사용한다.
     */
    UCSR1A = 0x00;

    UBRR1H = 0;
    UBRR1L = 103;

    /* 이번 과제는 송신만 사용한다. */
    UCSR1B = (1 << TXEN1);

    /* 데이터 8비트, 패리티 없음, 정지비트 1개 */
    UCSR1C =
        (1 << UCSZ11) |
        (1 << UCSZ10);
}

/* UART1로 문자 한 개 전송 */
static void UART1_Transmit(uint8_t data)
{
    /* 송신 버퍼가 빌 때까지 기다린다. */
    while (!(UCSR1A & (1 << UDRE1)))
    {
    }

    UDR1 = data;
}

/* UART1로 문자열 전송 */
static void UART1_Print(const char *text)
{
    while (*text != '\0')
    {
        UART1_Transmit((uint8_t)*text);
        text++;
    }
}

int main(void)
{
    UART1_Init();

    while (1)
    {
        UART1_Print("HelloWorld!\r\n");

        _delay_ms(1000);
    }

    return 0;
}