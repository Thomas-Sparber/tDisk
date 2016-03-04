#ifndef DROPBOXEXCEPTION_HPP
#define DROPBOXEXCEPTION_HPP

#include <string>
#include <utils.hpp>

namespace td
{

struct DropboxException
{

	template <class ...T>
	DropboxException(T ...t) :
		what(utils::concat(t...))
	{}

	std::string what;

}; //end struct DropboxException

} //end namespace td

#endif //DROPBOXEXCEPTION_HPP
