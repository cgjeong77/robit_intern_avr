/*==============================================================================
 *
 *  I2C LCD Driver
 *
 *  LCD       : 16x2 Character LCD
 *  I2C Chip  : PCF8574
 *  MCU       : ATmega128 (16MHz)
 *  Address   : 0x27
 *
 *  연결:
 *  LCD SCL → ATmega128 PD0
 *  LCD SDA → ATmega128 PD1
 *  LCD VCC → 5V
 *  LCD GND → GND
 *
==============================================================================*/

#ifndef LCD_TEXT_H_
#define LCD_TEXT_H_

/* LCD 초기화 */
void lcdInit(void);

/* LCD 화면 지우기 */
void lcdClear(void);

/* LCD에 명령 전송 */
void lcdCommand(unsigned char byte);

/* LCD에 문자 데이터 전송 */
void lcdData(unsigned char byte);

/* LCD 출력 위치 지정 */
void lcdDisplayPosition(unsigned char line, unsigned char col);

/* LCD 문자열 출력 */
void lcdString(unsigned char line, unsigned char col, char* str);

/* LCD 정수 출력 */
void lcdNumber(unsigned char line, unsigned char col, int num);

#endif