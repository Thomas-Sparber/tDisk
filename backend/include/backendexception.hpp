/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef BACKENDEXCEPTION_HPP
#define BACKENDEXCEPTION_HPP

#include <tdiskexception.hpp>

namespace td
{

/**
  * A BackendException is thrown whenever an error
  * occured in parsing or executing a backend command
 **/
struct BackendException : public tDiskException
{

	/**
	  * Constructs a BackendException using an arbitrary
	  * amount of arguments which are put together
	 **/
	template <class ...T>
	BackendException(const T& ...t) :
		tDiskException(t...)
	{}

}; //end class BackendException

} //end namespace td

#endif //BACKENDEXCEPTION_HPP
