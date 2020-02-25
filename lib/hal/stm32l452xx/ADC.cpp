/*
    Name: ADC.cpp

    Copyright(c) 2020 Mateusz Semegen
    This code is licensed under MIT license (see LICENSE file for details)
*/

#ifdef STM32L452xx

//this
#include <hal/stm32l452xx/ADC.hpp>

//cml
#include <common/bit.hpp>
#include <hal/systick.hpp>
#include <utils/sleep.hpp>

#ifdef CML_DEBUG
#include <debug/assert.hpp>
#include <hal/stm32l452xx/mcu.hpp>
#endif // CML_DEBUG

namespace cml {
namespace hal {
namespace stm32l452xx {

using namespace cml::common;
using namespace cml::utils;

bool ADC::enable(Resolution a_resolution, const Clock& a_clock, time_tick a_timeout)
{
    assert(true == systick::is_enabled());

    time_tick start = systick::get_counter();

    this->disable();

    set_flag(&(RCC->AHB2ENR), RCC_AHB2ENR_ADCEN);

    switch (a_clock.source)
    {
        case Clock::Source::asynchronous:
        {
            assert(mcu::Pll_config::Source::unknown != mcu::get_pll_config().source &&
                   true == mcu::get_pll_config().pllsai1.r.output_enabled);

            clear_flag(&(ADC1_COMMON->CCR), ADC_CCR_CKMODE);
            set_flag(&(ADC1_COMMON->CCR), ADC_CCR_PRESC, static_cast<uint32>(a_clock.asynchronous.source));
        }
        break;

        case Clock::Source::synchronous:
        {
            assert(Clock::Synchronous::Divider::_1 == a_clock.synchronous.divider ?
                   mcu::Bus_prescalers::AHB::_1 == mcu::get_bus_prescalers().ahb :
                   true);

            set_flag(&(ADC1_COMMON->CCR), static_cast<uint32>(a_clock.synchronous.divider));
        }
        break;

        case Clock::Source::unknown:
        {
            assert(a_clock.source != Clock::Source::unknown);
        }
    }

    set_flag(&(ADC1->CR), ADC_CR_ADCALDIF);
    sleep::us(21u);

    clear_flag(&(ADC1->CR), ADC_CR_ADCALDIF);
    set_flag(&(ADC1->CR), ADC_CR_ADCAL);

    bool ret = sleep::wait_until(ADC1->CR,
                                 ADC_CR_ADCAL,
                                 true,
                                 systick::get_counter(),
                                 a_timeout - (systick::get_counter() - start));

    if (true == ret)
    {
        set_flag(&(ADC1->CFGR), static_cast<uint32_t>(a_resolution));
        set_flag(&(ADC1->CR), ADC_CR_ADEN);

        ret = sleep::wait_until(ADC1->ISR,
                                ADC_ISR_ADRDY,
                                false,
                                systick::get_counter(),
                                a_timeout - (systick::get_counter() - start));
    }

    if (false == ret)
    {
        this->disable();
    }

    return ret;
}

void ADC::disable()
{
    ADC1->CR         = 0;
    ADC1_COMMON->CCR = 0;

    clear_flag(&(RCC->AHB2ENR), RCC_AHB2ENR_ADCEN);
}

void ADC::set_channels(const Channel* a_p_channels, uint32 a_channels_count)
{
    assert(nullptr != a_p_channels);
    assert(a_channels_count > 0);

    this->clear_channels();

    ADC1->SQR1 = a_channels_count - 1;

    for (uint32 i = 0; i < a_channels_count && i < 4; i++)
    {
        assert(Channel::Id::unknown != a_p_channels[i].id);
        set_flag(&(ADC1->SQR1), static_cast<uint32_t>(a_p_channels[i].id) << 6 * (i + 1));
    }

    for (uint32 i = 4; i < a_channels_count && i < 9; i++)
    {
        assert(Channel::Id::unknown != a_p_channels[i].id);
        set_flag(&(ADC1->SQR2), static_cast<uint32_t>(a_p_channels[i].id) << 6 * (i + 1));
    }

    for (uint32 i = 9; i < a_channels_count && i < 14; i++)
    {
        assert(Channel::Id::unknown != a_p_channels[i].id);
        set_flag(&(ADC1->SQR3), static_cast<uint32_t>(a_p_channels[i].id) << 6 * (i + 1));
    }

    for (uint32 i = 14; i < a_channels_count && i < 16; i++)
    {
        assert(Channel::Id::unknown != a_p_channels[i].id);
        set_flag(&(ADC1->SQR4), static_cast<uint32_t>(a_p_channels[i].id) << 6 * (i + 1));
    }

    for (uint32 i = 0; i < a_channels_count; i++)
    {
        const uint32 channel_sampling_time = static_cast<uint32_t>(a_p_channels[i].sampling_time) << (static_cast<uint32_t>(a_p_channels[i].id) * 3);

        if (static_cast<uint32_t>(a_p_channels[i].id) <= 9)
        {
            set_flag(&(ADC1->SMPR1), channel_sampling_time);
        }
        else
        {
            set_flag(&(ADC1->SMPR2), channel_sampling_time);
        }
    }
}

void ADC::clear_channels()
{
    ADC1->SQR1 = 0;

    ADC1->SQR2 = 0;
    ADC1->SQR3 = 0;
    ADC1->SQR4 = 0;

    ADC1->SMPR1 = 0;
    ADC1->SMPR2 = 0;
}

void ADC::read_polling(uint16* a_p_data, uint32 a_count)
{
    assert(nullptr != a_p_data);
    assert(a_count > 0);
    assert((ADC1->SQR1 & 0xFu) >= a_count);

    set_flag(&(ADC1->CR), ADC_CR_ADSTART);

    for (uint32 i = 0; i < a_count; i++)
    {
        sleep::wait_until(ADC1->ISR, ADC_ISR_EOC, false);
        a_p_data[i] = static_cast<uint16_t>(ADC1->DR);
    }

    sleep::wait_until(ADC1->ISR, ADC_ISR_EOS, false);
    set_flag(&(ADC1->ISR), ADC_ISR_EOS);

    set_flag(&(ADC1->CR), ADC_CR_ADSTP);
    clear_flag(&(ADC1->CR), ADC_CR_ADSTART);
}

} // namespace stm32l452xx
} // namespace hal
} // namespace cml

#endif