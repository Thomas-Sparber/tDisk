#ifndef FRONTENDEXCEPTION_HPP
#define FRONTENDEXCEPTION_HPP

#include <utils.hpp>

namespace td
{

struct FrontendException
{
	template <class ...T>
	FrontendException(T ...t) :
		what(td::concat(t...))
	{}

	std::string what;
}; //end class FrontendException

} //end namespace td

#endif //FRONTENDEXCEPTION_HPP
