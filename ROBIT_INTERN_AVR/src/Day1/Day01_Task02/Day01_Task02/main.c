/*
 * Day01_Task02.c
 *
 * Author : chang
 */

#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile unsigned char count = 0; // 2진 카운터 값

int main(void)
{
	DDRA = 0xFF; // LED 출력
	DDRD = 0x00; // PD2, PD3 입력
	DDRE = 0x00; // PE4, PE5 입력

	PORTA = 0xFF; // LED 모두 끄기
	PORTD = 0x0C; // PD2, PD3 풀업
	PORTE = 0x30; // PE4, PE5 풀업

	EICRA = 0xA0; // INT2, INT3 하강 에지
	EICRB = 0x0A; // INT4, INT5 하강 에지
	EIFR = 0x3C; // 남아 있는 인터럽트 제거
	EIMSK = 0x3C; // INT2~INT5 인터럽트 허용

	sei(); // 전체 인터럽트 허용

	while (1)
	{
		PORTA = ~count; // 현재 숫자를 2진수로 출력
		count++; // 카운터 1 증가
		_delay_ms(100); // 0.1초 대기
	}
}

ISR(INT4_vect) // 과제의 INT0 우측 이동
{
	unsigned char led = 0x07; // 처음 LED 3개 설정
	int i; // 이동 횟수
	int repeat; // 반복 횟수

	for (repeat = 0; repeat < 2; repeat++) // 우측 회전 2바퀴
	{
		for (i = 0; i < 8; i++) // LED 8칸 이동
		{
			PORTA = ~led; // 현재 위치의 LED 3개 켜기
			_delay_ms(300); // 이동 과정 확인

			led = (led << 1) | (led >> 7); // 실제 LED 기준 우측 이동
		}
	}
}

ISR(INT5_vect) // 과제의 INT1 좌측 이동
{
	unsigned char led = 0xE0; // 반대편 LED 3개 설정
	int i; // 이동 횟수
	int repeat; // 반복 횟수

	for (repeat = 0; repeat < 2; repeat++) // 좌측 회전 2바퀴
	{
		for (i = 0; i < 8; i++) // LED 8칸 이동
		{
			PORTA = ~led; // 현재 위치의 LED 3개 켜기
			_delay_ms(300); // 이동 과정 확인

			led = (led >> 1) | (led << 7); // 실제 LED 기준 좌측 이동
		}
	}
}

ISR(INT2_vect) // PD2 인터럽트
{
	int i; // LED 위치

	for (i = 7; i >= 0; i--) // LED 1개 좌측 이동
	{
		PORTA = ~(0x01 << i); // 해당 위치 LED 켜기
		_delay_ms(100); // 0.1초 대기
	}

	for (i = 1; i < 8; i++) // LED 1개 우측 이동
	{
		PORTA = ~(0x01 << i); // 해당 위치 LED 켜기
		_delay_ms(100); // 0.1초 대기
	}
}

ISR(INT3_vect) // PD3 인터럽트
{
	count = 0; // 2진 카운터 초기화
	PORTA = 0xFF; // LED에 0 표시
}