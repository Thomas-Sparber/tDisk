#ifndef SHELLRESULT_HPP
#define SHELLRESULT_HPP

#include <memory>
#include <vector>

#include <shellobject.hpp>

namespace td
{

namespace shell
{

class ShellResult
{

public:
	ShellResult() :
		results()
	{}

	ShellResult(const ShellResult &other) :
		results()
	{
		for(const std::unique_ptr<ShellObjectBase> &o : other.results)
		{
			results.emplace_back(o->clone());
		}
	}

	ShellResult(ShellResult &&other) = default;

	ShellResult& operator=(const ShellResult &other)
	{
		results.clear();

		for(const std::unique_ptr<ShellObjectBase> &o : other.results)
		{
			results.emplace_back(o->clone());
		}

		return (*this);
	}

	ShellResult& operator=(ShellResult &&other) = default;

	void addResult(std::unique_ptr<ShellObjectBase> &&result)
	{
		results.emplace_back(std::move(result));
	}

	void addResult(const ShellObjectBase &result)
	{
		results.emplace_back(result.clone());
	}

	bool empty() const
	{
		return results.empty();
	}

	std::size_t size() const
	{
		return results.size();
	}

	std::vector<std::unique_ptr<ShellObjectBase> >::iterator begin()
	{
		return results.begin();
	}
	
	std::vector<std::unique_ptr<ShellObjectBase> >::iterator end()
	{
		return results.end();
	}

	std::vector<std::unique_ptr<ShellObjectBase> >::const_iterator cbegin()
	{
		return results.cbegin();
	}
	
	std::vector<std::unique_ptr<ShellObjectBase> >::const_iterator cend()
	{
		return results.cend();
	}

	template <const char *name, class T = std::string>
	bool get(T &out) const
	{
		return results[0]->get<name,T>(out);
	}

	/**
	  * Retrieves the parameter with the given name
	 **/
	template <const char *name, class T = std::string>
	T get() const
	{
		T t;
		get<name,T>(t);
		return std::move(t);
	}

	const ShellObjectBase& operator[] (std::size_t i) const
	{
		return *results[i].get();
	}
	
	const std::string& getMessage() const
	{
		return results[0]->getMessage();
	}

private:
	std::vector<std::unique_ptr<ShellObjectBase> > results;

}; //end class ShellResult

} //end namespace td

} //end namespace td

#endif //SHELLRESULT_HPP