# ATmega128 과제 3 - TIMER1 / TIMER3 레지스터 설정 설명 보고서

> **광운대학교 AI로봇학과**  
> **작성자:** 정창규  
> **제출일:** (제출 날짜)

---

# 1. 개요 (Overview)

본 보고서는 ATmega128의 Timer1과 Timer3에서 사용하는 주요 레지스터의 기능을 정리한 것이다.
Timer1과 Timer3은 16비트 Timer/Counter로, 0부터 65535까지 카운트할 수 있다. 시간 측정뿐만 아니라 Overflow Interrupt, Compare Match, Input Capture, PWM 출력 등에 활용된다.

---

# 2. Timer1 / Timer3의 특징

| 항목 | 내용 |
| :--- | :--- |
| **구조** | 16비트 Timer/Counter |
| **카운트 범위** | 0x0000 ~ 0xFFFF |
| **Interrupt** | Overflow, Compare Match, Input Capture |
| **출력 비교 채널** | A, B, C |
| **주요 활용** | 시간 측정, PWM 출력, 모터 제어 |

Timer가 동작하면 TCNT 값이 Clock에 맞추어 증가한다. TCNT가 최대값을 넘어가면 Overflow가 발생하고, TCNT와 OCR 값이 같아지면 Compare Match가 발생한다.
또한 외부 ICP 핀으로 신호가 들어오면 그 순간의 TCNT 값이 ICR에 저장된다.

---

# 3. Timer1 주요 레지스터

| 레지스터 | 기능 |
| :--- | :--- |
| **TCCR1A** | 출력 방식과 파형 생성 모드 설정 |
| **TCCR1B** | Input Capture, 파형 생성 모드, 분주비 설정 |
| **TCCR1C** | 강제 Output Compare 설정 |
| **TCNT1H / TCNT1L** | 현재 Timer 카운트 값 저장 |
| **OCR1xH / OCR1xL** | Compare Match 기준값 저장 |
| **ICR1H / ICR1L** | Input Capture 값 또는 PWM TOP 값 저장 |
| **TIMSK / ETIMSK** | Timer Interrupt 활성화 |
| **TIFR / ETIFR** | Timer Interrupt 발생 상태 확인 |

---

# 4. Timer 제어 레지스터

## 4-1. TCCR1A

TCCR1A는 Timer1의 출력 방식과 파형 생성 모드 일부를 설정하는 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **COM1A1, COM1A0** | 채널 A 출력 방식 설정 |
| **COM1B1, COM1B0** | 채널 B 출력 방식 설정 |
| **COM1C1, COM1C0** | 채널 C 출력 방식 설정 |
| **WGM11, WGM10** | 파형 생성 모드 설정 |

COM 비트는 Compare Match가 발생했을 때 출력 핀을 어떻게 동작시킬지 결정한다.
WGM11과 WGM10은 TCCR1B의 WGM13, WGM12와 함께 사용된다. 네 비트의 조합에 따라 Normal, CTC, Fast PWM, Phase Correct PWM 등의 동작 모드가 결정된다.

## 4-2. TCCR1B

TCCR1B는 Timer1의 Clock, 분주비, 파형 생성 모드와 Input Capture 기능을 설정하는 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **ICNC1** | Input Capture 신호의 노이즈 제거 |
| **ICES1** | 상승 또는 하강 Edge 선택 |
| **WGM13, WGM12** | 파형 생성 모드 설정 |
| **CS12, CS11, CS10** | Clock Source와 분주비 설정 |

ICES1이 1이면 상승 Edge를 검출하고, 0이면 하강 Edge를 검출한다.
CS12, CS11, CS10의 조합을 이용하면 Timer 정지, 분주비 1, 8, 64, 256, 1024 또는 외부 Clock 입력을 선택할 수 있다.

## 4-3. TCCR1C

TCCR1C는 Output Compare 동작을 강제로 발생시키기 위해 사용하는 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **FOC1A** | 채널 A 강제 Compare |
| **FOC1B** | 채널 B 강제 Compare |
| **FOC1C** | 채널 C 강제 Compare |

FOC1x 비트를 사용하면 TCNT1과 OCR1x 값이 같아질 때까지 기다리지 않고 해당 채널의 Compare 동작을 발생시킬 수 있다.

---

# 5. 카운트, 비교 및 캡처 레지스터

## 5-1. TCNT1H, TCNT1L

TCNT1은 Timer1이 현재 어디까지 카운트했는지를 저장하는 16비트 레지스터이다.
ATmega128은 8비트 MCU이므로 상위 8비트는 TCNT1H에, 하위 8비트는 TCNT1L에 저장된다.

```text
0x0000 → 0x0001 → ... → 0xFFFF → 0x0000
```
TCNT1이 0xFFFF에서 0x0000으로 넘어가면 Overflow가 발생한다.

## 5-2. OCR1xH, OCR1xL

OCR1A, OCR1B, OCR1C는 TCNT1과 비교할 기준값을 저장하는 16비트 레지스터이다.

| 채널 | 상위 바이트 | 하위 바이트 |
| :---: | :--- | :--- |
| **A** | OCR1AH | OCR1AL |
| **B** | OCR1BH | OCR1BL |
| **C** | OCR1CH | OCR1CL |

Timer가 동작하는 동안 TCNT1과 OCR1x 값은 계속 비교된다. 두 값이 같아지면 Compare Match가 발생하며, 설정에 따라 Interrupt가 발생하거나 출력 핀의 상태가 바뀐다.
PWM 모드에서는 OCR1x 값이 출력 신호의 폭을 결정하므로, 모터의 속도와 같은 출력 세기를 조절하는 데 사용된다.

## 5-3. ICR1

ICR1은 외부 Input Capture 신호가 들어왔을 때 현재 TCNT1 값을 저장하는 16비트 레지스터이다.

| 레지스터 | 기능 |
| :--- | :--- |
| **ICR1H** | 상위 8비트 저장 |
| **ICR1L** | 하위 8비트 저장 |

외부 신호가 들어온 순간의 Timer 값을 저장할 수 있으므로 펄스의 주기나 폭을 측정할 때 사용한다.
일부 PWM 모드에서는 ICR1을 TOP 값으로 사용할 수 있다. 이 경우 TCNT1은 0부터 ICR1에 저장된 값까지 카운트하며, ICR1 값에 따라 PWM 주파수가 결정된다.

---

# 6. Interrupt 관련 레지스터

## 6-1. TIMSK

TIMSK는 Timer1의 주요 Interrupt를 활성화하는 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **TICIE1** | Timer1 Input Capture Interrupt 활성화 |
| **OCIE1A** | Timer1 Compare Match A Interrupt 활성화 |
| **OCIE1B** | Timer1 Compare Match B Interrupt 활성화 |
| **TOIE1** | Timer1 Overflow Interrupt 활성화 |

Timer1의 Compare Match 채널 C는 TIMSK가 아니라 ETIMSK의 `OCIE1C` 비트를 사용한다.

## 6-2. ETIMSK

ETIMSK는 Timer1 채널 C와 Timer3의 Interrupt를 활성화하는 확장 Interrupt Mask 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **TICIE3** | Timer3 Input Capture Interrupt 활성화 |
| **OCIE3A** | Timer3 Compare Match A Interrupt 활성화 |
| **OCIE3B** | Timer3 Compare Match B Interrupt 활성화 |
| **TOIE3** | Timer3 Overflow Interrupt 활성화 |
| **OCIE3C** | Timer3 Compare Match C Interrupt 활성화 |
| **OCIE1C** | Timer1 Compare Match C Interrupt 활성화 |

개별 Interrupt를 설정한 뒤에는 `sei()`를 사용하여 전체 Interrupt도 활성화해야 한다.

## 6-3. TIFR

TIFR은 Timer1에서 특정 조건이 발생했는지를 Flag로 나타내는 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **ICF1** | Timer1 Input Capture 발생 |
| **OCF1A** | Timer1 Compare Match A 발생 |
| **OCF1B** | Timer1 Compare Match B 발생 |
| **TOV1** | Timer1 Overflow 발생 |

Timer1 채널 C의 Compare Match Flag는 TIFR이 아니라 ETIFR의 `OCF1C` 비트에 저장된다.

## 6-4. ETIFR

ETIFR은 Timer1 채널 C와 Timer3에서 발생한 Interrupt 조건을 확인하는 확장 Interrupt Flag 레지스터이다.

| 비트 | 기능 |
| :--- | :--- |
| **ICF3** | Timer3 Input Capture 발생 |
| **OCF3A** | Timer3 Compare Match A 발생 |
| **OCF3B** | Timer3 Compare Match B 발생 |
| **TOV3** | Timer3 Overflow 발생 |
| **OCF3C** | Timer3 Compare Match C 발생 |
| **OCF1C** | Timer1 Compare Match C 발생 |

---

# 7. 16비트 레지스터 접근 시 주의사항

ATmega128은 8비트 MCU이므로 TCNT1, OCR1x, ICR1과 같은 16비트 레지스터는 상위 바이트와 하위 바이트로 나누어 처리한다.
직접 바이트 단위로 값을 기록할 때는 상위 바이트를 먼저 쓰고, 하위 바이트를 나중에 써야 한다. 값을 읽을 때는 반대로 하위 바이트를 먼저 읽고, 상위 바이트를 나중에 읽어야 한다.

| 구분 | 접근 순서 |
| :--- | :--- |
| **Write** | 상위 바이트 → 하위 바이트 |
| **Read** | 하위 바이트 → 상위 바이트 |

AVR-GCC에서는 다음과 같이 16비트 레지스터 이름을 사용하면 컴파일러가 필요한 순서를 처리한다.

```c
TCNT1 = 1000;
value = TCNT1;
```

---

# 8. PWM 관련 레지스터

ATmega128에는 PWM이라는 이름의 별도 레지스터가 있는 것이 아니라, Timer 관련 레지스터를 함께 설정하여 PWM을 만든다.

| 레지스터 | PWM에서의 역할 |
| :--- | :--- |
| **TCCR1A / TCCR1B** | PWM 모드, 출력 방식, 분주비 설정 |
| **TCNT1** | 현재 PWM 카운트 값 저장 |
| **ICR1** | PWM의 TOP 값과 주파수 결정 |
| **OCR1A / OCR1B / OCR1C** | 각 채널의 Duty Ratio 결정 |

PWM을 사용할 때는 TCCR1A와 TCCR1B에서 동작 모드와 분주비를 설정한다. 이후 ICR1에 TOP 값을 저장하면 PWM의 전체 주기와 주파수가 결정된다.
OCR1x에는 출력 상태가 바뀌는 기준값을 저장한다. OCR1x 값이 커질수록 출력 신호가 HIGH로 유지되는 시간이 길어지기 때문에 Duty Ratio가 증가한다.

예를 들어 Fast PWM 모드에서 ICR1이 799라면 OCR1 값은 다음과 같이 설정할 수 있다.

| Duty Ratio | OCR1 값 |
| :---: | :---: |
| **0%** | 0 |
| **25%** | 200 |
| **50%** | 400 |
| **75%** | 600 |
| **100%** | 799 |

위 값은 Fast PWM을 기준으로 한 예시이다. Phase Correct PWM은 Timer가 증가와 감소를 반복하므로 주파수 계산 방식이 다르다.

---

# 9. Timer3 관련 레지스터

Timer3은 Timer1과 구조가 비슷한 16비트 Timer/Counter이다.
Timer1의 TCCR1A/B/C, TCNT1, OCR1A/B/C, ICR1에 대응하여 Timer3에서는 TCCR3A/B/C, TCNT3, OCR3A/B/C, ICR3를 사용한다.
Timer3의 Interrupt 설정과 Flag 확인은 ETIMSK와 ETIFR에서 처리한다.

---

# 10. 결론 (Conclusion)

Timer1과 Timer3은 16비트 Timer/Counter로 시간 측정과 PWM 출력에 사용할 수 있다.
TCCR 레지스터는 Timer의 동작 모드와 분주비를 설정하고, TCNT는 현재 카운트 값을 저장한다. OCR은 Compare Match 기준값과 PWM Duty Ratio를 결정하며, ICR은 Input Capture 값 또는 PWM의 TOP 값으로 사용된다.
또한 Timer1의 채널 C와 Timer3의 Interrupt는 ETIMSK와 ETIFR에서 관리한다.

---

# 11. AI 툴 활용 명시 (AI Tools Declaration)

| 도구명 (Tool) | 활용 영역 | 세부 사용 목적 및 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 정리 | Timer1/Timer3 및 PWM 관련 레지스터의 개념을 정리한 후 공부에 이용 |