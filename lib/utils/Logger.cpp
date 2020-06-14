/*
    Name: Logger.cpp

    Copyright(c) 2019 Mateusz Semegen
    This code is licensed under MIT license (see LICENSE file for details)
*/

//this
#include <utils/Logger.hpp>

//cml
#include <common/cstring.hpp>
#include <common/integer.hpp>
#include <debug/assert.hpp>

namespace {

using namespace cml::common;

constexpr const char* tags[] =
{
    "[inf] ",
    "[wrn] ",
    "[err] ",
    "[omg] "
};

constexpr uint32 tag_length = 6;

} // namespace ::

namespace cml {
namespace utils {

using namespace cml::common;

void Logger::inf(const char* a_p_message)
{
    this->write(a_p_message, Stream_type::inf);
}

void Logger::wrn(const char* a_p_message)
{
    this->write(a_p_message, Stream_type::wrn);
}

void Logger::err(const char* a_p_message)
{
    this->write(a_p_message, Stream_type::err);
}

void Logger::omg(const char* a_p_message)
{
    this->write(a_p_message, Stream_type::omg);
}

void Logger::write(const char* a_p_message, Stream_type a_type)
{
    assert(nullptr != this->p_io_stream);

    if (true == this->is_stream_enabled(a_type))
    {
        const uint32 message_length = cstring::length(a_p_message, config::console::line_buffer_capacity);

        this->p_io_stream->transmit_bytes_polling(tags[static_cast<uint32>(a_type)], tag_length);
        this->p_io_stream->transmit_bytes_polling(a_p_message, message_length);
        this->p_io_stream->transmit_bytes_polling(&(config::new_line_character), 1);
    }
}

} // namespace utils
} // namespace cmls