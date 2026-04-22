#include "adc.h"

void ADC_Init_Custom(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    // PA1 -> ADC input
    GPIOA->MODER &= ~GPIO_MODER_MODER1_Msk;
    GPIOA->MODER |= GPIO_MODER_MODER1; // analog mode

    // ADC configuration
    ADC1->CFGR1 = 0; // single conversion, 12-bit default
    ADC1->CHSELR = ADC_CHSELR_CHSEL1; // channel 1 -> PA1
    ADC1->SMPR |= ADC_SMPR_SMP_2 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_0;

    ADC1->ISR |= (ADC_ISR_ADRDY | ADC_ISR_EOC | ADC_ISR_EOSEQ | ADC_ISR_OVR);

    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {}

    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {}
}

uint16_t ADC_Read(void)
{
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) {}
    return (uint16_t)ADC1->DR;
}