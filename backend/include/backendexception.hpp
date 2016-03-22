/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef BACKENDEXCEPTION_HPP
#define BACKENDEXCEPTION_HPP

#include <utils.hpp>

namespace td
{

/**
  * A BackendException is thrown whenever an error
  * occured in parsing or executing a backend command
  * @param what: Contains details about the exception
 **/
struct BackendException
{

	/**
	  * Constructs a BackendException using an arbitrary
	  * amount of arguments which are put together
	 **/
	template <class ...T>
	BackendException(T ...t) :
		what(td::utils::concat(t...))
	{}

	/**
	  * The details message of the BackendException
	 **/
	std::string what;

}; //end class BackendException

} //end namespace td

#endif //BACKENDEXCEPTION_HPP
