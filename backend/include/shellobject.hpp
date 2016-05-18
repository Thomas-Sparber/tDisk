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

namespace td
{

namespace shell
{

	struct BaseRetValue
	{

		virtual ~BaseRetValue() {}

		virtual const std::string& getValue() const = 0;

		virtual void setValue(const std::string &value) = 0;

	}; //end struct BaseRetValue

	template <class Type, int line, const char *name>
	struct RetValue : BaseRetValue
	{
		RetValue() :
			BaseRetValue(),
			value()
		{}

		RetValue(const std::string &v) :
			value(v)
		{}

		const char* getName() const
		{
			return name;
		}

		int getLine() const
		{
			return line;
		}

		virtual const std::string& getValue() const
		{
			return value;
		}

		virtual void setValue(const std::string &v)
		{
			this->value = v;
		}

		std::string value;
	}; //end class RetValue

	class ShellObjectBase
	{

	public:
		ShellObjectBase() :
			result()
		{}

		virtual ~ShellObjectBase() {}

		template <const char *name, class T = std::string>
		bool get(T &out) const
		{
			return utils::convertTo<T>(getValue(name), out);
		}

		template <const char *name, class T = std::string>
		T get() const
		{
			T t;
			get<name,T>(t);
			return std::move(t);
		}

		virtual ShellObjectBase* clone() const = 0;

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

		const std::string& getMessage() const
		{
			return result;
		}

	protected:
		virtual const std::string& getValue(const char *name) const = 0;

		virtual void parseArguments(const std::string &r) = 0;

	private:
		std::string result;

	}; //end class ShellObjectBase

	template <class ...RetValues>
	class ShellObject : public ShellObjectBase, RetValues...
	{

	public:
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
				BaseRetValue &v = getRetValue((int)(i+1), retValues);
				v.setValue(splitted[i]);
			}
		}

	private:
		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), BaseRetValue&>::type
		getRetValue(const char *name, std::tuple<Tp...>&)
		{
			throw BackendException("Parameter ",name," does not exist in ShellObject");
		}

		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), BaseRetValue&>::type
		getRetValue(const char *name, std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getName() == name)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(name, t);
		}

		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), const BaseRetValue&>::type
		getRetValue(const char *name, const std::tuple<Tp...>&)
		{
			throw BackendException("Parameter ",name," does not exist in ShellObject");
		}

		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), const BaseRetValue&>::type
		getRetValue(const char *name, const std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getName() == name)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(name, t);
		}



		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), BaseRetValue&>::type
		getRetValue(int line, std::tuple<Tp...>&)
		{
			throw BackendException("Parameter for line ",line," does not exist in ShellObject");
		}

		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), BaseRetValue&>::type
		getRetValue(int line, std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getLine() == line)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(line, t);
		}

		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I == sizeof...(Tp), const BaseRetValue&>::type
		getRetValue(int line, const std::tuple<Tp...>&)
		{
			throw BackendException("Parameter for line ",line," does not exist in ShellObject");
		}

		template<std::size_t I = 0, typename... Tp>
		static typename std::enable_if<I < sizeof...(Tp), const BaseRetValue&>::type
		getRetValue(int line, const std::tuple<Tp...>& t)
		{
			if(std::get<I>(t).getLine() == line)return std::get<I>(t);
			return getRetValue<I + 1, Tp...>(line, t);
		}

	private:
		std::tuple<RetValues...> retValues;

	}; //end class ShellObject


	constexpr char name[] = "name";
	constexpr char size[] = "size";
	constexpr char number[] = "number";
	constexpr char success[] = "success";

	typedef ShellObject<
		RetValue<std::string, 1, name>,
		RetValue<unsigned long long, 2, size>
	> FormatDiskResult;

	typedef ShellObject<
		RetValue<std::string, 1, name>,
		RetValue<unsigned long long, 2, number>,
		RetValue<bool, 3, success>
	> TestResult;

} //end namespace shell

} //end namespace td

#endif //SHELLOBJECT_HPP
