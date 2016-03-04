#ifndef BACKENDEXCEPTION_HPP
#define BACKENDEXCEPTION_HPP

#include <utils.hpp>

namespace td
{

struct BackendException
{
	template <class ...T>
	BackendException(T ...t) :
		what(td::concat(t...))
	{}

	std::string what;
}; //end class BackendException

} //end namespace td

#endif //BACKENDEXCEPTION_HPP
