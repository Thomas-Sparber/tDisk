/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef CONFIGEXCEPTION_HPP
#define CONFIGEXCEPTION_HPP

#include <tdiskexception.hpp>

namespace td
{

/**
  * A ConfigException is thrown whenever an error
  * occured when dealing with config files
 **/
struct ConfigException : public tDiskException
{

	/**
	  * Constructs a BackendException using an arbitrary
	  * amount of arguments which are put together
	 **/
	template <class ...T>
	ConfigException(T ...t) :
		tDiskException(t...)
	{}

}; //end class ConfigException

} //end namespace td

#endif //CONFIGEXCEPTION_HPP
