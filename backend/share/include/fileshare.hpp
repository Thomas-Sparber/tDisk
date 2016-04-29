/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef FILESHARE_HPP
#define FILESHARE_HPP

#include <string>
#include <vector>

#include <resultformatter.hpp>

namespace td
{

/**
  * This class represents a samba fileshare.
  * It is possible to manage all shares using it,
  * e.g. list, create, remove, modify
 **/
class Fileshare
{

public:

	/**
	  * Fills the given container with all available
	  * fileshares on the system
	 **/
	template <template<class ...T> class cont>
	static void getAllFileshares(cont<std::string> &out)
	{
		std::vector<std::string> temp;
		getAllFileshares(temp);

		for(std::string &s : temp)
			out.push_back(std::move(s));
	}

	/**
	  * Fills the given container with all available
	  * fileshares on the system
	 **/
	static void getAllFileshares(std::vector<std::string> &out);

	/**
	  * Fills the given container with all available
	  * fileshares on the system
	 **/
	template <template<class ...T> class cont>
	static void getAllFileshares(cont<Fileshare> &out)
	{
		std::vector<std::string> shares;
		getAllFileshares(shares);

		for(const std::string &s : shares)
		{
			Fileshare temp = Fileshare::load(s);
			out.push_back(std::move(temp));
		}
	}

	/**
	  * Loads the fileshare with the given name
	 **/
	static Fileshare load(const std::string &name);

	/**
	  * Creates the fileshare with the given name,
	  * path and optional description
	 **/
	static Fileshare create(const std::string &name, const std::string &path, const std::string &description="");

	/**
	  * Removes the fileshare with the given name
	 **/
	static void remove(const std::string &name);

public:

	/**
	  * Default constructor
	 **/
	Fileshare() :
		name(),
		path(),
		description()
	{}

	/**
	  * Constructor to create a fileshare usin
	  * its name, path and description
	 **/
	Fileshare(const std::string &str_name, const std::string &str_path, const std::string &str_description) :
		name(str_name),
		path(str_path),
		description(str_description)
	{}

	/**
	  * Returns the name of the fileshare
	 **/
	const std::string& getName() const
	{
		return name;
	}

	/**
	  * Returns the path of the fileshare
	 **/
	const std::string& getPath() const
	{
		return path;
	}

	/**
	  * Returns the description of the fileshare
	 **/
	const std::string& getDescription() const
	{
		return description;
	}

	/**
	  * Removes the fileshare from the system
	 **/
	void remove()
	{
		Fileshare::remove(name);
	}

	friend std::string createResultString<Fileshare>(const Fileshare &share, unsigned int hierarchy, const utils::ci_string &outputFormat);

private:

	/**
	  * The name of the fileshare
	 **/
	std::string name;

	/**
	  * The path of the fileshare
	 **/
	std::string path;

	/**
	  * The description of the fileshare
	 **/
	std::string description;

}; //end class Fileshare

/**
  * Stringifies the given Fileshare using the given format
 **/
template <> inline std::string createResultString(const Fileshare &share, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(share, name, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(share, path, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(share, description, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(share, name, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(share, path, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(share, description, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //FILESHARE_HPP
