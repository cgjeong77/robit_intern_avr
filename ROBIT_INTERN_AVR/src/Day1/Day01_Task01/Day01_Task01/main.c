/*
 * Day01_Task01.c
 *
 * Created: 2026-07-29 오후 5:20:00
 * Author : chang
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile int check = 0; // LED 이동 동작 확인

int main(void)
{
	DDRA = 0xFF; // LED 출력
	DDRE &= ~0x30; // PE4, PE5 입력
	DDRD &= ~0x0C; // PD2, PD3 입력

	PORTA = 0xFF; // LED 모두 끄기
	PORTE |= 0x30; // PE4, PE5 풀업
	PORTD |= 0x0C; // PD2, PD3 풀업

	EICRA = 0xA0; // INT2, INT3 하강 에지
	EIFR = 0x0C; // INT2, INT3 인터럽트 플래그 제거
	EIMSK = 0x0C; // INT2, INT3 인터럽트 허용

	sei(); // 전체 인터럽트 허용

	while (1)
	{
		if ((PINE & 0x30) == 0x00) // PE4와 PE5를 모두 누른 경우
		{
			PORTA = 0x00; // LED 모두 켜기
		}
		else if ((PINE & 0x10) == 0x00) // PE4의 SW1을 누른 경우
		{
			PORTA = 0x0F; // LED 4~7 켜기
		}
		else if ((PINE & 0x20) == 0x00) // PE5의 SW2를 누른 경우
		{
			PORTA = 0xF0; // LED 0~3 켜기
		}
		else if (check == 0) // 인터럽트 동작 중이 아닌 경우
		{
			PORTA = 0x00; // LED 모두 켜기
			_delay_ms(500); // 0.5초 대기

			if (check == 0) // 인터럽트가 발생하지 않은 경우
			{
				PORTA = 0xFF; // LED 모두 끄기
				_delay_ms(500); // 0.5초 대기
			}
		}
	}
}

ISR(INT2_vect) // PD2(INT2), 3번째 스위치
{
	int i; // LED 위치

	check = 1; // 기본 깜빡임 정지

	for (i = 7; i >= 0; i--) // LED를 오른쪽으로 이동
	{
		PORTA = ~(0x01 << i); // 해당 위치의 LED 켜기
		_delay_ms(100); // 0.1초 대기
	}

	check = 0; // 다시 기본 깜빡임 시작
}

ISR(INT3_vect) // PD3(INT3), 4번째 스위치
{
	int i; // LED 위치

	check = 1; // 기본 깜빡임 정지

	for (i = 0; i < 8; i++) // LED를 왼쪽으로 이동
	{
		PORTA = ~(0x01 << i); // 해당 위치의 LED 켜기
		_delay_ms(100); // 0.1초 대기
	}

	check = 0; // 다시 기본 깜빡임 시작
}