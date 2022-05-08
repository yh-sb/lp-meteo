#pragma once

#include "periph/pwm.hpp"
#include "tim_priv.hpp"
#include "stm32f4xx.h"

namespace periph::pwm_priv
{
constexpr pwm::ch_t const max_ch[tim::TIM_END] =
{
    pwm::CH_4, pwm::CH_4, pwm::CH_4, pwm::CH_4, pwm::CH_4, pwm::CH_END,
    pwm::CH_END, pwm::CH_4, pwm::CH_2, pwm::CH_1, pwm::CH_1, pwm::CH_2,
    pwm::CH_1, pwm::CH_1
};

constexpr uint32_t ccmr[pwm::CH_END][pwm::MODE_INVERTED + 1] = 
{
    {TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1, TIM_CCMR1_OC1M},
    {TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1, TIM_CCMR1_OC2M},
    {TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1, TIM_CCMR2_OC3M},
    {TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1, TIM_CCMR2_OC4M}
};

constexpr uint8_t gpio_af_list[tim::TIM_END] =
{
    // TIM2 for STM32F446xx (AF0) not implemented
    0x01, 0x01, 0x02,
    0x02, 0x02, 0x00,
    0x00, 0x03, 0x03,
    0x03, 0x03, 0x09,
    0x09, 0x09
};
};
