#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "LCD_Text.h"

#define LCD_ADDR 0x27

#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

static void twiInit(void);
static void twiStart(void);
static void twiStop(void);
static void twiWrite(unsigned char data);
static void lcdWrite4Bit(unsigned char data);

/* I2C 통신 초기화 */
static void twiInit(void)
{
    DDRD &= ~((1 << PD0) | (1 << PD1));
    PORTD |= (1 << PD0) | (1 << PD1);

    TWSR = 0x00;
    TWBR = 72;
    TWCR = (1 << TWEN);
}

/* I2C 통신 시작 및 LCD 주소 전송 */
static void twiStart(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
    {
    }

    TWDR = LCD_ADDR << 1;
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
    {
    }
}

/* I2C 통신 종료 */
static void twiStop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

/* I2C 데이터 전송 */
static void twiWrite(unsigned char data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
    {
    }
}

/* LCD에 4비트 전송 */
static void lcdWrite4Bit(unsigned char data)
{
    twiStart();

    twiWrite(data | LCD_BL);
    twiWrite(data | LCD_BL | LCD_EN);

    _delay_us(1);

    twiWrite((data | LCD_BL) & ~LCD_EN);

    twiStop();

    _delay_us(50);
}

/* LCD 명령 전송 */
void lcdCommand(unsigned char byte)
{
    lcdWrite4Bit(byte & 0xF0);
    lcdWrite4Bit((byte << 4) & 0xF0);

    if (byte == 0x01 || byte == 0x02)
    {
        _delay_ms(2);
    }
}

/* LCD 문자 전송 */
void lcdData(unsigned char byte)
{
    lcdWrite4Bit((byte & 0xF0) | LCD_RS);
    lcdWrite4Bit(((byte << 4) & 0xF0) | LCD_RS);
}

/* LCD 초기화 */
void lcdInit(void)
{
    twiInit();

    _delay_ms(50);

    lcdWrite4Bit(0x30);
    _delay_ms(5);

    lcdWrite4Bit(0x30);
    _delay_us(200);

    lcdWrite4Bit(0x30);
    lcdWrite4Bit(0x20);

    lcdCommand(0x28);  // 4비트, 2줄
    lcdCommand(0x0C);  // 화면 켜기
    lcdCommand(0x06);  // 문자 입력 후 오른쪽 이동
    lcdCommand(0x01);  // 화면 지우기
}

/* LCD 화면 지우기 */
void lcdClear(void)
{
    lcdCommand(0x01);
}

/* 출력 위치 지정 */
void lcdDisplayPosition(unsigned char line, unsigned char col)
{
    if (line == 0)
    {
        lcdCommand(0x80 + col);
    }
    else
    {
        lcdCommand(0xC0 + col);
    }
}

/* 문자열 출력 */
void lcdString(unsigned char line, unsigned char col, char *str)
{
    lcdDisplayPosition(line, col);

    while (*str != '\0')
    {
        lcdData(*str);
        str++;
    }
}

/* 정수 출력 */
void lcdNumber(unsigned char line, unsigned char col, int num)
{
    char text[12];
    int index = 0;
    int start;
    int end;
    char temp;

    if (num == 0)
    {
        text[index++] = '0';
    }
    else
    {
        if (num < 0)
        {
            text[index++] = '-';
            num = -num;
        }

        start = index;

        while (num > 0)
        {
            text[index++] = (num % 10) + '0';
            num /= 10;
        }

        end = index - 1;

        while (start < end)
        {
            temp = text[start];
            text[start] = text[end];
            text[end] = temp;

            start++;
            end--;
        }
    }

    text[index] = '\0';

    lcdString(line, col, text);
}