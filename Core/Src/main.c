#include "main.h"
#include <stdio.h>
#include <string.h>

void SystemClock_Config(void);
void Error_Handler(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* GPIO Configuration */

    // PA0 -> Light Sensor (ADC) 
    GPIOA->MODER &= ~GPIO_MODER_MODER0_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER0;   // analog mode

    // PA7 -> DIR output
    GPIOA->MODER &= ~GPIO_MODER_MODER7_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER7_0;

    // PB0, PB1, PB2 -> Buttons 
    GPIOB->MODER &= ~(GPIO_MODER_MODER0_Msk | GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER2_Msk);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR0_Msk | GPIO_PUPDR_PUPDR1_Msk | GPIO_PUPDR_PUPDR2_Msk);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0 | GPIO_PUPDR_PUPDR2_0); // pull-up

    // PB10, PB11 -> Limit Switches 
    GPIOB->MODER &= ~(GPIO_MODER_MODER10_Msk | GPIO_MODER_MODER11_Msk);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR10_Msk | GPIO_PUPDR_PUPDR11_Msk);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0); // pull-up

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


    /* ADC Configuration */
    ADC1->CFGR1 = 0; // single conversion, 12-bit default
    ADC1->CHSELR = ADC_CHSELR_CHSEL0; // channel 0 -> PA0
    ADC1->SMPR |= ADC_SMPR_SMP_2 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_0;

    ADC1->ISR |= (ADC_ISR_ADRDY | ADC_ISR_EOC | ADC_ISR_EOSEQ | ADC_ISR_OVR);

    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {

    }

    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {

    }

    /* UART2 Configuration */
    USART2->CR1 = 0;
    USART2->BRR = HAL_RCC_GetHCLKFreq() / 115200;
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* TIM3 PWM Configuration */
    TIM3->PSC = 47; // 48 MHz / (47+1) = 1 MHz timer clock
    TIM3->ARR = 999; // PWM period = 1000 counts -> 1 kHz
    TIM3->CCR1 = 0; // start stopped

    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM3->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos);  // PWM mode 1
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM3->CCER |= TIM_CCER_CC1E;
    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR |= TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;


    /* I2C1 CONFIG */
 
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->TIMINGR = 0x2000090E; // common timing value for 48MHz / 100kHz
    I2C1->CR1 |= I2C_CR1_PE;

    while (1)
    {
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
}