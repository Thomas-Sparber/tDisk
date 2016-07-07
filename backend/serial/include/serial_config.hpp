/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef SERIAL_CONFIG_HPP
#define SERIAL_CONFIG_HPP

#include <serialport.hpp>

namespace td
{

	/**
	  * The default baud rate which is used for serial port communication
	 **/
	constexpr Serialport::Baud default_baud = Serialport::Baud_2400;

	/**
	  * The default parity which is used for serial port communication
	 **/
	constexpr Serialport::Parity default_parity = Serialport::EvenParity;

} //end namespace td

#endif //SERIAL_CONFIG_HPP
