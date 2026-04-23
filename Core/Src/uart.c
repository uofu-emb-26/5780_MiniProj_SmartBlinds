#include "uart.h"

void UART2_Init_Custom(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    // PC4/PC5 -> USART3
    GPIOC->MODER &= ~(GPIO_MODER_MODER4_Msk | GPIO_MODER_MODER5_Msk);
    GPIOC->MODER |= (GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1); // alternate function

    GPIOC->AFR[0] &= ~((0xF << (4 * 4)) | (0xF << (5 * 4)));
    GPIOC->AFR[0] |=  ((1 << (4 * 4)) | (1 << (5 * 4))); // AF1 USART3

    USART3->CR1 = 0;
    USART3->BRR = HAL_RCC_GetHCLKFreq() / 115200;
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART2_SendString(const char *s)
{
    while (*s)
    {
        while (!(USART3->ISR & USART_ISR_TXE)) {}
        USART3->TDR = *s++;
    }
    while (!(USART3->ISR & USART_ISR_TC)) {}
}