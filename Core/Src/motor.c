#include "motor.h"

#define MOTOR_DUTY_PCT 75

// PA4 = ENABLE PWM using TIM14_CH1
// PA5 = IN_1
// PA9 = IN_2

void motor_open(void)
{
    // Clockwise = opening
    GPIOA->ODR |=  (1 << 5);   // IN_1 HIGH
    GPIOA->ODR &= ~(1 << 9);   // IN_2 LOW

    TIM14->CCR1 = (MOTOR_DUTY_PCT * TIM14->ARR) / 100;
}

void motor_close(void)
{
    // Counter-clockwise = closing
    GPIOA->ODR &= ~(1 << 5);   // IN_1 LOW
    GPIOA->ODR |=  (1 << 9);   // IN_2 HIGH

    TIM14->CCR1 = (MOTOR_DUTY_PCT * TIM14->ARR) / 100;
}

void motor_stop(void)
{
    TIM14->CCR1 = 0;

    GPIOA->ODR &= ~(1 << 5);   // IN_1 LOW
    GPIOA->ODR &= ~(1 << 9);   // IN_2 LOW
}

void motor_set_speed(uint8_t duty)
{
    if (duty > 100)
    {
        duty = 100;
    }

    TIM14->CCR1 = ((uint32_t)duty * TIM14->ARR) / 100;
}
