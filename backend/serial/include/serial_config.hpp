#ifndef SERIAL_CONFIG_HPP
#define SERIAL_CONFIG_HPP

#include <serialport.hpp>

namespace td
{

	constexpr Serialport::Baud default_baud = Serialport::Baud_9600;

	constexpr Serialport::Parity default_parity = Serialport::NoParity;

} //end namespace td

#endif //SERIAL_CONFIG_HPP
