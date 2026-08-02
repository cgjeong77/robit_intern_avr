/*
 * Day04_Task05.c
 *
 * PC에서 각도를 입력하면 SG90 서보모터가 움직이는 코드
 *
 * SG90 신호선 : PB7
 * UART0       : PE0(RX), PE1(TX)
 * MAX485 제어 : PE2
 * 통신속도    : 57600bps
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/* 핀 설정 */
#define SERVO_PIN           PB7
#define MAX485_CTRL_PIN     PE2

/* 서보 각도 범위 */
#define SERVO_MIN_ANGLE     0
#define SERVO_MAX_ANGLE     180
#define SERVO_START_ANGLE   90

/*
 * 사용 중인 서보가 1000~2000us에서는
 * 회전 범위가 작아서 600~2400us로 설정했다.
 */
#define SERVO_MIN_PULSE_US  600UL
#define SERVO_MAX_PULSE_US  2400UL

/* 서보 주기 20ms */
#define SERVO_PERIOD_TICKS  40000UL

/*
 * Timer1 분주비가 8이므로
 * 1us는 타이머 2카운트이다.
 */
#define TIMER_TICKS_PER_US  2UL

/* 현재 서보 HIGH 펄스 길이 */
volatile uint16_t servoHighTicks = 3000;

/* 현재 출력이 HIGH 구간인지 확인 */
volatile uint8_t servoHighState = 1;

/* 함수 선언 */
static void max485DisableReceive(void);

static void uart0Init(void);
static void uart0Transmit(uint8_t data);
static void uart0Print(const char *text);
static void uart0PrintNumber(uint16_t number);
static uint8_t uart0DataAvailable(void);
static uint8_t uart0Receive(void);

static void servoInit(void);
static void servoSetAngle(uint16_t angle);

static uint8_t parseAngle(
    const char *text,
    uint16_t *angle
);

int main(void)
{
    char input[8];

    uint8_t inputIndex = 0;
    uint8_t receivedData;

    uint16_t angle;

    /*
     * MAX485가 PE0 수신을 방해하지 않도록
     * RO 출력을 비활성화한다.
     */
    max485DisableReceive();

    uart0Init();
    servoInit();

    sei();

    /* 시작 위치는 90도 */
    servoSetAngle(SERVO_START_ANGLE);

    uart0Print("\r\n");
    uart0Print("SG90 Servo Control Start\r\n");
    uart0Print("Enter angle 0~180\r\n");
    uart0Print("\r\nANGLE > ");

    while (1)
    {
        /* UART로 문자가 들어온 경우만 처리 */
        if (uart0DataAvailable())
        {
            receivedData = uart0Receive();

            /* 입력한 문자를 터미널에 다시 표시 */
            uart0Transmit(receivedData);

            /* Enter를 누르면 입력값을 확인한다. */
            if ((receivedData == '\r') ||
                (receivedData == '\n'))
            {
                /*
                 * CR과 LF가 연속으로 들어오는 경우
                 * 두 번째 Enter 문자는 무시한다.
                 */
                if (inputIndex == 0)
                {
                    continue;
                }

                input[inputIndex] = '\0';

                uart0Print("\r\n");

                if (parseAngle(input, &angle))
                {
                    servoSetAngle(angle);

                    uart0Print("SERVO MOVED TO ");
                    uart0PrintNumber(angle);
                    uart0Print(" DEGREE\r\n");
                }
                else
                {
                    uart0Print("ERROR : ENTER 0~180\r\n");
                    uart0Print("SERVO POSITION NOT CHANGED\r\n");
                }

                inputIndex = 0;

                uart0Print("\r\nANGLE > ");
            }

            /* Backspace 처리 */
            else if ((receivedData == 8) ||
                     (receivedData == 127))
            {
                if (inputIndex > 0)
                {
                    inputIndex--;

                    uart0Transmit('\b');
                    uart0Transmit(' ');
                    uart0Transmit('\b');
                }
            }

            /* 일반 문자는 입력 배열에 저장 */
            else
            {
                if (inputIndex < sizeof(input) - 1)
                {
                    input[inputIndex] =
                        (char)receivedData;

                    inputIndex++;
                }
                else
                {
                    inputIndex = 0;

                    uart0Print("\r\n");
                    uart0Print("ERROR : INPUT TOO LONG\r\n");
                    uart0Print("\r\nANGLE > ");
                }
            }
        }
    }

    return 0;
}

/* MAX485의 RO 출력 끄기 */
static void max485DisableReceive(void)
{
    /*
     * RE와 DE가 PE2에 같이 연결된 회로 기준
     *
     * PE2가 HIGH이면 /RE가 비활성화되어
     * MAX485의 RO가 PE0을 방해하지 않는다.
     */
    DDRE |= (1 << MAX485_CTRL_PIN);
    PORTE |= (1 << MAX485_CTRL_PIN);
}

/* UART0 초기화 */
static void uart0Init(void)
{
    /* PE0은 수신, PE1은 송신 */
    DDRE &= ~(1 << PE0);
    DDRE |= (1 << PE1);

    /* 16MHz에서 57600bps */
    UCSR0A = 0x00;

    UBRR0H = 0;
    UBRR0L = 16;

    /* UART 송수신 활성화 */
    UCSR0B =
        (1 << RXEN0) |
        (1 << TXEN0);

    /* 8비트, 패리티 없음, 정지 비트 1개 */
    UCSR0C =
        (1 << UCSZ01) |
        (1 << UCSZ00);
}

/* 문자 하나 전송 */
static void uart0Transmit(uint8_t data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

/* 문자열 전송 */
static void uart0Print(const char *text)
{
    while (*text != '\0')
    {
        uart0Transmit((uint8_t)*text);
        text++;
    }
}

/* 숫자를 문자로 바꿔 전송 */
static void uart0PrintNumber(uint16_t number)
{
    char buffer[6];
    uint8_t index = 0;

    if (number == 0)
    {
        uart0Transmit('0');
        return;
    }

    /* 숫자를 뒤에서부터 저장 */
    while (number > 0)
    {
        buffer[index] =
            (char)((number % 10) + '0');

        number /= 10;
        index++;
    }

    /* 저장한 문자를 반대 순서로 출력 */
    while (index > 0)
    {
        index--;
        uart0Transmit((uint8_t)buffer[index]);
    }
}

/* UART 수신 데이터가 있는지 확인 */
static uint8_t uart0DataAvailable(void)
{
    if (UCSR0A & (1 << RXC0))
    {
        return 1;
    }

    return 0;
}

/* 수신된 문자 읽기 */
static uint8_t uart0Receive(void)
{
    return UDR0;
}

/* Timer1로 서보 PWM 설정 */
static void servoInit(void)
{
    /* PB7을 서보 신호 출력으로 설정 */
    DDRB |= (1 << SERVO_PIN);

    /* 첫 펄스는 HIGH로 시작 */
    PORTB |= (1 << SERVO_PIN);
    servoHighState = 1;

    TCCR1A = 0x00;
    TCCR1B = 0x00;

    TCNT1 = 0;

    /* 초기값 90도 = 1500us */
    servoHighTicks = 3000;

    OCR1A = servoHighTicks - 1;

    /* 남아 있는 비교일치 플래그 제거 */
    TIFR = (1 << OCF1A);

    /* Timer1 비교일치 인터럽트 사용 */
    TIMSK |= (1 << OCIE1A);

    /* CTC 모드, 분주비 8 */
    TCCR1B =
        (1 << WGM12) |
        (1 << CS11);
}

/* 입력받은 각도를 서보 펄스값으로 변환 */
static void servoSetAngle(uint16_t angle)
{
    uint32_t pulseUs;
    uint16_t newTicks;
    uint8_t oldSreg;

    if (angle > SERVO_MAX_ANGLE)
    {
        return;
    }

    /*
     * 0도는 600us,
     * 90도는 1500us,
     * 180도는 2400us가 되도록 계산한다.
     */
    pulseUs =
        SERVO_MIN_PULSE_US +
        (
            (uint32_t)angle *
            (
                SERVO_MAX_PULSE_US -
                SERVO_MIN_PULSE_US
            )
            / SERVO_MAX_ANGLE
        );

    newTicks =
        (uint16_t)(
            pulseUs * TIMER_TICKS_PER_US
        );

    /*
     * 인터럽트에서 사용하는 16비트 값을
     * 안전하게 변경하기 위해 잠시 인터럽트를 막는다.
     */
    oldSreg = SREG;

    cli();
    servoHighTicks = newTicks;
    SREG = oldSreg;
}

/* 입력 문자열이 0~180인지 확인 */
static uint8_t parseAngle(
    const char *text,
    uint16_t *angle
)
{
    uint16_t value = 0;
    uint8_t index = 0;

    if (text[0] == '\0')
    {
        return 0;
    }

    while (text[index] != '\0')
    {
        /* 숫자가 아닌 문자가 있으면 오류 */
        if ((text[index] < '0') ||
            (text[index] > '9'))
        {
            return 0;
        }

        value =
            (value * 10) +
            (uint16_t)(text[index] - '0');

        /* 180도를 넘으면 오류 */
        if (value > SERVO_MAX_ANGLE)
        {
            return 0;
        }

        index++;
    }

    *angle = value;

    return 1;
}

/* Timer1 비교일치 인터럽트 */
ISR(TIMER1_COMPA_vect)
{
    uint16_t highTicks;

    highTicks = servoHighTicks;

    if (servoHighState)
    {
        /* HIGH 펄스가 끝나면 LOW로 변경 */
        PORTB &= ~(1 << SERVO_PIN);
        servoHighState = 0;

        /*
         * 전체 20ms에서 HIGH 시간을 뺀 만큼
         * LOW 상태를 유지한다.
         */
        OCR1A =
            (uint16_t)(
                SERVO_PERIOD_TICKS -
                highTicks -
                1
            );
    }
    else
    {
        /* 다음 서보 펄스 시작 */
        PORTB |= (1 << SERVO_PIN);
        servoHighState = 1;

        OCR1A = highTicks - 1;
    }
}