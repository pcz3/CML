#pragma once

/*
    Name: I2C.hpp

    Copyright(c) 2019 Mateusz Semegen
    This code is licensed under MIT license (see LICENSE file for details)
*/

//externals
#include <stm32l4xx.h>

//cml
#include <common/bit.hpp>
#include <common/integer.hpp>
#include <common/Non_copyable.hpp>
#include <common/time_tick.hpp>
#include <debug/assert.hpp>

namespace cml {
namespace hal {
namespace stm32l452xx {

class I2C_master : private common::Non_copyable
{
public:

    enum class Id : common::uint32
    {
        _1,
        _2,
        _3,
        _4
    };

    enum class Clock_source
    {
        pclk1,
        sysclk,
        hsi
    };

public:

    I2C_master(Id a_id)
        : id(a_id)
    {}

    ~I2C_master()
    {
        this->diasble();
    }

    void enable(Clock_source a_clock_source, bool a_analog_filter, common::uint32 a_timings, bool a_is_fast_plus);
    void diasble();

    template<typename Data_t>
    void write_polling(const Data_t& a_data)
    {
        this->write_bytes_polling(static_cast<void*>(a_data), sizeof(a_data));
    }

    template<typename Data_t>
    bool write_polling(const Data_t& a_data, common::time_tick a_timeout_ms)
    {
        return this->write_bytes_polling(static_cast<void*>(a_data), sizeof(a_data), a_timeout_ms);
    }

    template<typename Data_t>
    void read_polling(Data_t* a_p_data)
    {
        this->read_bytes_polling(static_cast<void*>(a_p_data), sizeof(Data_t));
    }

    template<typename Data_t>
    bool read_polling(Data_t* a_p_data, common::time_tick a_timeout_ms)
    {
        return this->write_bytes_polling(static_cast<void*>(&a_p_data), sizeof(Data_t), a_timeout_ms);
    }

    void write_bytes_polling(const void* a_p_data, common::uint32 a_data_size_in_bytes);
    bool write_bytes_polling(const void* a_p_data, common::uint32 a_data_size_in_bytes, common::time_tick a_timeout_ms);

    void read_bytes_polling(void* a_p_data, common::uint32 a_data_size_in_bytes);
    bool read_bytes_polling(void* a_p_data, common::uint32 a_data_size_in_bytes, common::time_tick a_timeout_ms);

    bool is_analog_filter() const
    {
        assert(nullptr != this->p_i2c);

        return false == common::is_flag(p_i2c->CR1, I2C_CR1_ANFOFF);
    }

    common::uint32 get_timing() const
    {
        assert(nullptr != this->p_i2c);

        return this->p_i2c->TIMINGR;
    }

    Clock_source get_clock_source() const
    {
        return Clock_source::hsi;
    }

    bool is_fast_plus() const
    {
        return common::get_bit(SYSCFG->CFGR1, SYSCFG_CFGR1_I2C1_FMP_Pos + static_cast<common::uint32>(this->id));
    }

    Id get_id() const
    {
        return this->id;
    }

private:

    Id id;
    I2C_TypeDef* p_i2c;
};

class I2C_slave : private common::Non_copyable
{
public:

    enum class Id : common::uint32
    {
        _1,
        _2,
        _3,
        _4
    };

public:

    I2C_slave(Id a_id)
        : id(a_id)
    {}

    ~I2C_slave()
    {
        this->diasble();
    }

    void enable(bool a_analog_filter, common::uint32 a_timings, bool a_is_fast_plus);
    void diasble();

    template<typename Data_t>
    void write_polling(const Data_t& a_data)
    {
        this->write_bytes_polling(static_cast<void*>(a_data), sizeof(a_data));
    }

    template<typename Data_t>
    bool write_polling(const Data_t& a_data, common::time_tick a_timeout_ms)
    {
        return this->write_bytes_polling(static_cast<void*>(a_data), sizeof(a_data), a_timeout_ms);
    }

    template<typename Data_t>
    void read_polling(Data_t* a_p_data)
    {
        this->read_bytes_polling(static_cast<void*>(a_p_data), sizeof(Data_t));
    }

    template<typename Data_t>
    bool read_polling(Data_t* a_p_data, common::time_tick a_timeout_ms)
    {
        return this->write_bytes_polling(static_cast<void*>(&a_p_data), sizeof(Data_t), a_timeout_ms);
    }

    void write_bytes_polling(const void* a_p_data, common::uint32 a_data_size_in_bytes);
    bool write_bytes_polling(const void* a_p_data, common::uint32 a_data_size_in_bytes, common::time_tick a_timeout_ms);

    void read_bytes_polling(void* a_p_data, common::uint32 a_data_size_in_bytes);
    bool read_bytes_polling(void* a_p_data, common::uint32 a_data_size_in_bytes, common::time_tick a_timeout_ms);

    bool is_analog_filter() const
    {
        assert(nullptr != this->p_i2c);

        return false == common::is_flag(p_i2c->CR1, I2C_CR1_ANFOFF);
    }

    common::uint32 get_timing() const
    {
        assert(nullptr != this->p_i2c);

        return this->p_i2c->TIMINGR;
    }

    bool is_fast_plus() const
    {
        return common::get_bit(SYSCFG->CFGR1, SYSCFG_CFGR1_I2C1_FMP_Pos + static_cast<common::uint32>(this->id));
    }

    Id get_id() const
    {
        return this->id;
    }

private:

    Id id;
    I2C_TypeDef* p_i2c;
};

} // namespace stm32l452xx
} // namespace hal
} // namespace cml