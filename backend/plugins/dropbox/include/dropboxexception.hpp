#ifndef DROPBOXEXCEPTION_HPP
#define DROPBOXEXCEPTION_HPP

#include <string>
#include <utils.hpp>

struct DropboxException
{

	template <class ...T>
	DropboxException(T ...t) :
		what(concat(t...))
	{}

	std::string what;

}; //end struct DropboxException

#endif //DROPBOXEXCEPTION_HPP
