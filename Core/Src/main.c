#include "main.h"
#include <stdio.h>
#include <string.h>
#include "motor.h"
#include "adc.h"
#include "uart.h"

void SystemClock_Config(void);
void Error_Handler(void);

typedef enum {
  STATE_STOP,
  STATE_OPENING,
  STATE_CLOSING
  } State_t;

  typedef enum {
    MODE_MANUAL,
    MODE_AUTO
} Mode_t;
/* USER BUTTON PIN */
#define USER_BTN_PORT GPIOC
#define USER_BTN_PIN  GPIO_IDR_13

/* LIGHT THRESHOLDS (can be changed) */
#define LIGHT_OPEN_THRESHOLD   400
#define LIGHT_CLOSE_THRESHOLD  700

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
    UART2_Init_Custom();

    // PA0 -> Light Sensor (ADC) 
    GPIOA->MODER &= ~GPIO_MODER_MODER0_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER0; // analog mode

    // PA7 -> DIR output
    GPIOA->MODER &= ~GPIO_MODER_MODER7_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER7_0;

    // PB0: Manual Open/Close command button
    // PB2: Stop button
    GPIOB->MODER &= ~(GPIO_MODER_MODER0_Msk | GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER2_Msk);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR0_Msk | GPIO_PUPDR_PUPDR1_Msk | GPIO_PUPDR_PUPDR2_Msk);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0 | GPIO_PUPDR_PUPDR2_0); // pull-up

    // PB10, PB11 -> Limit Switches 
    GPIOB->MODER &= ~(GPIO_MODER_MODER10_Msk | GPIO_MODER_MODER11_Msk);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR10_Msk | GPIO_PUPDR_PUPDR11_Msk);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0); // pull-up

    // User button input (change pin if needed)
    USER_BTN_PORT->MODER &= ~(0x3u << (13 * 2)); // input mode for PC13
    USER_BTN_PORT->PUPDR &= ~(0x3u << (13 * 2)); // no pull

    // PA6 -> PWM (TIM3_CH1 alternate function) 
    GPIOA->MODER &= ~GPIO_MODER_MODER6_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER6_1; // alternate function
    GPIOA->AFR[0] &= ~(0xF << (6 * 4));
    GPIOA->AFR[0] |= (1 << (6 * 4)); // AF1 for TIM3_CH1 on many F0 parts

    // PA2/PA3 -> UART2 
    GPIOA->MODER &= ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER3_Msk);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1); // alternate function
    GPIOA->AFR[0] &= ~((0xF << (2 * 4)) | (0xF << (3 * 4)));
    GPIOA->AFR[0] |=  ((1 << (2 * 4)) | (1 << (3 * 4))); // AF1 USART2

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

    State_t state = STATE_STOP;
    Mode_t mode = MODE_MANUAL;

    uint8_t last_user_btn = 1; // active-low button, released = 1
    uint8_t motion_toggle = 0; // 0 -> next press opens, 1 -> next press closes

    char msg[128];

    while (1)
  {
      uint16_t light = ADC_Read();

      uint8_t motion_btn = ((GPIOB->IDR & GPIO_IDR_0) == 0); // PB0
      uint8_t stop_btn   = ((GPIOB->IDR & GPIO_IDR_2) == 0); // PB2

      uint8_t open_limit  = ((GPIOB->IDR & GPIO_IDR_10) == 0);
      uint8_t close_limit = ((GPIOB->IDR & GPIO_IDR_11) == 0);

      uint8_t user_btn = ((USER_BTN_PORT->IDR & USER_BTN_PIN) == 0);

      /* Toggle AUTO/MANUAL on user button press */
      if (user_btn && !last_user_btn)
      {
          mode = (mode == MODE_MANUAL) ? MODE_AUTO : MODE_MANUAL;

          snprintf(msg, sizeof(msg),
                  "Mode changed: %s\r\n",
                  (mode == MODE_MANUAL) ? "MANUAL" : "AUTO");

          UART2_SendString(msg);

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
              else // AUTO mode
              {
                  if ((light > LIGHT_CLOSE_THRESHOLD) && !close_limit)
                  {
                      state = STATE_CLOSING;
                  }
                  else if ((light < LIGHT_OPEN_THRESHOLD) && !open_limit)
                  {
                      state = STATE_OPENING;
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

      /* UART debug output */
      snprintf(msg, sizeof(msg),
              "Mode=%s State=%d Light=%u MotionBtn=%u StopBtn=%u OpenLim=%u CloseLim=%u\r\n",
              (mode == MODE_MANUAL) ? "MANUAL" : "AUTO",
              state, light, motion_btn, stop_btn, open_limit, close_limit);

      UART2_SendString(msg);
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
