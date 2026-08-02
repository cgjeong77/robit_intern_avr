# ATmega128 과제 1 - TIMER0 / TIMER2 보고서

> **광운대학교 AI로봇학과**  
> **작성자:** 정창규  
> **제출일:** 2026-08-02

---

# 1. 개요 (Overview)

본 과제는 ATmega128에서 제공하는 **Timer0**와 **Timer2**의 정의와 동작 원리, 그리고 사용 방법을 이해하는 것을 목표로 한다.

Timer는 MCU 내부 클럭을 이용하여 일정한 시간 간격을 생성하거나 인터럽트를 발생시키는 중요한 주변장치이다. LED 제어, PWM 출력, 시간 측정, 인터럽트 처리 등 다양한 기능을 구현할 때 사용된다.

### 핵심 목표

- Timer와 Counter의 개념 이해
- Timer0와 Timer2의 특징 이해
- Prescaler(분주비)의 역할 이해
- Timer Overflow Interrupt의 동작 원리 이해
- ATmega128에서 Timer0와 Timer2의 사용 방법 이해

---

# 2. 개발 환경 (Environment)

| 항목 | 내용 |
| :--- | :--- |
| **MCU** | ATmega128A (16MHz External Crystal) |
| **IDE / Compiler** | Microchip Studio 7.0 / AVR GCC |
| **언어** | C Language |
| **사용 주변장치** | Timer0, Timer2 |
| **클럭** | 16MHz |

---

# 3. Timer와 Counter의 정의

## 3-1. Counter

Counter는 외부에서 입력되는 클럭(Clock) 또는 펄스(Pulse)의 개수를 세는 기능이다.

ATmega128에서는 외부 핀(TOSC1, TOSC2, T1, T2, T3)을 통해 입력되는 신호의 상승 에지(Rising Edge)를 검출하여 이벤트의 발생 횟수를 계수한다.

## 3-2. Timer

Timer는 MCU 내부 클럭을 이용하여 일정한 시간 간격을 생성하는 기능이다.

설정된 시간이 지나면 인터럽트를 발생시키거나 PWM 출력과 같은 다양한 제어 기능에 활용된다.

---

# 4. Timer0와 Timer2

ATmega128의 Timer0와 Timer2는 모두 **8비트 Timer/Counter**이다.

따라서 카운터 값은 **0(0x00)부터 255(0xFF)**까지 증가하며, 최대값 이후에는 다시 0으로 돌아가면서 Overflow가 발생한다.

---

# 5. 동작 원리

Timer는 내부 클럭을 이용하여 TCNT 레지스터의 값을 1씩 증가시킨다.

동작 과정은 다음과 같다.

1. Timer 시작
2. 내부 클럭 입력
3. TCNT0 또는 TCNT2 값 증가
4. 255(0xFF)까지 카운트
5. 다음 클럭에서 0으로 변경
6. Overflow Interrupt 발생

이러한 과정을 반복하면서 일정한 시간 간격을 만들어 낸다.

---

# 6. Prescaler(분주비)

Prescaler는 입력되는 클럭을 일정 비율로 나누어 Timer의 동작 속도를 늦추는 기능이다.

대표적인 분주비는 다음과 같다.

- 1
- 8
- 32
- 64
- 128
- 256
- 1024

분주비를 크게 설정할수록 Timer의 동작 속도는 느려지고 더 긴 시간 간격을 만들 수 있다.

---

# 7. 주요 레지스터

| 레지스터 | 기능 |
| :--- | :--- |
| **TCCR0 / TCCR2** | Timer 동작 모드 및 분주비 설정 |
| **TCNT0 / TCNT2** | 현재 Timer 값을 저장 |
| **TIMSK** | Timer 인터럽트 허용 |
| **TIFR** | 인터럽트 발생 여부 확인 |
| **OCR0 / OCR2** | Compare Match 값 설정 |

---

# 8. Timer Interrupt

Timer Interrupt는 Timer가 특정 조건을 만족했을 때 자동으로 CPU에게 인터럽트를 발생시키는 기능이다.

대표적인 인터럽트는 다음과 같다.

| 종류 | 설명 |
| :--- | :--- |
| **Overflow Interrupt** | TCNT 값이 0xFF에서 0x00으로 넘어갈 때 발생 |
| **Compare Match Interrupt** | TCNT 값과 OCR 값이 같아질 때 발생 |
| **Capture Event** | 입력 신호를 저장하는 기능 |

TIMSK 레지스터를 이용하여 각 인터럽트를 활성화할 수 있다.

---

# 9. Timer0 / Timer2 사용 방법

ATmega128에서 Timer0와 Timer2를 사용하는 일반적인 과정은 다음과 같다.

1. TCCR0(TCCR2)에서 동작 모드 설정
2. Prescaler 설정
3. TCNT0(TCNT2)에 초기값 저장
4. TIMSK에서 인터럽트 활성화
5. 전역 인터럽트(sei()) 활성화
6. ISR(Interrupt Service Routine)에서 원하는 동작 수행

---

# 10. 활용 분야

Timer0와 Timer2는 다양한 임베디드 시스템에서 사용된다.

- LED 깜빡임
- 시간 측정
- Delay 생성
- PWM 출력
- 인터럽트 주기 생성
- 시계 구현
- 모터 제어

---

# 11. 장점과 단점

| 장점 | 단점 |
| :--- | :--- |
| 정확한 시간 생성 가능 | 레지스터 설정이 필요하다. |
| 인터럽트를 이용한 효율적인 제어 가능 | 분주비 계산이 필요하다. |
| PWM 출력 가능 | 8비트 Timer이므로 표현 가능한 범위가 제한된다. |
| 다양한 응용 가능 | 초기 설정 과정이 다소 복잡하다. |

---

# 12. 결론 (Conclusion)

ATmega128의 Timer0와 Timer2는 내부 클럭을 이용하여 시간을 측정하고 일정한 주기의 인터럽트를 발생시키는 중요한 주변장치이다. Timer는 Prescaler와 다양한 레지스터를 이용하여 원하는 시간 간격을 설정할 수 있으며, LED 제어, PWM 출력, 시계 구현, 모터 제어 등 다양한 임베디드 시스템에서 활용된다. 따라서 Timer의 동작 원리와 레지스터 설정 방법을 이해하는 것은 ATmega128 프로그래밍에서 매우 중요하다.

---

# 13. AI 툴 활용 명시 (AI Tools Declaration)

| 도구명 (Tool) | 활용 영역 | 세부 사용 목적 및 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 정리 | Timer0/Timer2의 정의, 동작 원리, 레지스터 및 사용 방법에 대한 개념을 정리한 후 학습에 활용 |
| **ATmega128 Datasheet** | 내용 검증 | Timer0/Timer2 관련 레지스터와 인터럽트 동작 원리 확인 |

### AI 활용 및 검증 원칙

1. AI를 활용하여 Timer0/Timer2의 개념을 정리한 후 공부에 이용하였다.
2. Timer 관련 레지스터와 동작 원리는 ATmega128 데이터시트 및 교육 자료를 참고하여 내용을 확인하였다.
3. AI는 개념 학습을 위한 보조 도구로만 활용하였다.