#ifndef TDISKEXCEPTION_HPP
#define TDISKEXCEPTION_HPP

#include <utils.hpp>

namespace td
{

/**
  * Is thrown whenever a tDisk related error happened
 **/
struct tDiskException : public std::exception
{

	/**
	  * Constructs a tDiskException using any number of arguments
	 **/
	template <class ...T>
	tDiskException(T ...t) :
		msg(utils::concat(t...))
	{}

	/**
	  * Returns the error string of the exception
	 **/
	virtual const char* what() const throw()
	{
		return msg.c_str();
	}
	
	/**
	  * The reason for the error
	 **/
	std::string msg;
	
}; //end class tDiskException

} //end namespace td

#endif //TDISKEXCEPTION_HPP
