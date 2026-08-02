/*
 * Day04_Task04.c
 *
 * PSD 센서값에 이동평균 필터를 적용한 뒤
 * 원본 ADC값, 필터값, 거리를 UART로 출력
 *
 * PSD 센서 : PF1
 * UART0    : PE0(RX), PE1(TX)
 * 통신속도 : 57600bps
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/* 측정 설정 */
#define PSD_ADC_CHANNEL       1
#define FILTER_SIZE           5
#define MEASURE_PERIOD_MS     100

/* PSD 센서 측정 가능 거리 */
#define PSD_MIN_DISTANCE_CM   20
#define PSD_MAX_DISTANCE_CM   80

/* ADC값 정상 범위 */
#define PSD_ADC_MIN_VALID     20
#define PSD_ADC_MAX_VALID     1000

/* 최근 ADC값을 저장할 이동평균 필터 */
typedef struct
{
    uint16_t buffer[FILTER_SIZE];
    uint32_t sum;
    uint8_t index;
    uint8_t initialized;
} MovingAverageFilter;

/* 함수 선언 */
static void uart0Init(void);
static void uart0Transmit(char data);
static void uart0String(const char *text);
static void uart0Number(uint16_t number);

static void adcInit(void);
static uint16_t adcRead(uint8_t channel);

static void movingAverageInit(
    MovingAverageFilter *filter,
    uint16_t first_value
);

static uint16_t movingAverageUpdate(
    MovingAverageFilter *filter,
    uint16_t new_value
);

static int32_t psdAdcToDistance10(uint16_t adc_value);

static void printMeasurement(
    uint16_t raw_adc,
    uint16_t filtered_adc
);

int main(void)
{
    uint16_t raw_adc;
    uint16_t filtered_adc;

    MovingAverageFilter filter =
    {
        {0, 0, 0, 0, 0},
        0,
        0,
        0
    };

    uart0Init();
    adcInit();

    uart0String("\r\n");
    uart0String("PSD Moving Average Filter Start\r\n");
    uart0String("ADC Channel : PF1\r\n");
    uart0String("Filter Size : 5\r\n");
    uart0String("Baud Rate   : 57600\r\n");
    uart0String("\r\n");

    while (1)
    {
        /* PSD 센서의 현재 ADC값 읽기 */
        raw_adc = adcRead(PSD_ADC_CHANNEL);

        /*
         * 처음 실행할 때 버퍼를 첫 ADC값으로 채운다.
         * 초기값 0 때문에 평균이 작아지는 것을 막기 위한 처리다.
         */
        if (!filter.initialized)
        {
            movingAverageInit(&filter, raw_adc);
        }

        /* 최근 5개 ADC값의 평균 계산 */
        filtered_adc = movingAverageUpdate(
            &filter,
            raw_adc
        );

        /* 측정 결과를 터미널로 출력 */
        printMeasurement(
            raw_adc,
            filtered_adc
        );

        _delay_ms(MEASURE_PERIOD_MS);
    }
}

/* UART0 초기화 */
static void uart0Init(void)
{
    /* PE0은 수신, PE1은 송신 */
    DDRE &= ~(1 << PE0);
    DDRE |= (1 << PE1);

    /* 16MHz에서 57600bps */
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
static void uart0Transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

/* 문자열 전송 */
static void uart0String(const char *text)
{
    while (*text != '\0')
    {
        uart0Transmit(*text);
        text++;
    }
}

/* 정수값 전송 */
static void uart0Number(uint16_t number)
{
    char buffer[6];
    uint8_t index = 0;

    if (number == 0)
    {
        uart0Transmit('0');
        return;
    }

    /* 숫자를 뒤에서부터 문자로 저장 */
    while (number > 0)
    {
        buffer[index] = (number % 10) + '0';
        number /= 10;
        index++;
    }

    /* 저장한 문자를 반대 순서로 출력 */
    while (index > 0)
    {
        index--;
        uart0Transmit(buffer[index]);
    }
}

/* ADC 초기화 */
static void adcInit(void)
{
    /* PF1을 ADC 입력으로 설정 */
    DDRF &= ~(1 << PF1);
    PORTF &= ~(1 << PF1);

    /* 기준전압 AVCC, ADC1 선택 */
    ADMUX =
        (1 << REFS0) |
        PSD_ADC_CHANNEL;

    /* ADC 활성화, 분주비 128 */
    ADCSRA =
        (1 << ADEN) |
        (1 << ADPS2) |
        (1 << ADPS1) |
        (1 << ADPS0);

    /* 첫 번째 변환값은 사용하지 않음 */
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }
}

/* 지정한 ADC 채널 읽기 */
static uint16_t adcRead(uint8_t channel)
{
    /* 기준전압은 유지하고 채널만 변경 */
    ADMUX =
        (ADMUX & 0xE0) |
        (channel & 0x07);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}

/* 이동평균 필터 초기값 설정 */
static void movingAverageInit(
    MovingAverageFilter *filter,
    uint16_t first_value
)
{
    uint8_t i;

    filter->sum = 0;
    filter->index = 0;

    /* 버퍼 전체를 처음 측정한 값으로 채운다. */
    for (i = 0; i < FILTER_SIZE; i++)
    {
        filter->buffer[i] = first_value;
        filter->sum += first_value;
    }

    filter->initialized = 1;
}

/* 새로운 ADC값을 넣고 이동평균 계산 */
static uint16_t movingAverageUpdate(
    MovingAverageFilter *filter,
    uint16_t new_value
)
{
    /* 가장 오래된 값을 합계에서 뺀다. */
    filter->sum -= filter->buffer[filter->index];

    /* 해당 자리에 새로운 값을 저장한다. */
    filter->buffer[filter->index] = new_value;
    filter->sum += new_value;

    /* 다음에 바꿀 버퍼 위치로 이동 */
    filter->index++;

    if (filter->index >= FILTER_SIZE)
    {
        filter->index = 0;
    }

    return (uint16_t)(
        filter->sum / FILTER_SIZE
    );
}

/*
 * ADC값을 거리의 10배 값으로 변환한다.
 *
 * 예시:
 * 반환값 152는 실제 거리 15.2cm를 뜻한다.
 *
 * 거리(cm) = 4800 / (ADC - 20)
 */
static int32_t psdAdcToDistance10(uint16_t adc_value)
{
    if (adc_value <= 20)
    {
        return -1;
    }

    return 48000L /
        ((int32_t)adc_value - 20L);
}

/* RAW값, 필터값, 거리 출력 */
static void printMeasurement(
    uint16_t raw_adc,
    uint16_t filtered_adc
)
{
    int32_t distance10;
    uint16_t distance_integer;
    uint8_t distance_decimal;

    uart0String("RAW: ");
    uart0Number(raw_adc);

    uart0String(" | FILTERED: ");
    uart0Number(filtered_adc);

    /* ADC값이 비정상적인 경우 */
    if (filtered_adc < PSD_ADC_MIN_VALID ||
        filtered_adc > PSD_ADC_MAX_VALID)
    {
        uart0String(" | ERROR: INVALID ADC\r\n");
        return;
    }

    distance10 = psdAdcToDistance10(filtered_adc);

    /* 거리 계산이 불가능한 경우 */
    if (distance10 < 0)
    {
        uart0String(" | ERROR: CALCULATION\r\n");
        return;
    }

    distance_integer =
        (uint16_t)(distance10 / 10);

    distance_decimal =
        (uint8_t)(distance10 % 10);

    /* 센서와 너무 가까운 경우 */
    if (distance_integer < PSD_MIN_DISTANCE_CM)
    {
        uart0String(" | TOO CLOSE\r\n");
        return;
    }

    /* 센서에서 너무 먼 경우 */
    if (distance_integer > PSD_MAX_DISTANCE_CM)
    {
        uart0String(" | TOO FAR\r\n");
        return;
    }

    uart0String(" | DISTANCE: ");
    uart0Number(distance_integer);
    uart0Transmit('.');
    uart0Transmit(distance_decimal + '0');
    uart0String("cm\r\n");
}