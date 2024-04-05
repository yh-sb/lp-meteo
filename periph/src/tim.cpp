#include <stddef.h>
#include <limits>
#include <assert.h>
#include "periph/tim.hpp"
#include "periph/rcc.hpp"
#include "tim_priv.hpp"
#include "stm32f4xx.h"
#include "core_cm4.h"
#include "FreeRTOS.h"

using namespace periph;

static tim *obj_list[tim::TIM_END];

tim::tim(tim_t tim):
    _tim(tim),
    _us(0),
    _ctx(NULL),
    _cb(NULL)
{
    assert(tim < TIM_END && tim_priv::tim[_tim]);
    
    *tim_priv::rcc_en_reg[_tim] |= tim_priv::rcc_en[_tim];
    *tim_priv::rcc_rst_reg[_tim] |= tim_priv::rcc_rst[_tim];
    *tim_priv::rcc_rst_reg[_tim] &= ~tim_priv::rcc_rst[_tim];
    
    obj_list[_tim] = this;
    
    /* Allow that only counter overflow/underflow generates irq
    (avoid irq generation when set UG bit) */
    tim_priv::tim[_tim]->CR1 |= TIM_CR1_URS;
    
    // Enable interrupt
    tim_priv::tim[_tim]->DIER |= TIM_DIER_UIE;
    
    NVIC_SetPriority(tim_priv::irqn[_tim],
        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(tim_priv::irqn[_tim]);
}

tim::~tim()
{
    NVIC_DisableIRQ(tim_priv::irqn[_tim]);
    tim_priv::tim[_tim]->CR1 &= ~TIM_CR1_CEN;
    *tim_priv::rcc_en_reg[_tim] &= ~tim_priv::rcc_en[_tim];
    obj_list[_tim] = NULL;
}

void tim::cb(cb_t cb, void *ctx)
{
    _cb = cb;
    _ctx = ctx;
}

void tim::us(uint32_t us)
{
    assert(us > 0);
    
    _us = us;
    uint16_t psc, arr;
    calc_clk(_tim, _us, psc, arr);
    
    TIM_TypeDef *tim = tim_priv::tim[_tim];
    tim->PSC = psc;
    tim->ARR = arr;
    
    // Update ARR, PSC and clear CNT register
    tim->EGR = TIM_EGR_UG;
}

void tim::start(bool is_cyclic)
{
    assert(_us > 0);
    assert(_cb);
    TIM_TypeDef *tim = tim_priv::tim[_tim];
    // This action allowed only when TIM is disabled
    assert(!(tim->CR1 & TIM_CR1_CEN));
    
    if(is_cyclic)
        tim->CR1 &= ~TIM_CR1_OPM;
    else
        tim->CR1 |= TIM_CR1_OPM;
    
    tim->CNT = 0;
    tim->CR1 |= TIM_CR1_CEN;
}

void tim::stop()
{
    TIM_TypeDef *tim = tim_priv::tim[_tim];
    
    tim->CR1 &= ~TIM_CR1_CEN;
    tim->CNT = 0;
    tim->SR &= ~TIM_SR_UIF;
}

bool tim::is_expired() const
{
    return !(tim_priv::tim[_tim]->CR1 & TIM_CR1_CEN);
}

void tim::calc_clk(tim_t tim, uint32_t us, uint16_t &psc, uint16_t &arr)
{
    uint32_t clk_freq = rcc_get_freq(tim_priv::rcc_src[tim]);
    // If APBx prescaller no equal to 1, TIMx prescaller multiplies by 2
    if(clk_freq != rcc_get_freq(RCC_SRC_AHB))
        clk_freq *= 2;
    
    uint32_t tmp_arr = us * (clk_freq / 1000000);
    constexpr auto tim_resol = std::numeric_limits<uint16_t>::max();
    uint32_t tmp_psc = 1;
    
    if(tmp_arr > tim_resol)
    {
        // tmp_arr is too big for ARR register (16 bit), increase the prescaler
        tmp_psc = ((tmp_arr + (tim_resol / 2)) / tim_resol) + 1;
        tmp_arr /= tmp_psc;
    }
    
    assert(tmp_psc <= tim_resol);
    assert(tmp_arr <= tim_resol);
    
    psc = tmp_psc - 1;
    arr = tmp_arr - 1;
}

extern "C" void tim_irq_hndlr(periph::tim *obj)
{
    TIM_TypeDef *tim = tim_priv::tim[obj->_tim];
    uint32_t sr = tim->SR;
    
    if((tim->DIER & TIM_DIER_UIE) && (sr & TIM_SR_UIF))
        tim->SR &= ~TIM_SR_UIF;
    else if((tim->DIER & TIM_DIER_CC1IE) && (sr & TIM_SR_CC1IF))
        tim->SR &= ~TIM_SR_CC1IF;
    else if((tim->DIER & TIM_DIER_CC2IE) && (sr & TIM_SR_CC2IF))
        tim->SR &= ~TIM_SR_CC2IF;
    else if((tim->DIER & TIM_DIER_CC3IE) && (sr & TIM_SR_CC3IF))
        tim->SR &= ~TIM_SR_CC3IF;
    else if((tim->DIER & TIM_DIER_CC4IE) && (sr & TIM_SR_CC4IF))
        tim->SR &= ~TIM_SR_CC4IF;
    else
        return;
    
    if(obj->_cb)
        obj->_cb(obj, obj->_ctx);
}

#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
    defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
    defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
    defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
    defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
    defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void TIM2_IRQHandler(void)
{
    tim_irq_hndlr(obj_list[tim::TIM_2]);
}

extern "C" void TIM3_IRQHandler(void)
{
    tim_irq_hndlr(obj_list[tim::TIM_3]);
}

extern "C" void TIM4_IRQHandler(void)
{
    tim_irq_hndlr(obj_list[tim::TIM_4]);
}
#endif

extern "C" void TIM5_IRQHandler(void)
{
    tim_irq_hndlr(obj_list[tim::TIM_5]);
}

#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F410Cx) || \
    defined(STM32F410Rx) || defined(STM32F410Tx) || defined(STM32F412Cx) || \
    defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
    defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
    defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
    defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void TIM6_DAC_IRQHandler(void)
{
    tim_irq_hndlr(obj_list[tim::TIM_6]);
}
#endif

#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F412Cx) || \
    defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
    defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
    defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
    defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void TIM7_IRQHandler(void)
{
    tim_irq_hndlr(obj_list[tim::TIM_7]);
}
#endif

extern "C" void TIM1_BRK_TIM9_IRQHandler(void)
{
    if((TIM1->DIER & TIM_DIER_BIE) && (TIM1->SR & TIM_SR_BIF))
        tim_irq_hndlr(obj_list[tim::TIM_1]);
    else
        tim_irq_hndlr(obj_list[tim::TIM_9]);
}

#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
    defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
    defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
    defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
    defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
    defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void TIM1_UP_TIM10_IRQHandler(void)
{
    if((TIM1->DIER & TIM_DIER_UIE) && (TIM1->SR & TIM_SR_UIF))
        tim_irq_hndlr(obj_list[tim::TIM_1]);
    else
        tim_irq_hndlr(obj_list[tim::TIM_10]);
}
#endif

extern "C" void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
    uint32_t sr = TIM1->SR;
    
    if(((TIM1->DIER & TIM_DIER_COMIE) && (sr & TIM_SR_COMIF)) ||
        ((TIM1->DIER & TIM_DIER_TIE) && (sr & TIM_SR_TIF)))
    {
        tim_irq_hndlr(obj_list[tim::TIM_1]);
    }
    else
        tim_irq_hndlr(obj_list[tim::TIM_11]);
}

#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F412Cx) || \
    defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
    defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
    defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
    defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void TIM8_BRK_TIM12_IRQHandler(void)
{
    if((TIM8->DIER & TIM_DIER_BIE) && (TIM8->SR & TIM_SR_BIF))
        tim_irq_hndlr(obj_list[tim::TIM_8]);
    else
        tim_irq_hndlr(obj_list[tim::TIM_12]);
}

extern "C" void TIM8_UP_TIM13_IRQHandler(void)
{
    if((TIM8->DIER & TIM_DIER_UIE) && (TIM8->SR & TIM_SR_UIF))
        tim_irq_hndlr(obj_list[tim::TIM_8]);
    else
        tim_irq_hndlr(obj_list[tim::TIM_13]);
}

extern "C" void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
    uint32_t sr = TIM8->SR;
    
    if(((TIM8->DIER & TIM_DIER_COMIE) && (sr & TIM_SR_COMIF)) ||
        ((TIM8->DIER & TIM_DIER_TIE) && (sr & TIM_SR_TIF)))
    {
        tim_irq_hndlr(obj_list[tim::TIM_8]);
    }
    else
        tim_irq_hndlr(obj_list[tim::TIM_14]);
}
#endif
