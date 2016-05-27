#ifndef FORMATEXCEPTION_HPP
#define FORMATEXCEPTION_HPP

#include <tdiskexception.hpp>

namespace td
{

/**
  * A FormanException is thrown whenever an error occurred while
  * formatting an object
 **/
struct FormatException : public tDiskException
{
	
	/**
	  * Constructs a FormatException using any number of arguments
	  * and concats them
	 **/
	template <class ...T>
	FormatException(const T& ...t) :
		tDiskException(t...)
	{}
	 
}; //end class FormatException

} //end namespace td

#endif //FORMATEXCEPTION_HPP
