/*
 * Day03_Task02.c
 *
 * ATmega128의 UART0를 이용해 PC에서 명령을 받고 LED를 제어한다.
 *
 * UART0
 * - PE0 : RXD0
 * - PE1 : TXD0
 * - 통신 속도 : 57600bps
 *
 * MAX485
 * - RO      : PE0
 * - DI      : PE1
 * - RE, DE  : PE2
 *
 * 스위치
 * - SW1 : PE4
 *
 * LED
 * - PA0 ~ PA7
 * - LOW일 때 켜지는 Active LOW 방식
 *
 * 동작
 * - 0~7 입력 : 해당 번호의 LED 켜기
 * - 8 입력   : 0번부터 왼쪽 방향으로 한 바퀴 이동
 * - 9 입력   : 0번부터 오른쪽 방향으로 한 바퀴 이동
 * - SW1      : LED 상태 초기화
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/* 사용할 핀 설정 */
#define SW1_PIN             PE4
#define MAX485_CTRL_PIN     PE2

/* LED는 Active LOW이므로 0xFF를 출력하면 모두 꺼진다. */
#define LED_OFF_VALUE       0xFF

/* 함수 선언 */
static void UART0_Init(void);
static uint8_t UART0_Receive(void);
static void UART0_Transmit(uint8_t data);
static void UART0_Print(const char *text);

static void LED_On(uint8_t number);
static void LED_Reset(void);
static void LED_SweepLeft(void);
static void LED_SweepRight(void);

static void Check_ResetSwitch(void);

/* 현재 켜져 있는 LED 번호
 * -1은 켜진 LED가 없는 상태를 의미한다.
 */
static int8_t currentLED = -1;

/* UART0 초기화 */
static void UART0_Init(void)
{
	/*
	 * 16MHz에서 57600bps를 사용한다.
	 * 일반 속도 모드이므로 UBRR0 값은 16이다.
	 */
	UBRR0H = 0;
	UBRR0L = 16;

	UCSR0A = 0x00;                                  // 일반 속도 모드 사용
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);           // 송신과 수신 사용
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);         // 데이터 8비트, 정지비트 1개
}

/* PC에서 문자 1개를 받을 때까지 기다린다. */
static uint8_t UART0_Receive(void)
{
	while (!(UCSR0A & (1 << RXC0)))
	{
	}

	return UDR0;
}

/* UART0로 문자 1개를 전송한다. */
static void UART0_Transmit(uint8_t data)
{
	while (!(UCSR0A & (1 << UDRE0)))
	{
	}

	UDR0 = data;
}

/* 문자열을 한 글자씩 전송한다. */
static void UART0_Print(const char *text)
{
	while (*text != '\0')
	{
		UART0_Transmit((uint8_t)*text);
		text++;
	}
}

/* 원하는 번호의 LED만 켠다. */
static void LED_On(uint8_t number)
{
	PORTA = (uint8_t)~(1 << number);
	currentLED = (int8_t)number;
}

/* LED를 모두 끄고 현재 상태를 초기화한다. */
static void LED_Reset(void)
{
	PORTA = LED_OFF_VALUE;
	currentLED = -1;
}

/*
 * 왼쪽 방향으로 LED를 한 바퀴 이동시킨다.
 *
 * 이동 순서
 * 0 → 7 → 6 → 5 → 4 → 3 → 2 → 1 → 0
 *
 * LED가 한 칸 이동할 때마다 PC에 LEFT를 전송한다.
 */
static void LED_SweepLeft(void)
{
	int8_t number;

	/* 이전 LED 위치와 관계없이 항상 0번부터 시작한다. */
	LED_On(0);
	_delay_ms(150);

	for (number = 7; number >= 0; number--)
	{
		LED_On((uint8_t)number);
		UART0_Print("LEFT\r\n");
		_delay_ms(150);
	}

	/* 한 바퀴 이동한 뒤 최종 위치는 0번이다. */
	currentLED = 0;
}

/*
 * 오른쪽 방향으로 LED를 한 바퀴 이동시킨다.
 *
 * 이동 순서
 * 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 0
 *
 * LED가 한 칸 이동할 때마다 PC에 RIGHT를 전송한다.
 */
static void LED_SweepRight(void)
{
	uint8_t number;

	/* 이전 LED 위치와 관계없이 항상 0번부터 시작한다. */
	LED_On(0);
	_delay_ms(150);

	for (number = 1; number <= 7; number++)
	{
		LED_On(number);
		UART0_Print("RIGHT\r\n");
		_delay_ms(150);
	}

	/* 7번에서 마지막으로 0번으로 돌아온다. */
	LED_On(0);
	UART0_Print("RIGHT\r\n");
	_delay_ms(150);

	currentLED = 0;
}

/* SW1이 눌렸는지 확인한다. */
static void Check_ResetSwitch(void)
{
	static uint8_t previousState = 1;
	uint8_t currentState;

	/* 스위치를 누르면 LOW가 입력된다. */
	currentState = (PINE & (1 << SW1_PIN)) ? 1 : 0;

	/* 스위치가 HIGH에서 LOW로 바뀐 순간만 처리한다. */
	if ((previousState == 1) && (currentState == 0))
	{
		_delay_ms(20);                              // 스위치 채터링 방지

		if (!(PINE & (1 << SW1_PIN)))
		{
			LED_Reset();
			UART0_Print("RESET\r\n");
		}
	}

	previousState = currentState;
}

int main(void)
{
	uint8_t receivedData;
	char ledMessage[] = "0 LED on\r\n";

	/* PORTA 전체를 LED 출력으로 설정한다. */
	DDRA = 0xFF;
	PORTA = LED_OFF_VALUE;

	/* SW1을 입력으로 설정하고 내부 풀업을 사용한다. */
	DDRE &= ~(1 << SW1_PIN);
	PORTE |= (1 << SW1_PIN);

	/* UART0 핀 방향을 설정한다. */
	DDRE &= ~(1 << PE0);                            // PE0는 수신 입력
	DDRE |= (1 << PE1);                             // PE1은 송신 출력

	/*
	 * MAX485의 RE와 DE가 PE2에 연결되어 있다.
	 *
	 * PE2를 HIGH로 두면 /RE도 HIGH가 되어
	 * MAX485의 RO 출력이 비활성화된다.
	 * 따라서 PC에서 들어오는 UART 신호와 충돌하지 않는다.
	 */
	DDRE |= (1 << MAX485_CTRL_PIN);
	PORTE |= (1 << MAX485_CTRL_PIN);

	UART0_Init();

	/* 프로그램이 시작되었음을 PC에 알린다. */
	UART0_Print("READY\r\n");

	while (1)
	{
		/* SW1이 눌렸는지 계속 확인한다. */
		Check_ResetSwitch();

		/* UART0에 데이터가 들어온 경우에만 처리한다. */
		if (UCSR0A & (1 << RXC0))
		{
			receivedData = UART0_Receive();

			/* 터미널에서 함께 전송되는 Enter 문자는 무시한다. */
			if ((receivedData == '\r') ||
			    (receivedData == '\n'))
			{
				continue;
			}

			/* 0~7을 입력하면 해당 번호의 LED를 켠다. */
			if ((receivedData >= '0') &&
			    (receivedData <= '7'))
			{
				LED_On((uint8_t)(receivedData - '0'));

				ledMessage[0] = (char)receivedData;
				UART0_Print(ledMessage);
			}

			/* 8을 입력하면 왼쪽 방향으로 한 바퀴 이동한다. */
			else if (receivedData == '8')
			{
				LED_SweepLeft();
			}

			/* 9를 입력하면 오른쪽 방향으로 한 바퀴 이동한다. */
			else if (receivedData == '9')
			{
				LED_SweepRight();
			}

			/* 지정되지 않은 문자가 들어오면 오류를 출력한다. */
			else
			{
				UART0_Print("ERROR\r\n");
			}
		}
	}

	return 0;
}