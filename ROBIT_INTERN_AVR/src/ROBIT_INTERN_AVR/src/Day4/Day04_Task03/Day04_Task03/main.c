/*
 * Day04_Task03.c
 *
 * PSD 센서로 거리를 측정하고 UART로 출력
 *
 * PSD 센서 출력 : PF1
 * UART0         : PE0(RX), PE1(TX)
 * 통신 속도     : 57600bps
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/* 센서 및 측정 설정 */
#define PSD_ADC_CHANNEL       1
#define PSD_SAMPLE_COUNT      16
#define MEASURE_DELAY_MS      100

/* ADC와 거리의 정상 범위 */
#define PSD_ADC_MIN_VALID     60
#define PSD_ADC_MAX_VALID     900

#define PSD_MIN_DISTANCE_CM   20
#define PSD_MAX_DISTANCE_CM   80

/* 함수 선언 */
void uart0Init(void);
void uart0Transmit(char data);
void uart0String(const char *text);
void uart0Number(uint16_t number);

void adcInit(void);
uint16_t adcRead(uint8_t channel);
uint16_t adcReadAverage(uint8_t channel, uint8_t sample_count);

int16_t psdAdcToDistance(uint16_t adc_value);

int main(void)
{
    uint16_t adc_value;
    int16_t distance_cm;

    uart0Init();
    adcInit();

    uart0String("\r\n");
    uart0String("PSD Distance Measurement Start\r\n");
    uart0String("ADC Channel : PF1\r\n");
    uart0String("Baud Rate   : 57600\r\n");
    uart0String("\r\n");

    while (1)
    {
        /* ADC값을 여러 번 읽어서 평균 사용 */
        adc_value = adcReadAverage(
            PSD_ADC_CHANNEL,
            PSD_SAMPLE_COUNT
        );

        /* ADC값이 정상 범위인지 확인 */
        if (adc_value < PSD_ADC_MIN_VALID ||
            adc_value > PSD_ADC_MAX_VALID)
        {
            uart0String("ERROR : Invalid ADC value, ADC = ");
            uart0Number(adc_value);
            uart0String("\r\n");
        }
        else
        {
            distance_cm = psdAdcToDistance(adc_value);

            /* 센서의 측정 가능 범위를 벗어난 경우 */
            if (distance_cm < PSD_MIN_DISTANCE_CM ||
                distance_cm > PSD_MAX_DISTANCE_CM)
            {
                uart0String("OUT OF RANGE, ADC = ");
                uart0Number(adc_value);
                uart0String("\r\n");
            }
            else
            {
                uart0String("Distance : ");
                uart0Number((uint16_t)distance_cm);

                uart0String(" cm, ADC : ");
                uart0Number(adc_value);

                uart0String("\r\n");
            }
        }

        _delay_ms(MEASURE_DELAY_MS);
    }
}

/* UART0 초기화 */
void uart0Init(void)
{
    /* PE0은 수신, PE1은 송신 */
    DDRE &= ~(1 << PE0);
    DDRE |= (1 << PE1);

    /* 16MHz에서 57600bps */
    UBRR0H = 0;
    UBRR0L = 16;

    /* UART 송수신 사용 */
    UCSR0B =
        (1 << RXEN0) |
        (1 << TXEN0);

    /* 8비트, 패리티 없음, 정지 비트 1개 */
    UCSR0C =
        (1 << UCSZ01) |
        (1 << UCSZ00);
}

/* 문자 하나 전송 */
void uart0Transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

/* 문자열 전송 */
void uart0String(const char *text)
{
    while (*text != '\0')
    {
        uart0Transmit(*text);
        text++;
    }
}

/* 정수값을 문자열로 바꿔 전송 */
void uart0Number(uint16_t number)
{
    char buffer[6];
    uint8_t index = 0;

    if (number == 0)
    {
        uart0Transmit('0');
        return;
    }

    while (number > 0)
    {
        buffer[index] = (number % 10) + '0';
        number /= 10;
        index++;
    }

    while (index > 0)
    {
        index--;
        uart0Transmit(buffer[index]);
    }
}

/* ADC 초기화 */
void adcInit(void)
{
    /* PF1을 ADC 입력으로 설정 */
    DDRF &= ~(1 << PF1);
    PORTF &= ~(1 << PF1);

    /* 기준 전압 AVCC, ADC1 선택 */
    ADMUX =
        (1 << REFS0) |
        PSD_ADC_CHANNEL;

    /* ADC 사용, 분주비 128 */
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

/* 지정한 ADC 채널의 값 읽기 */
uint16_t adcRead(uint8_t channel)
{
    /* 기준 전압은 유지하고 채널만 변경 */
    ADMUX =
        (ADMUX & 0xE0) |
        (channel & 0x07);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}

/* ADC값을 여러 번 읽어서 평균 계산 */
uint16_t adcReadAverage(
    uint8_t channel,
    uint8_t sample_count
)
{
    uint32_t total = 0;
    uint8_t i;

    if (sample_count == 0)
    {
        return 0;
    }

    for (i = 0; i < sample_count; i++)
    {
        total += adcRead(channel);
        _delay_ms(2);
    }

    return (uint16_t)(total / sample_count);
}

/*
 * ADC값을 거리로 변환
 *
 * 사용한 근사식:
 * 거리(cm) = 4800 / (ADC - 20)
 *
 * 실제 센서에 따라 약간의 오차가 생길 수 있다.
 */
int16_t psdAdcToDistance(uint16_t adc_value)
{
    if (adc_value <= 20)
    {
        return -1;
    }

    return (int16_t)(
        4800UL / (adc_value - 20)
    );
}