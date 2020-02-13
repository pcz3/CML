/*
    Name: main.cpp

    Copyright(c) 2019 Mateusz Semegen
    This code is licensed under MIT license (see LICENSE file for details)
*/

//cml
#include <collection/Vector.hpp>
#include <common/cstring.hpp>
#include <hal/GPIO.hpp>
#include <hal/MCU.hpp>
#include <hal/Systick.hpp>
#include <hal/USART.hpp>
#include <utils/Command_line.hpp>

namespace
{

using namespace cml::collection;
using namespace cml::common;
using namespace cml::hal;
using namespace cml::utils;

void led_cli_callback(const Vector<Command_line::Callback::Parameter>& a_params, void* a_p_user_data)
{
    Output_pin* p_led_pin = reinterpret_cast<Output_pin*>(a_p_user_data);

    if (2 == a_params.get_length())
    {
        bool is_on = cstring::equals(a_params[1].a_p_value, "on", a_params[1].length);

        if (true == is_on)
        {
            p_led_pin->set_level(Output_pin::Level::high);
        }
        else
        {
            bool is_off = cstring::equals(a_params[1].a_p_value, "off", a_params[1].length);

            if (true == is_off)
            {
                p_led_pin->set_level(Output_pin::Level::low);
            }
        }
    }
}

} // namespace ::

int main()
{
    using namespace cml::common;
    using namespace cml::hal;
    using namespace cml::utils;

    MCU::get_instance().enable_hsi_clock(MCU::Hsi_frequency::_16_MHz);
    MCU::get_instance().set_sysclk(MCU::Sysclk_source::hsi, { MCU::Bus_prescalers::AHB::_1,
                                                              MCU::Bus_prescalers::APB1::_1,
                                                              MCU::Bus_prescalers::APB2::_1 });

    if (MCU::Sysclk_source::hsi == MCU::get_instance().get_sysclk_source())
    {
        USART::Config usart_config =
        {
            115200u,
            USART::Oversampling::_16,
            USART::Word_length::_8_bits,
            USART::Stop_bits::_1,
            USART::Flow_control::none,
            USART::Parity::none,
        };

        USART::Clock usart_clock
        {
            USART::Clock::Source::sysclk,
            SystemCoreClock
        };

        Alternate_function_pin::Config usart_pin_config =
        {
            Alternate_function_pin::Mode::push_pull,
            Alternate_function_pin::Pull::up,
            Alternate_function_pin::Speed::low,
            0x4u
        };

        MCU::get_instance().disable_msi_clock();
        Systick::get_instance().enable((1u << __NVIC_PRIO_BITS) - 1u);

        GPIO gpio_port_a(GPIO::Id::a);
        gpio_port_a.enable();

        Alternate_function_pin console_usart_TX_pin(&gpio_port_a, 2);
        Alternate_function_pin console_usart_RX_pin(&gpio_port_a, 15);

        console_usart_TX_pin.enable(usart_pin_config);
        console_usart_RX_pin.enable(usart_pin_config);

        USART console_usart(USART::Id::_2);
        bool usart_ready = console_usart.enable(usart_config, usart_clock, 10);

        if (true == usart_ready)
        {
            GPIO gpio_port_b(GPIO::Id::b);
            gpio_port_b.enable();

            Output_pin led_pin(&gpio_port_b, 3);
            led_pin.enable({ Output_pin::Mode::push_pull, Output_pin::Pull::down, Output_pin::Speed::low });

            Command_line command_line(&console_usart, "cmd > ", "Command not found");

            command_line.register_callback({ "led", led_cli_callback, &led_pin });

            command_line.enable();
            command_line.write_prompt();

            while (true)
            {
                command_line.update();
            }
        }
    }

    while (true);
}