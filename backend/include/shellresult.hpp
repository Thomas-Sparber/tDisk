#ifndef SHELLRESULT_HPP
#define SHELLRESULT_HPP

#include <memory>
#include <vector>

#include <shellobject.hpp>

namespace td
{

namespace shell
{

/**
  * A ShellResult represents the result of a shell command
  * which can consist of one or multiple shell objects and the
  * shell command message
 **/
class ShellResult
{

public:
	/**
	  * Default constructor
	 **/
	ShellResult() :
		results()
	{}

	/**
	  * Default copy constructor
	 **/
	ShellResult(const ShellResult &other) :
		results()
	{
		for(const std::unique_ptr<ShellObjectBase> &o : other.results)
		{
			//Clone each object
			results.emplace_back(o->clone());
		}
	}

	/**
	  * Default move constructor
	 **/
	ShellResult(ShellResult &&other) = default;

	/**
	  * Default assignment operator
	 **/
	ShellResult& operator=(const ShellResult &other)
	{
		results.clear();

		for(const std::unique_ptr<ShellObjectBase> &o : other.results)
		{
			//Clone each object
			results.emplace_back(o->clone());
		}

		return (*this);
	}

	/**
	  * Default move operator
	 **/
	ShellResult& operator=(ShellResult &&other) = default;

	/**
	  * Adds a result object to the result
	 **/
	void addResult(std::unique_ptr<ShellObjectBase> &&result)
	{
		results.emplace_back(std::move(result));
	}

	/**
	  * Adds a result object to the result.
	  * The result object is cloned
	 **/
	void addResult(const ShellObjectBase &result)
	{
		results.emplace_back(result.clone());
	}

	/**
	  * Checks whether the result contains any objects
	 **/
	bool empty() const
	{
		return results.empty();
	}

	/**
	  * Returns the amount of result objects
	  * within the result
	 **/
	std::size_t size() const
	{
		return results.size();
	}

	/**
	  * Iterator for iterating over the list of
	  * result objects
	 **/
	std::vector<std::unique_ptr<ShellObjectBase> >::iterator begin()
	{
		return results.begin();
	}

	/**
	  * Iterator for iterating over the list of
	  * result objects
	 **/
	std::vector<std::unique_ptr<ShellObjectBase> >::iterator end()
	{
		return results.end();
	}

	/**
	  * Iterator for iterating over the list of
	  * result objects
	 **/
	std::vector<std::unique_ptr<ShellObjectBase> >::const_iterator cbegin()
	{
		return results.cbegin();
	}

	/**
	  * Iterator for iterating over the list of
	  * result objects
	 **/
	std::vector<std::unique_ptr<ShellObjectBase> >::const_iterator cend()
	{
		return results.cend();
	}

	/**
	  * Gets the parameter with the given name and type
	  * from the first result object
	 **/
	template <const char *name, class T = std::string>
	bool get(T &out) const
	{
		return results[0]->get<name,T>(out);
	}

	/**
	  * Gets the parameter with the given name and type
	  * from the first result object
	 **/
	template <const char *name, class T = std::string>
	T get() const
	{
		T t;
		get<name,T>(t);
		return std::move(t);
	}

	/**
	  * Accesses the result object of the given index
	 **/
	const ShellObjectBase& operator[] (std::size_t i) const
	{
		return *results[i].get();
	}

	/**
	  * Returns the command message
	 **/
	const std::string& getMessage() const
	{
		return results[0]->getMessage();
	}

private:

	/**
	  * The internal container where the result objects are stored
	 **/
	std::vector<std::unique_ptr<ShellObjectBase> > results;

}; //end class ShellResult

} //end namespace td

} //end namespace td

#endif //SHELLRESULT_HPP
