/*
    Name: CRC32.cpp

    Copyright(c) 2019 Mateusz Semegen
    This code is licensed under MIT license (see LICENSE file for details)
*/

#ifdef STM32L452xx

//this
#include <hal/stm32l452xx/CRC32.hpp>

namespace cml {
namespace hal {
namespace stm32l452xx {

using namespace cml::common;

void CRC32::enable(In_data_reverse a_in_reverse, Out_data_reverse a_out_reverse)
{
    set_flag(&(RCC->AHB1ENR), RCC_AHB1ENR_CRCEN);

    clear_flag(&(this->p_crc->CR), CRC_CR_POLYSIZE);
    set_flag(&(this->p_crc->CR), static_cast<uint32>(a_in_reverse));
    set_flag(&(this->p_crc->CR), static_cast<uint32>(a_out_reverse));
}

void CRC32::disable()
{
    clear_flag(&(RCC->AHB1ENR), RCC_AHB1ENR_CRCEN);
}

} // namespace cml
} // namespace hal
} // namespace stm32l452xx

#endif // STM32L452xx