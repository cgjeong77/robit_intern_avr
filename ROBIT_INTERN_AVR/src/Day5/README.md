# ATmega128 TIMER1 / TIMER3 레지스터 설정 정리

> **광운대학교 AI로봇학과**  
> **작성자:** 정창규  
> **제출일:** 2026-08-03

---

## 1. Timer1과 Timer3

Timer1과 Timer3은 ATmega128에 포함된 16비트 Timer/Counter이다. 두 Timer는 0x0000부터 0xFFFF까지 카운트할 수 있으며, 시간 측정, Overflow, Compare Match, Input Capture, PWM 출력 등에 사용할 수 있다.

---

## 2. Timer/Counter1 관련 레지스터

Timer1에서 사용하는 주요 레지스터는 다음과 같다.

```text
TCCR1A, TCCR1B, TCCR1C
TCNT1H, TCNT1L
OCR1AH, OCR1AL
OCR1BH, OCR1BL
OCR1CH, OCR1CL
ICR1H, ICR1L
SFIOR
TIMSK, ETIMSK
TIFR, ETIFR
```

TCCR1A, TCCR1B, TCCR1C는 Timer1의 동작 방식을 설정하며, TCNT1은 현재 카운트 값을 저장한다. OCR1은 비교할 기준값을 저장하고, ICR1은 외부 신호가 입력된 순간의 카운트 값을 저장하거나 PWM의 TOP 값으로 사용할 수 있다.

---

## 3. TCCR1A

TCCR1A는 Timer1의 출력 방식과 파형 생성 모드를 설정하는 레지스터이다.
COM1A, COM1B, COM1C 비트는 각 채널의 출력 방식을 설정한다. 출력 단자를 사용하지 않거나, 비반전 출력 또는 반전 출력으로 사용할 수 있다.
WGM11과 WGM10은 파형 생성 모드를 설정하는 비트이다. 이 비트들은 TCCR1B의 WGM13, WGM12와 함께 사용되며, 조합에 따라 Normal, CTC, Fast PWM, Phase Correct PWM 등의 모드가 결정된다.

---

## 4. TCCR1B

TCCR1B는 Input Capture 기능, 파형 생성 모드, Timer Clock을 설정하는 레지스터이다.
ICNC1은 ICP1 핀으로 들어오는 Input Capture 신호의 노이즈를 제거할 때 사용한다. 노이즈 제거 기능을 사용하면 입력 신호가 시스템 Clock의 4주기만큼 지연되어 처리된다.
ICES1은 Input Capture에서 감지할 Edge를 선택한다. 값이 0이면 Falling Edge를 감지하고, 1이면 Rising Edge를 감지한다.
WGM13과 WGM12는 TCCR1A의 WGM11, WGM10과 함께 Timer1의 동작 모드를 설정한다. CS12, CS11, CS10은 Timer1에서 사용할 Clock과 분주비를 선택하는 데 사용한다.

---

## 5. TCCR1C

TCCR1C는 Output Compare 동작을 소프트웨어로 강제로 발생시키는 데 사용한다.
FOC1A, FOC1B, FOC1C는 각각 채널 A, B, C의 Compare Match 동작을 강제로 발생시킨다. 주로 Normal 또는 CTC 모드에서 사용하며, PWM 모드에서는 사용하지 않고 0으로 설정한다.

---

## 6. TCNT1H, TCNT1L

TCNT1은 Timer1의 현재 카운트 값을 저장하는 16비트 레지스터이다.
ATmega128은 8비트 MCU이기 때문에 16비트 값을 상위 바이트와 하위 바이트로 나누어 저장한다. TCNT1H에는 상위 8비트가 저장되고, TCNT1L에는 하위 8비트가 저장된다.

```text
0x0000 → 0x0001 → ... → 0xFFFF → 0x0000
```

카운트 값이 0xFFFF를 넘어 다시 0x0000이 되면 Overflow가 발생한다.

---

## 7. OCR1xH, OCR1xL

OCR1A, OCR1B, OCR1C는 TCNT1과 비교할 기준값을 저장하는 16비트 레지스터이다.

```text
채널 A : OCR1AH, OCR1AL
채널 B : OCR1BH, OCR1BL
채널 C : OCR1CH, OCR1CL
```

Timer가 동작하는 동안 TCNT1과 OCR1x의 값이 계속 비교된다. 두 값이 같아지면 Compare Match가 발생하며, 설정에 따라 Interrupt가 발생하거나 OC1x 출력 단자의 상태가 바뀐다.
PWM 모드에서는 OCR1x 값에 따라 출력 펄스의 폭이 달라지므로 Duty Ratio를 조절하는 데 사용한다.

---

## 8. ICR1H, ICR1L

ICR1은 ICP1 핀으로 Input Capture 신호가 들어왔을 때 현재 TCNT1 값을 저장하는 16비트 레지스터이다.
외부 신호가 입력된 순간의 카운트 값을 저장할 수 있기 때문에 신호의 주기나 펄스 폭을 측정할 때 사용할 수 있다.
Fast PWM 모드에서는 ICR1을 Timer의 TOP 값으로 사용할 수 있다. 이 경우 ICR1에 저장된 값이 Timer의 최대 카운트 값이 되며, PWM 주파수를 결정하는 데 사용된다.

---

## 9. SFIOR

SFIOR은 Timer의 Prescaler와 MCU의 Pull-up 기능 등을 설정하는 레지스터이다.
PSR0은 Timer0의 Prescaler를 초기화하고, PSR321은 Timer1, Timer2, Timer3이 공유하는 Prescaler를 초기화한다.
TSM은 여러 Timer의 동작 시점을 맞출 때 사용한다. Prescaler를 초기화한 뒤 여러 Timer를 동시에 시작해야 하는 경우에 활용할 수 있다.
PUD는 ATmega128 내부 Pull-up 저항을 전체적으로 비활성화하는 기능을 한다.

---

## 10. TIMSK와 ETIMSK

TIMSK와 ETIMSK는 Timer Interrupt의 사용 여부를 설정하는 레지스터이다.
TIMSK에서 Timer1과 관련된 비트는 다음과 같다.

- **TICIE1** : Timer1 Input Capture Interrupt 활성화
- **OCIE1A** : Timer1 채널 A Compare Match Interrupt 활성화
- **OCIE1B** : Timer1 채널 B Compare Match Interrupt 활성화
- **TOIE1** : Timer1 Overflow Interrupt 활성화

Timer1 채널 C와 Timer3의 Interrupt는 ETIMSK에서 설정한다.

- **TICIE3** : Timer3 Input Capture Interrupt 활성화
- **OCIE3A, OCIE3B, OCIE3C** : Timer3 채널 A, B, C Compare Match Interrupt 활성화
- **TOIE3** : Timer3 Overflow Interrupt 활성화
- **OCIE1C** : Timer1 채널 C Compare Match Interrupt 활성화

각 비트를 1로 설정하면 해당 Interrupt를 사용할 수 있다. 실제로 Interrupt가 실행되려면 SREG의 전체 Interrupt 허용 비트도 활성화되어 있어야 한다.

---

## 11. TIFR과 ETIFR

TIFR과 ETIFR은 Timer에서 특정 Event가 발생했는지를 나타내는 Flag 레지스터이다.
TIFR에서는 Timer1의 Input Capture, 채널 A와 B의 Compare Match, Overflow 발생 여부를 확인할 수 있다.

- **ICF1** : Timer1 Input Capture 발생
- **OCF1A, OCF1B** : Timer1 채널 A, B Compare Match 발생
- **TOV1** : Timer1 Overflow 발생

ETIFR에서는 Timer3의 Event와 Timer1 채널 C의 Compare Match 발생 여부를 확인할 수 있다.

- **ICF3** : Timer3 Input Capture 발생
- **OCF3A, OCF3B, OCF3C** : Timer3 채널 A, B, C Compare Match 발생
- **TOV3** : Timer3 Overflow 발생
- **OCF1C** : Timer1 채널 C Compare Match 발생

해당 Event가 발생하면 하드웨어가 관련 Flag를 자동으로 1로 설정한다.

---

## 12. PWM 관련 설정

ATmega128에는 PWM이라는 이름의 별도 레지스터가 있는 것이 아니라 Timer 관련 레지스터를 함께 사용하여 PWM을 생성한다.
TCCR1A와 TCCR1B에서는 PWM 모드, 출력 방식, Clock 분주비를 설정한다. TCNT1에는 현재 카운트 값이 저장되며, ICR1은 PWM의 TOP 값으로 사용하여 주파수를 결정한다. OCR1A, OCR1B, OCR1C는 각 채널의 Duty Ratio를 설정한다.
수업 자료와 같이 ICR1을 799로 설정한 경우 OCR1 값은 다음과 같이 설정할 수 있다.

| Duty Ratio | OCR1 값 |
| :---: | :---: |
| **0%** | 0 |
| **25%** | 200 |
| **50%** | 400 |
| **75%** | 600 |
| **100%** | 799 |

ICR1은 PWM의 전체 주기를 결정하고, OCR1x는 출력 펄스의 폭을 조절한다.

---

## 13. 정리

Timer1과 Timer3은 16비트 Timer/Counter로 시간 측정, Compare Match, Input Capture, Overflow Interrupt, PWM 등에 사용할 수 있다.
TCCR1A, TCCR1B, TCCR1C는 Timer1의 동작 방식과 출력 방식을 설정한다. TCNT1은 현재 카운트 값을 저장하고, OCR1x는 TCNT1과 비교할 기준값을 저장한다. ICR1은 Input Capture 값을 저장하거나 Fast PWM에서 TOP 값으로 사용된다.
TIMSK와 ETIMSK는 Interrupt를 활성화하는 데 사용하며, TIFR과 ETIFR은 Timer에서 Event가 발생했는지를 확인하는 데 사용한다.

---

## 14. AI 툴 활용 명시

| 도구명 | 활용 영역 | 사용 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 정리 | Timer1, Timer3 및 PWM 관련 레지스터를 공부하고 내용을 정리하는 데 이용 |