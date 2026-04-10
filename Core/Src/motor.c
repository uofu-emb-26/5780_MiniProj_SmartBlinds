#include "motor.h"

// PA7 = direction
// TIM3_CH1 (PA6) = PWM

void motor_open(void) 
{
    // PA7 high -> open direction
    GPIOA->ODR |= (1 << 7);

    // default speed = 75%
    TIM3->CCR1 = (75 * TIM3->ARR) / 100;
}

void motor_close(void) 
{
    // PA7 low -> close direction
    GPIOA->ODR &= ~(1 << 7);

    // default speed = 75%
    TIM3->CCR1 = (75 * TIM3->ARR) / 100;
}

void motor_stop(void) 
{
    // stop motor
    TIM3->CCR1 = 0;
}

void motor_set_speed(uint8_t duty)
{
    if (duty > 100) 
    {
        duty = 100;
    }

    TIM3->CCR1 = ((uint32_t)duty * TIM3->ARR) / 100;
}