/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISKOFFLINEEXCEPTION_HPP
#define TDISKOFFLINEEXCEPTION_HPP

#include <tdiskexception.hpp>

namespace td
{

/**
  * A tDiskOfflineException is thrown when the tDisk
  * driver is offline
 **/
struct tDiskOfflineException : public tDiskException
{

	/**
	  * Constructs a BackendException using an arbitrary
	  * amount of arguments which are put together
	 **/
	template <class ...T>
	tDiskOfflineException(T ...t) :
		tDiskException(t...)
	{}

}; //end class tDiskOfflineException

} //end namespace td

#endif //TDISKOFFLINEEXCEPTION_HPP
