# ATmega128 TIMER1 / TIMER3 레지스터 정리

> **광운대학교 AI로봇학과**  
> **작성자:** 정창규  
> **제출일:** 2026-08-03

---

## 1. Timer1과 Timer3

이번 수업에서는 ATmega128의 Timer1과 Timer3, 그리고 PWM 동작에 대해 학습하였다.
Timer1과 Timer3은 모두 16비트 Timer/Counter로, 카운트 값의 범위는 0x0000부터 0xFFFF까지이다. 8비트 Timer인 Timer0, Timer2보다 카운트 범위가 넓기 때문에 비교적 긴 시간을 측정하거나 PWM을 세밀하게 제어할 때 사용할 수 있다.
Timer가 동작하면 TCNT 값이 Clock에 맞추어 증가한다. 값이 최대 범위를 넘어가면 Overflow가 발생하고, TCNT와 OCR의 값이 같아지면 Compare Match가 발생한다. 또한 ICP 핀으로 외부 신호가 들어오면 그 순간의 TCNT 값이 ICR에 저장된다.

---

## 2. Timer1 제어 레지스터

### TCCR1A

TCCR1A는 Timer1의 출력 방식과 파형 생성 모드를 설정할 때 사용한다.
COM1A, COM1B, COM1C 비트는 Compare Match가 발생했을 때 각 출력 핀을 어떻게 동작시킬지 결정한다. WGM11과 WGM10은 TCCR1B에 있는 WGM13, WGM12와 함께 사용되며, 네 비트의 조합에 따라 Normal, CTC, Fast PWM, Phase Correct PWM 등의 모드가 정해진다.

| 비트 | 역할 |
| :--- | :--- |
| **COM1A1, COM1A0** | 채널 A 출력 방식 설정 |
| **COM1B1, COM1B0** | 채널 B 출력 방식 설정 |
| **COM1C1, COM1C0** | 채널 C 출력 방식 설정 |
| **WGM11, WGM10** | 파형 생성 모드 설정 |

### TCCR1B

TCCR1B에서는 Timer1에 사용할 Clock과 분주비를 설정한다. 또한 Input Capture 기능에서 어떤 Edge를 감지할지 정할 수 있다.
ICES1이 1이면 상승 Edge를 감지하고, 0이면 하강 Edge를 감지한다. CS12, CS11, CS10 비트의 조합으로 분주비 1, 8, 64, 256, 1024 또는 외부 Clock을 선택할 수 있다.

| 비트 | 역할 |
| :--- | :--- |
| **ICNC1** | Input Capture 신호의 노이즈 제거 |
| **ICES1** | 상승 또는 하강 Edge 선택 |
| **WGM13, WGM12** | 파형 생성 모드 설정 |
| **CS12, CS11, CS10** | Clock Source와 분주비 설정 |

### TCCR1C

TCCR1C는 Output Compare 동작을 강제로 발생시킬 때 사용한다.
FOC1A, FOC1B, FOC1C 비트를 이용하면 TCNT1과 OCR1x 값이 같아질 때까지 기다리지 않고 해당 채널의 Compare 동작을 발생시킬 수 있다.

---

## 3. TCNT1, OCR1, ICR1

### TCNT1

TCNT1은 Timer1의 현재 카운트 값을 저장하는 16비트 레지스터이다.
ATmega128은 8비트 MCU이므로 TCNT1 값은 상위 바이트인 TCNT1H와 하위 바이트인 TCNT1L로 나누어 저장된다.

```text
0x0000 → 0x0001 → ... → 0xFFFF → 0x0000
```
TCNT1이 0xFFFF에서 0x0000으로 돌아가면 Overflow가 발생한다.

### OCR1A, OCR1B, OCR1C

OCR1A, OCR1B, OCR1C는 TCNT1과 비교할 값을 저장하는 레지스터이다.
Timer가 동작하는 동안 TCNT1과 OCR1x의 값은 계속 비교된다. 두 값이 같아지면 Compare Match가 발생하고, 설정에 따라 Interrupt가 발생하거나 출력 핀의 상태가 바뀐다.
PWM에서는 OCR1x 값이 출력 펄스의 폭을 결정한다.

| 채널 | 상위 바이트 | 하위 바이트 |
| :---: | :--- | :--- |
| **A** | OCR1AH | OCR1AL |
| **B** | OCR1BH | OCR1BL |
| **C** | OCR1CH | OCR1CL |

### ICR1

ICR1은 Input Capture 신호가 들어왔을 때 현재 TCNT1 값을 저장하는 16비트 레지스터이다.
외부 신호가 들어온 순간의 Timer 값을 저장할 수 있으므로 펄스의 주기나 폭을 측정할 때 사용할 수 있다.
일부 PWM 모드에서는 ICR1을 TOP 값으로 사용한다. 이 경우 TCNT1은 0부터 ICR1에 저장된 값까지 카운트하며, ICR1 값에 따라 PWM 주파수가 결정된다.

---

## 4. Interrupt 관련 레지스터

Timer1의 주요 Interrupt는 TIMSK에서 활성화한다.

| 비트 | 역할 |
| :--- | :--- |
| **TICIE1** | Input Capture Interrupt 활성화 |
| **OCIE1A** | Compare Match A Interrupt 활성화 |
| **OCIE1B** | Compare Match B Interrupt 활성화 |
| **TOIE1** | Overflow Interrupt 활성화 |

Timer1의 채널 C와 Timer3의 Interrupt는 TIMSK가 아니라 ETIMSK에서 관리한다.

| 비트 | 역할 |
| :--- | :--- |
| **TICIE3** | Timer3 Input Capture Interrupt 활성화 |
| **OCIE3A** | Timer3 Compare Match A Interrupt 활성화 |
| **OCIE3B** | Timer3 Compare Match B Interrupt 활성화 |
| **TOIE3** | Timer3 Overflow Interrupt 활성화 |
| **OCIE3C** | Timer3 Compare Match C Interrupt 활성화 |
| **OCIE1C** | Timer1 Compare Match C Interrupt 활성화 |

Interrupt가 발생했는지는 TIFR과 ETIFR의 Flag를 통해 확인할 수 있다.
TIFR에는 Timer1의 ICF1, OCF1A, OCF1B, TOV1이 있으며, Timer1 채널 C의 OCF1C와 Timer3 관련 Flag는 ETIFR에 저장된다.
개별 Interrupt를 설정한 뒤 실제로 Interrupt를 사용하려면 `sei()`를 호출하여 전체 Interrupt도 활성화해야 한다.

```c
sei();
```

---

## 5. 16비트 레지스터 접근

ATmega128은 8비트 MCU이기 때문에 TCNT1, OCR1x, ICR1과 같은 16비트 레지스터는 상위 바이트와 하위 바이트로 나누어 처리한다.
직접 바이트 단위로 접근할 경우 값을 쓸 때는 상위 바이트를 먼저 쓰고, 값을 읽을 때는 하위 바이트를 먼저 읽어야 한다.

| 구분 | 접근 순서 |
| :--- | :--- |
| **Write** | 상위 바이트 → 하위 바이트 |
| **Read** | 하위 바이트 → 상위 바이트 |

AVR-GCC에서는 아래와 같이 16비트 레지스터 이름을 사용하면 컴파일러가 접근 순서를 처리해 준다.

```c
TCNT1 = 1000;
value = TCNT1;
```

---

## 6. PWM에서 사용하는 레지스터

ATmega128에는 PWM이라는 이름의 별도 레지스터가 있는 것은 아니다. Timer1의 여러 레지스터를 함께 설정하여 PWM 신호를 만든다.
TCCR1A와 TCCR1B에서는 PWM 모드, 출력 방식, 분주비를 설정한다. ICR1은 Timer가 카운트할 TOP 값을 저장하여 PWM 주파수를 정하고, OCR1A, OCR1B, OCR1C는 각 채널의 Duty Ratio를 정한다.
비반전 Fast PWM을 기준으로 OCR1x 값이 커지면 출력이 HIGH로 유지되는 시간이 길어져 Duty Ratio가 증가한다.

예를 들어 ICR1이 799인 경우 OCR1 값은 다음과 같이 설정할 수 있다.

| Duty Ratio | OCR1 값 |
| :---: | :---: |
| **0%** | 0 |
| **25%** | 200 |
| **50%** | 400 |
| **75%** | 600 |
| **100%** | 799 |

위 값은 Fast PWM을 기준으로 한 예시이다. Phase Correct PWM은 Timer가 증가와 감소를 반복하기 때문에 주파수 계산 방식이 다르다.

---

## 7. Timer3

Timer3은 Timer1과 구조가 비슷한 16비트 Timer/Counter이다.
Timer1의 TCCR1A/B/C, TCNT1, OCR1A/B/C, ICR1에 대응하여 Timer3에서는 TCCR3A/B/C, TCNT3, OCR3A/B/C, ICR3를 사용한다.
Timer3의 Interrupt 설정과 Flag 확인은 ETIMSK와 ETIFR에서 처리한다.

---

## 8. 정리

Timer1과 Timer3은 시간 측정, Interrupt 발생, PWM 출력 등에 사용할 수 있는 16비트 Timer/Counter이다.
TCCR 레지스터에서는 동작 모드와 분주비를 설정하고, TCNT에는 현재 카운트 값이 저장된다. OCR은 Compare Match와 PWM의 Duty Ratio를
결정하며, ICR은 Input Capture 값 또는 PWM의 TOP 값으로 사용된다.
Timer1의 채널 C와 Timer3의 Interrupt는 ETIMSK와 ETIFR에서 관리한다는 점도 함께 확인하였다.

---

## 9. AI 툴 활용 명시

| 도구명 | 활용 영역 | 사용 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 정리 | Timer1, Timer3 및 PWM 관련 레지스터를 공부하고 내용을 정리하는 데 이용 |