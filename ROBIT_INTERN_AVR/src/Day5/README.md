# ATmega128 TIMER1 / TIMER3 레지스터 정리

> **광운대학교 AI로봇학과**  
> **작성자:** 정창규  
> **제출일:** 2026-08-03

---

## 1. Timer1과 Timer3

이번 수업에서는 ATmega128의 Timer1과 Timer3, 그리고 PWM 동작에 대해 학습하였다.
Timer1과 Timer3은 모두 16비트 Timer/Counter이며, 0x0000부터 0xFFFF까지 카운트할 수 있다. Timer0과 Timer2는 8비트이지만 Timer1과 Timer3은 16비트이기 때문에 더 넓은 범위의 값을 사용할 수 있다.
Timer1과 Timer3은 내부 또는 외부 Clock을 선택하여 동작한다. TCNT 값이 최대값을 넘어가면 Overflow Interrupt가 발생하고, TCNT 값과 OCR 값이 같아지면 Compare Match Interrupt가 발생한다.

---

## 2. Timer1 관련 레지스터

수업에서 다룬 Timer1 관련 레지스터는 다음과 같다.

| 레지스터 | 역할 |
| :--- | :--- |
| **TCCR1A, TCCR1B, TCCR1C** | Timer1 동작 모드와 출력 방식 설정 |
| **TCNT1H, TCNT1L** | 현재 Timer1 카운트 값 저장 |
| **OCR1AH/L, OCR1BH/L, OCR1CH/L** | Output Compare 기준값 저장 |
| **ICR1H, ICR1L** | Input Capture 값 또는 PWM TOP 값 저장 |
| **TIMSK, ETIMSK** | Timer Interrupt 설정 |
| **TIFR, ETIFR** | Timer Interrupt Flag 확인 |

---

## 3. TIMSK

TIMSK는 Timer1에서 사용할 Interrupt를 활성화하는 레지스터이다.

| 비트 | 역할 |
| :--- | :--- |
| **TICIE1** | Input Capture Interrupt 활성화 |
| **OCIE1A** | Output Compare Match A Interrupt 활성화 |
| **OCIE1B** | Output Compare Match B Interrupt 활성화 |
| **TOIE1** | Overflow Interrupt 활성화 |

각 Interrupt 비트를 1로 설정하면 해당 Interrupt를 사용할 수 있다. 실제로 Interrupt가 발생하려면 SREG의 전체 Interrupt 허용 비트도 활성화되어 있어야 한다.

---

## 4. Timer1 제어 레지스터

### TCCR1A

TCCR1A는 Timer1의 비교 출력 방식과 파형 생성 모드를 설정할 때 사용한다.

| 비트 | 역할 |
| :--- | :--- |
| **COM1A1, COM1A0** | 채널 A 출력 방식 설정 |
| **COM1B1, COM1B0** | 채널 B 출력 방식 설정 |
| **COM1C1, COM1C0** | 채널 C 출력 방식 설정 |
| **WGM11, WGM10** | 파형 생성 모드 설정 |

COM 비트는 각 채널의 반전 출력과 비반전 출력을 선택할 때 사용한다.
WGM11과 WGM10은 TCCR1B의 WGM13, WGM12와 함께 사용되며, 비트 조합에 따라 Normal, CTC, Fast PWM, Phase Correct PWM 등의 모드가 정해진다.

### TCCR1B

TCCR1B는 Input Capture 기능과 Timer1의 파형 생성 모드, Clock Source를 설정하는 레지스터이다.

| 비트 | 역할 |
| :--- | :--- |
| **ICNC1** | Input Capture 신호의 노이즈 제거 |
| **ICES1** | Input Capture Edge 선택 |
| **WGM13, WGM12** | 파형 생성 모드 설정 |
| **CS12, CS11, CS10** | Clock Source와 분주비 설정 |

ICNC1을 1로 설정하면 ICP1 핀으로 들어오는 신호에 노이즈 제거 기능이 적용된다.
ICES1이 1이면 상승 Edge를 검출하고, 0이면 하강 Edge를 검출한다.
CS12, CS11, CS10은 Timer1이 사용할 Clock과 분주비를 정할 때 사용한다.

### TCCR1C

TCCR1C는 Output Compare 동작을 강제로 발생시킬 때 사용한다.
FOC1A, FOC1B, FOC1C 비트를 이용하면 현재 Timer 값과 관계없이 해당 채널의 Output Compare 동작을 발생시킬 수 있다.

---

## 5. TCNT1, OCR1, ICR1

### TCNT1H, TCNT1L

TCNT1은 Timer1의 현재 카운트 값을 저장하는 16비트 레지스터이다.
16비트 값은 상위 바이트인 TCNT1H와 하위 바이트인 TCNT1L로 나누어 저장된다.

```text
0x0000 → 0x0001 → ... → 0xFFFF → 0x0000
```
TCNT1이 0xFFFF를 넘어 다시 0x0000이 되면 Overflow가 발생한다.

### OCR1xH, OCR1xL

OCR1A, OCR1B, OCR1C는 TCNT1과 비교할 값을 저장하는 16비트 레지스터이다.

| 채널 | 상위 바이트 | 하위 바이트 |
| :---: | :--- | :--- |
| **A** | OCR1AH | OCR1AL |
| **B** | OCR1BH | OCR1BL |
| **C** | OCR1CH | OCR1CL |

TCNT1 값과 OCR1x 값이 같아지면 Output Compare Match가 발생한다.
PWM 모드에서는 OCR1x 값에 따라 출력 펄스의 폭이 달라지므로 Duty Ratio를 조절할 때 사용한다.

### ICR1

ICR1은 ICP1 핀으로 Input Capture 신호가 들어왔을 때 현재 TCNT1 값을 저장하는 16비트 레지스터이다.
이를 이용하면 외부 신호가 입력된 순간의 Timer 값을 확인할 수 있다.
또한 Fast PWM 모드에서는 ICR1을 TOP 값으로 사용할 수 있다. 이 경우 ICR1은 Timer가 카운트할 최대값이 되며 PWM 주파수를 정하는 데 사용된다.

---

## 6. TIFR

TIFR은 Timer에서 Interrupt 조건이 발생했는지를 Flag로 나타내는 레지스터이다.
ICP1 핀의 신호로 인해 Input Capture가 발생하면 ICF1 비트가 1로 설정되어 Input Capture Interrupt가 요청된다. Interrupt 처리가 시작되면 해당 Flag는 다시 Clear된다.

---

## 7. PWM

PWM은 Pulse Width Modulation의 약자로, 펄스의 폭을 조절하는 방식이다.
ATmega128에서는 Timer1의 TCCR1A, TCCR1B, TCNT1, OCR1x, ICR1 등의 레지스터를 이용하여 PWM을 생성할 수 있다.
TCCR1A와 TCCR1B에서는 Fast PWM 또는 Phase Correct PWM과 같은 동작 모드를 설정한다. ICR1은 PWM의 TOP 값으로 사용하여 전체 주파수를 정하고, OCR1x는 출력 펄스의 폭을 정하여 Duty Ratio를 조절한다.
수업 자료와 같이 ICR1을 799로 설정한 경우, Duty Ratio에 따른 OCR1 값은 다음과 같다.

| Duty Ratio | OCR1 값 |
| :---: | :---: |
| **0%** | 0 |
| **25%** | 200 |
| **50%** | 400 |
| **75%** | 600 |
| **100%** | 799 |

ICR1은 PWM의 전체 주기와 주파수를 정하고, OCR1은 출력 펄스의 폭을 조절하는 역할을 한다.
Fast PWM은 Timer 값이 BOTTOM에서 TOP까지 증가한 뒤 다시 BOTTOM으로 돌아오는 방식이다. Phase Correct PWM은 Timer 값이 BOTTOM에서 TOP까지 증가한 후 다시 BOTTOM 방향으로 감소하는 방식이다.

---

## 8. Timer3

Timer3은 Timer1과 같은 16비트 Timer/Counter이며, 기본적인 동작 방식과 사용할 수 있는 기능도 Timer1과 비슷하다. Timer3도 Overflow, Compare Match, Input Capture, PWM 등에 사용할 수 있다.

---

## 9. 정리

Timer1과 Timer3은 ATmega128의 16비트 Timer/Counter이며, 시간 측정, Interrupt 발생, PWM 출력 등에 사용할 수 있다.
TCCR1A와 TCCR1B는 Timer의 동작 모드와 Clock을 설정하고, TCNT1은 현재 카운트 값을 저장한다. OCR1은 Compare Match와 PWM Duty Ratio를 정하며, ICR1은 Input Capture 값을 저장하거나 Fast PWM의 TOP 값으로 사용된다.

---

## 10. AI 툴 활용 명시

| 도구명 | 활용 영역 | 사용 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 정리 | Timer1, Timer3 및 PWM 관련 레지스터를 공부하고 내용을 정리하는 데 이용 |