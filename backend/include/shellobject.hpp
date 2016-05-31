/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef SHELLOBJECT_HPP
#define SHELLOBJECT_HPP

#include <string>
#include <tuple>

#include <backendexception.hpp>
#include <convert.hpp>
#include <shellretvalue.hpp>

namespace td
{

namespace shell
{

	/**
	  * This is the base object Of the return object af a shell command.
	  * It provides functions to retrieve parameters, parse an entire return
	  * string and store/retrieve the error message (entire return string)
	 **/
	class ShellObjectBase
	{

	public:
		/**
		  * Default constructor
		 **/
		ShellObjectBase() :
			result()
		{}

		/**
		  * Default virtual destructor
		 **/
		virtual ~ShellObjectBase() {}

		/**
		  * Retrieves the parameter with the given name and stores it
		  * in "out". The function returns true if the parameter
		  * was received successfully.
		 **/
		template <const char *name, class T = std::string>
		bool get(T &out) const
		{
			try {
				return utils::convertTo(getValue(name), out);
			} catch (const BackendException &e) {
				return false;
			}
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

		/**
		  * Clones the ShellObjectBase
		 **/
		virtual ShellObjectBase* clone() const = 0;

		/**
		  * Parses the given return string
		 **/
		bool parse(const std::string &r)
		{
			this->result = r;

			try {
				parseArguments(result);
				return true;
			} catch(const BackendException &e) {
				return false;
			}
		}

		/**
		  * Returns the entire return string
		  * which - in case of an error - should be the
		  * error message
		 **/
		const std::string& getMessage() const
		{
			return result;
		}

	protected:

		/**
		  * Retrieves the value of the parameter with the given name
		 **/
		virtual const std::string& getValue(const char *name) const = 0;

		/**
		  * Parses the return string and fills the parameters
		 **/
		virtual void parseArguments(const std::string &r) = 0;

	private:

		/**
		  * The entire return string
		 **/
		std::string result;

	}; //end class ShellObjectBase

	/**
	  * This is the specialized version of the ShellObjectBase
	  * which implements all the desired return parameters
	 **/
	template <class ...RetValues>
	class ShellObject : public ShellObjectBase, RetValues...
	{

	public:
		/**
		  * Default constructor
		 **/
		ShellObject() :
			retValues()
		{}

		virtual const std::string& getValue(const char *name) const
		{
			return getRetValue(name, retValues).getValue();
		}

		virtual ShellObjectBase* clone() const
		{
			return new ShellObject<RetValues...>(*this);
		}

		virtual void parseArguments(const std::string &r)
		{
			std::vector<std::string> splitted;
			utils::split(r, '\n', splitted);

			for(std::size_t i = 0; i < splitted.size(); ++i)
			{
				ShellRetValueBase &v = getRetValue((int)(i+1), retValues);
				v.setValue(splitted[i]);
			}
		}

	private:
		/**
		  * Retrieves the parameter with the given name
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), ShellRetValueBase&>::type
		getRetValue(const char *name, std::tuple<Tp...>&)
		{
			throw BackendException("Parameter ",name," does not exist in ShellObject");
		}

		/**
		  * Retrieves the parameter with the given name
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), ShellRetValueBase&>::type
		getRetValue(const char *name, std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getName() == name)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(name, t);
		}

		/**
		  * Retrieves the parameter with the given name
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), const ShellRetValueBase&>::type
		getRetValue(const char *name, const std::tuple<Tp...>&)
		{
			throw BackendException("Parameter ",name," does not exist in ShellObject");
		}

		/**
		  * Retrieves the parameter with the given name
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), const ShellRetValueBase&>::type
		getRetValue(const char *name, const std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getName() == name)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(name, t);
		}



		/**
		  * Retrieves the parameter with the given line number
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), ShellRetValueBase&>::type
		getRetValue(int line, std::tuple<Tp...>&)
		{
			throw BackendException("Parameter for line ",line," does not exist in ShellObject");
		}

		/**
		  * Retrieves the parameter with the given line number
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), ShellRetValueBase&>::type
		getRetValue(int line, std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getLine() == line)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(line, t);
		}

		/**
		  * Retrieves the parameter with the given line number
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), const ShellRetValueBase&>::type
		getRetValue(int line, const std::tuple<Tp...>&)
		{
			throw BackendException("Parameter for line ",line," does not exist in ShellObject");
		}

		/**
		  * Retrieves the parameter with the given line number
		 **/
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), const ShellRetValueBase&>::type
		getRetValue(int line, const std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getLine() == line)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(line, t);
		}

	private:
		/**
		  * Contains all return parameters
		 **/
		std::tuple<RetValues...> retValues;

	}; //end class ShellObject


	constexpr char name[] = "name";
	constexpr char path[] = "path";
	constexpr char size[] = "size";
	constexpr char number[] = "number";
	constexpr char success[] = "success";

	typedef ShellObject<
		ShellRetValue<std::string, 1, name>,
		ShellRetValue<unsigned long long, 2, size>
	> FormatDiskResult;

	typedef ShellObject<
		ShellRetValue<std::string, 1, name>,
		ShellRetValue<unsigned long long, 2, number>,
		ShellRetValue<bool, 3, success>
	> TestResult;

	typedef ShellObject<
		ShellRetValue<std::string, 1, path>
	> tDiskPostCreateResult;

	typedef ShellObject<
		ShellRetValue<std::string, 1, path>
	> GetMountPointResult;

	typedef ShellObject<
		ShellRetValue<std::string, 1, size>
	> DiskFreeSpaceResult;

} //end namespace shell

} //end namespace td

#endif //SHELLOBJECT_HPP
