#include "uart.h"

void UART2_Init_Custom(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    // PA2/PA3 -> USART2
    GPIOA->MODER &= ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER3_Msk);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1); // alternate function

    GPIOA->AFR[0] &= ~((0xF << (2 * 4)) | (0xF << (3 * 4)));
    GPIOA->AFR[0] |=  ((1 << (2 * 4)) | (1 << (3 * 4))); // AF1 USART2

    USART2->CR1 = 0;
    USART2->BRR = HAL_RCC_GetHCLKFreq() / 115200;
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART2_SendString(const char *s)
{
    while (*s)
    {
        while (!(USART2->ISR & USART_ISR_TXE)) {}
        USART2->TDR = *s++;
    }
    while (!(USART2->ISR & USART_ISR_TC)) {}
}