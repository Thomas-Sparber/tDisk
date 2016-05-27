#ifndef JSONUTILS_HPP
#define JSONUTILS_HPP

#include <dropboxexception.hpp>

namespace td
{

	template <class T>
	inline void extractJson(T &out, const Json::Value &v, const std::string &value)
	{
		if(v.type() == Json::nullValue)throw DropboxException("Can't extract Json value \"",value,"\"");
		if(v[value].type() == Json::nullValue)throw DropboxException("Can't extract Json value \"",value,"\"");
		out = v[value];
	}

	template <>
	inline void extractJson(unsigned long long &out, const Json::Value &v, const std::string &value)
	{
		if(v.type() == Json::nullValue)throw DropboxException("Can't extract Json value \"",value,"\"");
		if(v[value].type() == Json::nullValue)throw DropboxException("Can't extract Json value \"",value,"\"");
		out = v[value].asUInt64();
	}

	template <class T, std::size_t size>
	inline void extractJson(std::array<T,size> &out, const Json::Value &v, const std::string &value)
	{
		if(v.type() != Json::arrayValue)throw DropboxException("Json value ", value, " is not an array");
		if(v.size() != size)throw DropboxException("Json array size of ", value, " not identical: ", v.size(), " != ", size);

		for(std::size_t i = 0; i < size; ++i)
			extractJson(out[i], v[value][(Json::Value::ArrayIndex)i], "");
	}

	template <class T, class ...V>
	inline void extractJson(T &out, const Json::Value &v, const std::string &value, V ...values)
	{
		if(v.type() == Json::nullValue)throw DropboxException("Can't extract Json value \"",value,"\"");
		if(v[value].type() == Json::nullValue)throw DropboxException("Can't extract Json value \"",value,"\"");
		extractJson(out, v[value], values...);
	}

	template <class T, class ...V>
	inline bool extractJsonOptional(T &out, const Json::Value &v, V ...values)
	{
		try {
			extractJson(out, v, values...);
			return true;
		} catch(const DropboxException &e) {
			return false;
		}
	}

	template <>
	inline void extractJson(std::string &out, const Json::Value &v, const std::string &value)
	{
		if(v[value].type() == Json::nullValue)throw DropboxException("Json value ", value, " is not a string");
		out = v[value].asString();
	}

	template <>
	inline void extractJson(uint64_t &out, const Json::Value &v, const std::string &value)
	{
		if(v[value].type() != Json::uintValue && v[value].type() != Json::intValue)throw DropboxException("Json value ", value, " is not a uint");
		out = v[value].asUInt64();
	}

	template <>
	inline void extractJson(bool &out, const Json::Value &v, const std::string &value)
	{
		if(v[value].type() != Json::booleanValue)throw DropboxException("Json value ", value, " is not a bool");
		out = v[value].asUInt64();
	}

	template <>
	inline void extractJson(double &out, const Json::Value &v, const std::string &value)
	{
		if(v[value].type() != Json::realValue)throw DropboxException("Json value ", value, " is not a double");
		out = v[value].asDouble();
	}

	template <>
	inline void extractJson(uint32_t &out, const Json::Value &v, const std::string &value)
	{
		if(v[value].type() != Json::uintValue && v[value].type() != Json::intValue)throw DropboxException("Json value ", value, " is not a uint");
		out = v[value].asUInt();
	}

} //end namespace td

#endif //JSONUTILS_HPP
