/*
    Name: USART.cpp

    Copyright(c) 2019 Mateusz Semegen
    This code is licensed under MIT license (see LICENSE file for details)
*/

#ifdef STM32L011xx

//this
#include <hal/stm32l011xx/USART.hpp>

//cml
#include <debug/assert.hpp>
#include <hal/core/systick.hpp>
#include <utils/wait.hpp>

namespace {

using namespace cml::common;
using namespace cml::hal::core;
using namespace cml::hal::stm32l011xx;

USART* p_usart_2 = nullptr;

bool is_USART_ISR_error()
{
    return false;
}

USART::Bus_status get_bus_status_from_USART_ISR()
{
    return USART::Bus_status::ok;
}

void clear_USART_ISR_errors()
{

}

} // namespace ::

extern "C"
{

void USART2_IRQHandler()
{
    assert(nullptr != p_usart_2);
    usart_handle_interrupt(p_usart_2);
}

} // extern "C"

namespace cml {
namespace hal {
namespace stm32l011xx {

using namespace cml::common;
using namespace cml::hal::core;
using namespace cml::utils;

void usart_handle_interrupt(USART* a_p_this)
{
    assert(nullptr != a_p_this);

    uint32 isr = USART2->ISR;
    uint32 cr1 = USART2->CR1;

    if (true == is_flag(isr, USART_ISR_TXE) && true == is_flag(cr1, USART_CR1_TXEIE))
    {
        const bool procceed = a_p_this->tx_callback.function(reinterpret_cast<volatile uint32*>(&(USART2->TDR)),
                                                             a_p_this->tx_callback.p_user_data);

        if (false == procceed)
        {
            a_p_this->stop_transmit_bytes_it();
        }
    }

    if (true == is_flag(isr, USART_ISR_RXNE) && true == is_flag(cr1, USART_CR1_RXNEIE))
    {
        const bool procceed = a_p_this->rx_callback.function(USART2->RDR,
                                                             a_p_this->rx_callback.p_user_data);

        if (false == procceed)
        {
            a_p_this->stop_receive_bytes_it();
        }
    }
}

bool USART::enable(const Config& a_config, const Clock &a_clock, uint32 a_irq_priority, time::tick a_timeout_ms)
{
    assert(nullptr               == p_usart_2);
    assert(0                     != a_config.baud_rate);
    assert(Flow_control::unknown != a_config.flow_control);
    assert(Parity::unknown       != a_config.parity);
    assert(Stop_bits::unknown    != a_config.stop_bits);
    assert(Word_length::unknown  != a_config.word_length);

    assert(Clock::Source::unknown != a_clock.source);
    assert(0                      != a_clock.frequency_hz);

    time::tick start = systick::get_counter();

    p_usart_2 = this;

    constexpr uint32 clock_source_lut[] = { 0, RCC_CCIPR_USART2SEL_0, RCC_CCIPR_USART2SEL_1 };
    set_flag(&(RCC->CCIPR), RCC_CCIPR_USART2SEL, clock_source_lut[static_cast<uint32>(a_clock.source)]);
    set_flag(&(RCC->APB1ENR), RCC_APB1ENR_USART2EN);

    NVIC_SetPriority(USART2_IRQn, a_irq_priority);
    NVIC_EnableIRQ(USART2_IRQn);

    USART2->CR2 = static_cast<uint32>(a_config.stop_bits);
    USART2->CR3 = static_cast<uint32>(a_config.flow_control);

    switch (a_config.oversampling)
    {
        case Oversampling::_16:
        {
            USART2->BRR = a_clock.frequency_hz / a_config.baud_rate;
        }
        break;

        case Oversampling::_8:
        {
            uint32 usartdiv = 2 * a_clock.frequency_hz / a_config.baud_rate;
            USART2->BRR = ((usartdiv & 0xFFF0u) | ((usartdiv & 0xFu) >> 1)) & 0xFFFF;
        }
        break;

        case Oversampling::unknown:
        {
            assert(a_config.oversampling != Oversampling::unknown);
        }
        break;
    }

    set_flag(&(USART2->CR1), static_cast<uint32>(a_config.word_length)  |
                             static_cast<uint32>(a_config.oversampling) |
                             static_cast<uint32>(a_config.parity));

    set_flag(&(USART2->CR1), USART_CR1_UE);
    set_flag(&(USART2->CR1), USART_CR1_RE | USART_CR1_TE);

    this->baud_rate = a_config.baud_rate;
    this->clock     = a_clock;

    bool ret = wait::until(&(USART2->ISR), USART_ISR_REACK | USART_ISR_TEACK, false, start, a_timeout_ms);

    if (false == ret)
    {
        this->disable();
    }

    return ret;
}

void USART::disable()
{
    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    clear_flag(&(RCC->APB1ENR), RCC_APB1ENR_USART2EN);
    NVIC_DisableIRQ(USART2_IRQn);

    p_usart_2 = nullptr;
}

uint32 USART::transmit_bytes_polling(const void* a_p_data, uint32 a_data_size_in_bytes, Bus_status* a_p_status)
{
    assert(nullptr != p_usart_2);
    assert(nullptr != a_p_data);
    assert(a_data_size_in_bytes > 0);

    set_flag(&(USART2->ICR), USART_ICR_TCCF);

    uint32 ret                  = 0;
    bool last_transfer_complete = true;

    while ((ret < a_data_size_in_bytes || false == last_transfer_complete) &&
           false == is_USART_ISR_error())
    {
        if (true == is_flag(USART2->ISR, USART_ISR_TXE) && true == last_transfer_complete)
        {
            USART2->TDR = static_cast<const uint8*>(a_p_data)[ret++];
            last_transfer_complete = false;
        }

        if (false == last_transfer_complete && is_flag(USART2->ISR, USART_ISR_TC))
        {
            set_flag(&(USART2->ICR), USART_ICR_TCCF);
            last_transfer_complete = true;
        }
    }

    if (nullptr != a_p_status)
    {
        (*a_p_status) = get_bus_status_from_USART_ISR();
    }

    if (true == is_USART_ISR_error())
    {
        clear_USART_ISR_errors();
    }

    return ret;
}

uint32 USART::transmit_bytes_polling(const void* a_p_data,
                                     uint32 a_data_size_in_bytes,
                                     time::tick a_timeout_ms,
                                     Bus_status* a_p_status)
{
    assert(nullptr != p_usart_2);
    assert(nullptr != a_p_data);
    assert(a_data_size_in_bytes > 0);
    assert(true == systick::is_enabled());

    time::tick start = systick::get_counter();

    set_flag(&(USART2->ICR), USART_ICR_TCCF);

    uint32 ret = 0;
    bool last_transfer_complete = true;

    while ((ret < a_data_size_in_bytes || false == last_transfer_complete) &&
           false == is_USART_ISR_error() &&
           a_timeout_ms < time::diff(systick::get_counter(), start))
    {
        if (true == is_flag(USART2->ISR, USART_ISR_TXE) && true == last_transfer_complete)
        {
            USART2->TDR = static_cast<const uint8*>(a_p_data)[ret++];
            last_transfer_complete = false;
        }

        if (false == last_transfer_complete && is_flag(USART2->ISR, USART_ISR_TC))
        {
            set_flag(&(USART2->ICR), USART_ICR_TCCF);
            last_transfer_complete = true;
        }
    }

    if (nullptr != a_p_status)
    {
        (*a_p_status) = get_bus_status_from_USART_ISR();
    }

    if (true == is_USART_ISR_error())
    {
        clear_USART_ISR_errors();
    }

    return ret;
}

uint32 USART::receive_bytes_polling(void* a_p_data, uint32 a_data_size_in_bytes, Bus_status* a_p_status)
{
    assert(nullptr != p_usart_2);
    assert(nullptr != a_p_data);
    assert(a_data_size_in_bytes > 0);

    set_flag(&(USART2->ICR), USART_ICR_IDLECF);

    uint32 ret = 0;

    while (false == is_flag(USART2->ISR, USART_ISR_IDLE) &&
           false == is_USART_ISR_error())
    {
        if (true == is_flag(USART2->ISR, USART_ISR_RXNE))
        {
            if (ret < a_data_size_in_bytes)
            {
                static_cast<uint8*>(a_p_data)[ret++] = USART2->RDR;
            }
            else
            {
                set_flag(&(USART2->RQR), USART_RQR_RXFRQ);
                ret++;
            }
        }
    }

    if (nullptr != a_p_status)
    {
        (*a_p_status) = get_bus_status_from_USART_ISR();
    }

    if (true == is_USART_ISR_error())
    {
        clear_USART_ISR_errors();
    }

    return ret;
}

uint32 USART::receive_bytes_polling(void* a_p_data,
                                    uint32 a_data_size_in_bytes,
                                    time::tick a_timeout_ms,
                                    Bus_status* a_p_status)
{
    assert(nullptr != p_usart_2);
    assert(nullptr != a_p_data);
    assert(a_data_size_in_bytes > 0);
    assert(true == systick::is_enabled());

    time::tick start = systick::get_counter();

    set_flag(&(USART2->ICR), USART_ICR_IDLECF);

    uint32 ret = 0;

    while (false == is_flag(USART2->ISR, USART_ISR_IDLE) &&
           false == is_USART_ISR_error() && 
           a_timeout_ms < time::diff(systick::get_counter(), start))
    {
        if (true == is_flag(USART2->ISR, USART_ISR_RXNE))
        {
            if (ret < a_data_size_in_bytes)
            {
                static_cast<uint8*>(a_p_data)[ret++] = USART2->RDR;
            }
            else
            {
                set_flag(&(USART2->RQR), USART_RQR_RXFRQ);
                ret++;
            }
        }
    }

    if (nullptr != a_p_status)
    {
        (*a_p_status) = get_bus_status_from_USART_ISR();
    }

    if (true == is_USART_ISR_error())
    {
        clear_USART_ISR_errors();
    }

    return ret;
}

void USART::start_transmit_bytes_it(const TX_callback& a_callback)
{
    assert(nullptr != p_usart_2);
    assert(nullptr != a_callback.function);

    this->tx_callback = a_callback;

    set_flag(&(USART2->CR1), USART_CR1_TXEIE);
}

void USART::start_receive_bytes_it(const RX_callback& a_callback)
{
    assert(nullptr != p_usart_2);
    assert(nullptr != a_callback.function);

    this->rx_callback = a_callback;

    set_flag(&(USART2->CR1), USART_CR1_RXNEIE);
}

void USART::stop_transmit_bytes_it()
{
    assert(nullptr != p_usart_2);

    clear_flag(&(USART2->CR1), USART_CR1_TXEIE);

    this->tx_callback = { nullptr, nullptr };
}

void USART::stop_receive_bytes_it()
{
    assert(nullptr != p_usart_2);

    clear_flag(&(USART2->CR1), USART_CR1_RXNEIE);

    this->rx_callback  = { nullptr, nullptr };
}

void USART::set_baud_rate(uint32 a_baud_rate)
{
    assert(nullptr != p_usart_2);
    assert(0 != a_baud_rate);

    const Oversampling oversampling = this->get_oversampling();

    switch (oversampling)
    {
        case Oversampling::_8:
        {
            uint32 usartdiv = 2 * this->clock.frequency_hz / a_baud_rate;
            USART2->BRR = ((usartdiv & 0xFFF0u) | ((usartdiv & 0xFu) >> 1)) & 0xFFFF;
        }
        break;

        case Oversampling::_16:
        {
            USART2->BRR = this->clock.frequency_hz / a_baud_rate;
        }
        break;

        case Oversampling::unknown:
        {
            assert(Oversampling::unknown != oversampling);
        }
        break;
    }
}

void USART::set_oversampling(Oversampling a_oversampling)
{
    assert(nullptr != p_usart_2);
    assert(Oversampling::unknown != a_oversampling);

    clear_flag(&(USART2->CR1), USART_CR1_UE);
    set_flag(&(USART2->CR1), static_cast<uint32>(a_oversampling));
    set_flag(&(USART2->CR1), USART_CR1_UE);
}

void USART::set_word_length(Word_length a_word_length)
{
    assert(nullptr != p_usart_2);
    assert(Word_length::unknown != a_word_length);

    clear_flag(&(USART2->CR1), USART_CR1_UE);
    set_flag(&(USART2->CR1), static_cast<uint32>(a_word_length) | USART_CR1_UE);
}

void USART::set_parity(Parity a_parity)
{
    assert(nullptr != p_usart_2);
    assert(Parity::unknown != a_parity);

    clear_flag(&(USART2->CR1), USART_CR1_UE);
    set_flag(&(USART2->CR1), static_cast<uint32>(a_parity) | USART_CR1_UE);
}

void USART::set_stop_bits(Stop_bits a_stop_bits)
{
    assert(nullptr != p_usart_2);
    assert(Stop_bits::unknown != a_stop_bits);

    clear_flag(&(USART2->CR1), USART_CR1_UE);
    set_flag(&(USART2->CR2), static_cast<uint32>(a_stop_bits));
    set_flag(&(USART2->CR1), USART_CR1_UE);
}

void USART::set_flow_control(Flow_control a_flow_control)
{
    assert(nullptr != p_usart_2);
    assert(Flow_control::unknown != a_flow_control);

    clear_flag(&(USART2->CR1), USART_CR1_UE);
    set_flag(&(USART2->CR3), static_cast<uint32>(a_flow_control));
    set_flag(&(USART2->CR1), USART_CR1_UE);
}

USART::Oversampling USART::get_oversampling() const
{
    assert(nullptr != p_usart_2);

    return static_cast<Oversampling>(get_flag(USART2->CR1, static_cast<uint32>(USART_CR1_OVER8)));
}

USART::Word_length USART::get_word_length() const
{
    assert(nullptr != p_usart_2);

    return static_cast<Word_length>(get_flag(USART2->CR1, USART_CR1_M0 | USART_CR1_M1));
}

USART::Stop_bits USART::get_stop_bits() const
{
    assert(nullptr != p_usart_2);

    return static_cast<Stop_bits>(get_flag(USART2->CR2, USART_CR2_STOP));
}

USART::Flow_control USART::get_flow_control() const
{
    assert(nullptr != p_usart_2);

    return static_cast<Flow_control>(get_flag(USART2->CR3, USART_CR3_RTSE | USART_CR3_CTSE));
}

} // naespace stm32l011xx
} // namespace hal
} // namespace cml`

#endif // STM32L011xx