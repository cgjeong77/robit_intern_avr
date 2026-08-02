/*
 * Day03_Task03.c
 *
 * ATmega128 + MAX485 + Dynamixel Protocol 2.0
 *
 * [동작]
 * - 가변저항 PF0의 ADC값 0~1023을 목표 위치로 설정
 * - PC에서 0~9 입력 시 목표 속도를 0~300으로 설정
 * - LCD 1행에 목표 속도 표시
 * - LCD 2행에 목표 위치 표시
 *
 * [LCD 출력 예시]
 * Speed: 100
 * Pos  : 512
 *
 * [연결]
 * - UART0 RXD0     : PE0
 * - UART0 TXD0     : PE1
 * - MAX485 RE/DE   : PE2
 * - 가변저항       : PF0
 * - LCD SCL        : PD0
 * - LCD SDA        : PD1
 *
 * [Dynamixel]
 * - ID       : 1
 * - Protocol : 2.0
 * - Baudrate : 57600bps
 *
 * 주의:
 * 제어 테이블 주소는 Protocol 2.0 X-Series 계열 기준이다.
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#include "LCD_Text.h"

/* UART 설정 */

#define UART0_BAUD 57600UL

#define UBRR0_VALUE \
    ((((F_CPU) + (8UL * UART0_BAUD)) / \
    (16UL * UART0_BAUD)) - 1UL)

/* 다이나믹셀 설정 */

#define DXL_ID 1

/* X-Series Protocol 2.0 제어 테이블 주소 */
#define ADDR_TORQUE_ENABLE         64
#define ADDR_STATUS_RETURN_LEVEL   68
#define ADDR_PROFILE_VELOCITY      112
#define ADDR_GOAL_POSITION         116

#define MAX_GOAL_SPEED             300UL
#define INITIAL_GOAL_SPEED         100UL

/* ADC값이 4 이상 변했을 때만 새 위치를 전송한다. */
#define POSITION_CHANGE_THRESHOLD  4

/* MAX485 방향 제어 */

#define MAX485_DIR_PIN PE2

/*
 * PE2 HIGH: 송신 모드
 * PE2 LOW : 수신 모드
 */
#define MAX485_TX_MODE() \
    (PORTE |= (1 << MAX485_DIR_PIN))

#define MAX485_RX_MODE() \
    (PORTE &= ~(1 << MAX485_DIR_PIN))

/*
 * 다이나믹셀 응답은 받지 않고 송신 모드를 유지한다.
 * MAX485의 RO가 꺼져 PC UART 입력과 충돌하지 않는다.
 */
#define KEEP_MAX485_TX_MODE 1

/* UART 수신 변수 */

static volatile uint8_t receivedDigit = 0;
static volatile uint8_t newDigitFlag = 0;

/* 함수 선언 */

static void UART0_Init(void);
static void UART0_Transmit(uint8_t data);

static void ADC_Init(void);
static uint16_t ADC_Read(void);
static uint16_t ADC_ReadAverage(void);

static uint16_t Dynamixel_UpdateCRC(
    uint16_t crcAccum,
    const uint8_t *data,
    uint16_t dataLength
);

static void Dynamixel_Write(
    uint8_t id,
    uint16_t address,
    const uint8_t *data,
    uint8_t dataLength
);

static void Dynamixel_Write1Byte(
    uint8_t id,
    uint16_t address,
    uint8_t value
);

static void Dynamixel_Write4Byte(
    uint8_t id,
    uint16_t address,
    uint32_t value
);

static void LCD_ShowValues(
    uint16_t speed,
    uint16_t position
);

/* UART0 초기화 */

static void UART0_Init(void)
{
    /* UART0 핀 방향 설정 */
    DDRE &= ~(1 << PE0);
    DDRE |= (1 << PE1);

    /* 16MHz, 57600bps, 일반 속도 모드 */
    UCSR0A = 0x00;

    UBRR0H = (uint8_t)(UBRR0_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR0_VALUE;

    /* 송신, 수신, 수신 인터럽트 활성화 */
    UCSR0B =
        (1 << RXEN0) |
        (1 << TXEN0) |
        (1 << RXCIE0);

    /* 8비트, 패리티 없음, 정지비트 1개 */
    UCSR0C =
        (1 << UCSZ01) |
        (1 << UCSZ00);
}

/* UART0 한 바이트 송신 */

static void UART0_Transmit(uint8_t data)
{
    /* 송신 버퍼가 빌 때까지 기다린다. */
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

/* UART0 수신 인터럽트 */

/* PC에서 받은 0~9만 저장한다. */
ISR(USART0_RX_vect)
{
    uint8_t data;

    data = UDR0;

    if ((data >= '0') && (data <= '9'))
    {
        receivedDigit = (uint8_t)(data - '0');
        newDigitFlag = 1;
    }
}

/* ADC 초기화 */

static void ADC_Init(void)
{
    /* PF0을 ADC 입력으로 사용하고 풀업은 끈다. */
    DDRF &= ~(1 << PF0);
    PORTF &= ~(1 << PF0);

    /* 기준전압은 AVCC, 채널은 ADC0(PF0) */
    ADMUX = (1 << REFS0);

    /* ADC 활성화, 128분주(125kHz) */
    ADCSRA =
        (1 << ADEN) |
        (1 << ADPS2) |
        (1 << ADPS1) |
        (1 << ADPS0);

    /* 첫 번째 변환값은 버린다. */
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }
}

/* ADC 1회 측정 */

static uint16_t ADC_Read(void)
{
    /* ADC 변환 시작 */
    ADCSRA |= (1 << ADSC);

    /* 변환이 끝날 때까지 기다린다. */
    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADCW;
}

/* ADC 평균값 계산 */

/* ADC값을 8번 읽어 평균을 구한다. */
static uint16_t ADC_ReadAverage(void)
{
    uint8_t i;
    uint32_t sum = 0;

    for (i = 0; i < 8; i++)
    {
        sum += ADC_Read();
        _delay_us(100);
    }

    return (uint16_t)(sum / 8UL);
}

/* 다이나믹셀 CRC 계산 */

/* Dynamixel Protocol 2.0 CRC-16 계산 */
static uint16_t Dynamixel_UpdateCRC(
    uint16_t crcAccum,
    const uint8_t *data,
    uint16_t dataLength
)
{
    uint16_t byteIndex;
    uint8_t bitIndex;

    for (byteIndex = 0; byteIndex < dataLength; byteIndex++)
    {
        crcAccum ^=
            (uint16_t)data[byteIndex] << 8;

        for (bitIndex = 0; bitIndex < 8; bitIndex++)
        {
            if (crcAccum & 0x8000)
            {
                crcAccum =
                    (uint16_t)(
                        (crcAccum << 1) ^
                        0x8005
                    );
            }
            else
            {
                crcAccum <<= 1;
            }
        }
    }

    return crcAccum;
}

/* 다이나믹셀 WRITE 패킷 전송 */

static void Dynamixel_Write(
    uint8_t id,
    uint16_t address,
    const uint8_t *data,
    uint8_t dataLength
)
{
    uint8_t packet[20];

    uint8_t packetIndex = 0;
    uint8_t i;

    uint16_t length;
    uint16_t crc;

    /* Length = Instruction + Address + Data + CRC */
    length =
        (uint16_t)(
            1 +
            2 +
            dataLength +
            2
        );

    /* Header */
    packet[packetIndex++] = 0xFF;
    packet[packetIndex++] = 0xFF;
    packet[packetIndex++] = 0xFD;
    packet[packetIndex++] = 0x00;

    /* ID */
    packet[packetIndex++] = id;

    /* Length */
    packet[packetIndex++] =
        (uint8_t)(length & 0xFF);

    packet[packetIndex++] =
        (uint8_t)(length >> 8);

    /* WRITE 명령 */
    packet[packetIndex++] = 0x03;

    /* 주소 */
    packet[packetIndex++] =
        (uint8_t)(address & 0xFF);

    packet[packetIndex++] =
        (uint8_t)(address >> 8);

    /* 데이터 */
    for (i = 0; i < dataLength; i++)
    {
        packet[packetIndex++] = data[i];
    }

    /* CRC 계산 */
    crc = Dynamixel_UpdateCRC(
        0,
        packet,
        packetIndex
    );

    /* CRC 추가 */
    packet[packetIndex++] =
        (uint8_t)(crc & 0xFF);

    packet[packetIndex++] =
        (uint8_t)(crc >> 8);

    /* MAX485 송신 모드 */
    MAX485_TX_MODE();

    _delay_us(10);

    /* 기존 송신 완료 플래그 제거 */
    UCSR0A |= (1 << TXC0);

    /* 패킷 전송 */
    for (i = 0; i < packetIndex; i++)
    {
        UART0_Transmit(packet[i]);
    }

    /* 마지막 바이트까지 전송될 때까지 기다린다. */
    while (!(UCSR0A & (1 << TXC0)))
    {
    }

    /* 다음 전송을 위해 플래그 제거 */
    UCSR0A |= (1 << TXC0);

#if !KEEP_MAX485_TX_MODE

    /* 응답을 받을 때만 수신 모드로 바꾼다. */
    MAX485_RX_MODE();

#endif
}

/* 다이나믹셀 1바이트 쓰기 */

static void Dynamixel_Write1Byte(
    uint8_t id,
    uint16_t address,
    uint8_t value
)
{
    Dynamixel_Write(
        id,
        address,
        &value,
        1
    );
}

/* 다이나믹셀 4바이트 쓰기 */

static void Dynamixel_Write4Byte(
    uint8_t id,
    uint16_t address,
    uint32_t value
)
{
    uint8_t data[4];

    /* 4바이트 값을 리틀엔디안 순서로 저장한다. */
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);

    Dynamixel_Write(
        id,
        address,
        data,
        4
    );
}

/* LCD 표시 */

static void LCD_ShowValues(
    uint16_t speed,
    uint16_t position
)
{
    char firstLine[17];
    char secondLine[17];

    /*
     * LCD에 속도와 위치를 표시한다.
     * 남는 자리는 공백으로 채워 이전 값이 남지 않게 한다.
     */
    snprintf(
        firstLine,
        sizeof(firstLine),
        "Speed:%4u     ",
        speed
    );

    snprintf(
        secondLine,
        sizeof(secondLine),
        "Pos  :%4u     ",
        position
    );

    lcdString(0, 0, firstLine);
    lcdString(1, 0, secondLine);
}

/* main */

int main(void)
{
    uint16_t adcValue;

    uint16_t goalPosition = 0;
    uint16_t goalSpeed = INITIAL_GOAL_SPEED;

    uint16_t lastPosition = 0xFFFF;

    uint16_t displayedPosition = 0xFFFF;
    uint16_t displayedSpeed = 0xFFFF;

    /* MAX485 방향 제어 핀 설정 */
    DDRE |= (1 << MAX485_DIR_PIN);

    /* 처음에는 수신 모드 */
    MAX485_RX_MODE();

    UART0_Init();
    ADC_Init();

    lcdInit();
    lcdClear();

    /* 전체 인터럽트 허용 */
    sei();

    /* 초기값을 LCD에 표시 */
    LCD_ShowValues(
        goalSpeed,
        goalPosition
    );

    /* 다이나믹셀 전원 안정화 대기 */
    _delay_ms(2000);

#if KEEP_MAX485_TX_MODE

    /*
     * WRITE 명령에 응답하지 않도록 설정한다.
     * 과제에서는 응답 패킷을 사용하지 않는다.
     */
    Dynamixel_Write1Byte(
        DXL_ID,
        ADDR_STATUS_RETURN_LEVEL,
        1
    );

    _delay_ms(20);

    /*
     * 이후 송신 모드를 유지해
     * PC UART 입력과의 충돌을 막는다.
     */
    MAX485_TX_MODE();

#endif

    /* 다이나믹셀 토크 켜기 */
    Dynamixel_Write1Byte(
        DXL_ID,
        ADDR_TORQUE_ENABLE,
        1
    );

    _delay_ms(50);

    /* 초기 속도값 전송 */
    Dynamixel_Write4Byte(
        DXL_ID,
        ADDR_PROFILE_VELOCITY,
        goalSpeed
    );

    _delay_ms(50);

    while (1)
    {
        /* PC에서 받은 숫자로 목표 속도를 바꾼다. */

        if (newDigitFlag)
        {
            uint8_t digit;

            /* 인터럽트와 겹치지 않게 값을 복사한다. */
            cli();

            digit = receivedDigit;
            newDigitFlag = 0;

            sei();

            /*
             * PC 입력 0~9를 속도 0~300으로 변환한다.
             * 예: 3은 100, 6은 200, 9는 300
             */
            goalSpeed =
                (uint16_t)(
                    (uint32_t)digit *
                    MAX_GOAL_SPEED /
                    9UL
                );

            /* 목표 속도 전송 */
            Dynamixel_Write4Byte(
                DXL_ID,
                ADDR_PROFILE_VELOCITY,
                goalSpeed
            );
        }
        /* 가변저항 값으로 목표 위치를 정한다. */

        adcValue = ADC_ReadAverage();

        /* ADC값 0~1023을 목표 위치로 그대로 사용한다. */
        goalPosition = adcValue;

        /* 위치가 4 이상 변했을 때만 새 값을 전송한다. */
        if (
            (lastPosition == 0xFFFF) ||

            (
                (goalPosition > lastPosition) &&
                (
                    goalPosition - lastPosition >=
                    POSITION_CHANGE_THRESHOLD
                )
            ) ||

            (
                (lastPosition > goalPosition) &&
                (
                    lastPosition - goalPosition >=
                    POSITION_CHANGE_THRESHOLD
                )
            )
        )
        {
            /* 목표 위치 전송 */
            Dynamixel_Write4Byte(
                DXL_ID,
                ADDR_GOAL_POSITION,
                goalPosition
            );

            lastPosition = goalPosition;
        }
        /* 값이 바뀌었을 때만 LCD를 갱신한다. */

        if (
            (goalSpeed != displayedSpeed) ||
            (lastPosition != displayedPosition)
        )
        {
            LCD_ShowValues(
                goalSpeed,
                lastPosition
            );

            displayedSpeed = goalSpeed;
            displayedPosition = lastPosition;
        }

        _delay_ms(50);
    }

    return 0;
}