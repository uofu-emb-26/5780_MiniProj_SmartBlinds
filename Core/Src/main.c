#include "main.h"
#include <stdio.h>
#include <string.h>
#include "motor.h"
#include "adc.h"
#include "uart.h"
#include "lcd.h"

void SystemClock_Config(void);
void Error_Handler(void);
void SysTick_Handler(void);

typedef enum {
  STATE_STOP,
  STATE_OPENING,
  STATE_CLOSING
  } State_t;

  typedef enum {
    MODE_MANUAL,
    MODE_AUTO
} Mode_t;
/* USER BUTTON PIN - onboard B1 on STM32F0-Discovery, active-HIGH */
#define USER_BTN_PORT GPIOA
#define USER_BTN_PIN  GPIO_IDR_0

/* LIGHT THRESHOLDS (can be changed)
 * Higher ADC value = brighter light
 * Bright (above 700) -> open blinds
 * Dark  (below 400)  -> close blinds */
#define LIGHT_OPEN_THRESHOLD   700
#define LIGHT_CLOSE_THRESHOLD  400

/* Discovery board LEDs: PC6=RED, PC7=BLUE, PC8=ORANGE, PC9=GREEN */
#define LED_RED_PIN    (1U << 6)
#define LED_BLUE_PIN   (1U << 7)
#define LED_ORANGE_PIN (1U << 8)
#define LED_GREEN_PIN  (1U << 9)
#define LED_ALL_PINS   (LED_RED_PIN | LED_BLUE_PIN | LED_ORANGE_PIN | LED_GREEN_PIN)

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    ADC_Init_Custom();
    UART3_Init_Custom();

    /* LED pins PC6-PC9 as outputs */
    GPIOC->MODER &= ~(GPIO_MODER_MODER6_Msk | GPIO_MODER_MODER7_Msk |
                       GPIO_MODER_MODER8_Msk | GPIO_MODER_MODER9_Msk);
    GPIOC->MODER |=  (GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 |
                       GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0);
    GPIOC->ODR &= ~LED_ALL_PINS; /* all LEDs off */

    // PA0 -> User button (B1 on Discovery board, active-high with external pull-down)
    GPIOA->MODER &= ~GPIO_MODER_MODER0_Msk; // input
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0_Msk; // no pull (board has external pull-down)

    // PA1 -> Light Sensor (ADC) - configured in ADC_Init_Custom

    // PA7 -> DIR output
    GPIOA->MODER &= ~GPIO_MODER_MODER7_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER7_0;

    // PB0: Manual Open/Close command button
    // PB2: Stop button
    GPIOB->MODER &= ~(GPIO_MODER_MODER0_Msk | GPIO_MODER_MODER2_Msk);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR0_Msk | GPIO_PUPDR_PUPDR2_Msk);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR2_0); // pull-up

    // PB10, PB11 -> Limit Switches (disabled for now)
    // GPIOB->MODER &= ~(GPIO_MODER_MODER10_Msk | GPIO_MODER_MODER11_Msk);
    // GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR10_Msk | GPIO_PUPDR_PUPDR11_Msk);
    // GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0); // pull-up

    // PA6 -> PWM (TIM3_CH1 alternate function)
    GPIOA->MODER &= ~GPIO_MODER_MODER6_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER6_1; // alternate function
    GPIOA->AFR[0] &= ~(0xF << (6 * 4));
    GPIOA->AFR[0] |= (1 << (6 * 4)); // AF1 for TIM3_CH1 on many F0 parts

    /* TIM3 CH1 PWM setup
     * HSI = 8 MHz, PSC = 7 -> timer counts at 1 MHz
     * ARR = 999 -> 1 kHz PWM frequency (1 MHz / 1000)
     * motor_open/close set CCR1 to control duty cycle */
    TIM3->PSC  = 7;       // 8 MHz / (7+1) = 1 MHz tick
    TIM3->ARR  = 999;     // 1 MHz / (999+1) = 1 kHz PWM
    TIM3->CCR1 = 0;       // start with 0% duty (motor off)
    TIM3->CCMR1 = (6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM mode 1 + preload
    TIM3->CCER = TIM_CCER_CC1E;  // enable CH1 output
    TIM3->CR1  = TIM_CR1_CEN;    // start timer

    // UART pins configured inside UART3_Init_Custom (PC4/PC5 -> USART3)

    // PB6/PB7 -> I2C1
    GPIOB->MODER &= ~(GPIO_MODER_MODER6_Msk | GPIO_MODER_MODER7_Msk);
    GPIOB->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1); // alternate function
    GPIOB->OTYPER |= (GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7); // open-drain
    GPIOB->AFR[0] &= ~((0xF << (6 * 4)) | (0xF << (7 * 4)));
    GPIOB->AFR[0] |=  ((1 << (6 * 4)) | (1 << (7 * 4))); // AF1 I2C1


    /* I2C1 CONFIG */

    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->TIMINGR = 0x2000090E; // common timing value for 48MHz / 100kHz
    I2C1->CR1 |= I2C_CR1_PE;

    LCD_Init();

    State_t state = STATE_STOP;
    Mode_t mode = MODE_MANUAL;

    uint8_t last_user_btn = 0; // active-high button, released = 0
    uint8_t motion_toggle = 0; // 0 -> next press opens, 1 -> next press closes

    /* Sentinel values so the first iteration always refreshes the LCD */
    Mode_t  prev_mode  = (Mode_t)0xFF;
    State_t prev_state = (State_t)0xFF;

    char msg[128];

    while (1)
  {
    uint16_t light = 0;
    for (int i = 0; i < 5; i++)
     {
        light += ADC_Read();
     }
     light /= 5;

      uint8_t motion_btn = ((GPIOB->IDR & GPIO_IDR_0) == 0); // PB0
      uint8_t stop_btn   = ((GPIOB->IDR & GPIO_IDR_2) == 0); // PB2

      uint8_t open_limit  = 0; // limit switches disabled
      uint8_t close_limit = 0;

      uint8_t user_btn = ((USER_BTN_PORT->IDR & USER_BTN_PIN) != 0);

      /* Toggle AUTO/MANUAL on user button press */
      if (user_btn && !last_user_btn)
      {
          mode = (mode == MODE_MANUAL) ? MODE_AUTO : MODE_MANUAL;

          snprintf(msg, sizeof(msg),
                  "Mode changed: %s\r\n",
                  (mode == MODE_MANUAL) ? "MANUAL" : "AUTO");

          UART3_SendString(msg);

          HAL_Delay(250); // simple debounce
      }
      last_user_btn = user_btn;

      /* FSM transition logic */
      switch (state)
      {
          case STATE_STOP:
              if (stop_btn)
              {
                  state = STATE_STOP;
              }
              else if (mode == MODE_MANUAL)
              {
                  if (motion_btn)
                  {
                      if (!motion_toggle && !open_limit)
                      {
                          state = STATE_OPENING;
                          motion_toggle = 1;
                      }
                      else if (motion_toggle && !close_limit)
                      {
                          state = STATE_CLOSING;
                          motion_toggle = 0;
                      }
                  }
              }
              else // AUTO mode: bright -> open, dark -> close
              {
                  if ((light > LIGHT_OPEN_THRESHOLD) && !open_limit)
                  {
                      state = STATE_OPENING;
                  }
                  else if ((light < LIGHT_CLOSE_THRESHOLD) && !close_limit)
                  {
                      state = STATE_CLOSING;
                  }
              }
              break;

          case STATE_OPENING:
              if (stop_btn || open_limit)
              {
                  state = STATE_STOP;
              }
              break;

          case STATE_CLOSING:
              if (stop_btn || close_limit)
              {
                  state = STATE_STOP;
              }
              break;
      }

      /* FSM output logic */
      switch (state)
      {
          case STATE_STOP:
              motor_stop();
              break;

          case STATE_OPENING:
              motor_open();
              break;

          case STATE_CLOSING:
              motor_close();
              break;
      }

      /* LED feedback:
       *   GREEN  = heartbeat (toggles each loop)
       *   BLUE   = AUTO mode
       *   ORANGE = motor opening
       *   RED    = motor closing
       */
      GPIOC->ODR ^= LED_GREEN_PIN;

      if (mode == MODE_AUTO)
          GPIOC->ODR |= LED_BLUE_PIN;
      else
          GPIOC->ODR &= ~LED_BLUE_PIN;

      if (state == STATE_OPENING)
          GPIOC->ODR |= LED_ORANGE_PIN;
      else
          GPIOC->ODR &= ~LED_ORANGE_PIN;

      if (state == STATE_CLOSING)
          GPIOC->ODR |= LED_RED_PIN;
      else
          GPIOC->ODR &= ~LED_RED_PIN;

      /* LCD feedback: only redraw when mode or state changes */
      if (mode != prev_mode || state != prev_state)
      {
          char *state_str = (state == STATE_STOP)    ? "STOP   " :
                            (state == STATE_OPENING) ? "OPENING" :
                                                        "CLOSING";

          LCD_Clear();
          LCD_SetCursor(0, 0);
          LCD_Print((mode == MODE_MANUAL) ? "Mode: MANUAL" : "Mode: AUTO  ");
          LCD_SetCursor(1, 0);
          LCD_Print("State: ");
          LCD_Print(state_str);

          prev_mode  = mode;
          prev_state = state;
      }

      /* UART debug output */
      snprintf(msg, sizeof(msg),
              "Mode=%s State=%d Light=%u UserBtn=%u MotionBtn=%u StopBtn=%u OpenLim=%u CloseLim=%u\r\n",
              (mode == MODE_MANUAL) ? "MANUAL" : "AUTO",
              state, light, user_btn, motion_btn, stop_btn, open_limit, close_limit);

      UART3_SendString(msg);
      HAL_Delay(200);
  }
}


    /**
    * @brief System Clock Configuration
    * @retval None
    */
    void SystemClock_Config(void)
    {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                    RCC_CLOCKTYPE_SYSCLK |
                                    RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
    }

    void Error_Handler(void)
    {
    __disable_irq();
    while (1)
    {

    }

    }

    void SysTick_Handler(void)
    {
        HAL_IncTick();
    }
