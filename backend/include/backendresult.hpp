#ifndef BACKENDRESULT_HPP
#define BACKENDRESULT_HPP

#include <map>
#include <string>

#include <resultformatter.hpp>
#include <utils.hpp>

namespace td
{

/**
  * This enum defines the result type
 **/
enum class BackendResultType
{

	/** Means that the result is not specific to any component **/
	general,

	/** Means that the result is meant for the driver **/
	driver,

	/** Means that the result is meant for the config file **/
	configfile

}; //end enum BackendResultType

/**
  * Defines the result's error codes
 **/
enum class BackendErrorCode
{
	Success,

	Warning,

	Error
}; //end enum BackendErrorCode

/**
  * This struct defines an individual result, e.g. for the driver
 **/
struct IndividualResult
{

	/**
	  * Default constructor
	 **/
	IndividualResult() :
		errorCode(BackendErrorCode::Success),
		message(),
		warning(),
		error()
	{}

	/**
	  * Returns the IndividualResult's message. It looks like the following
	  *   ( Error-Code ) [Highest priority message e.g. Error]
	  *   	--> [Lower priority message e.g. Warning]
	  *   	--> ...
	  *   	--> [Lowest priority message e.g. Message]
	 **/
	std::string asString() const
	{
		if(message == "" && warning == "" && message == "")return getErrorStringCode(errorCode);

		return utils::concat(
			"( ",getErrorStringCode(errorCode)," ) ",

			(error != "") ? error : "",

			(warning != "" && error != "") ? "\n\t--> " : "",	//Insert --> if neccessary
			(warning != "") ? warning : "",

			(message != "" && (warning != "" || error != "")) ? "\n\t--> " : "",	//Insert --> if neccessary
			(message != "") ? message : ""
		);
	}

	/**
	  * Returns the string representation of the given error code
	 **/
	static std::string getErrorStringCode(BackendErrorCode errorCode)
	{
		switch(errorCode)
		{
			case BackendErrorCode::Success: return "Success";
			case BackendErrorCode::Warning: return "Warning";
			case BackendErrorCode::Error: return "Error";
		}

		return utils::concat("Invalid BackendErrorCode: ", int(errorCode));
	}

	/**
	  * The error code of the result
	 **/
	BackendErrorCode errorCode;

	/**
	  * The (non error) message of the result
	 **/
	std::string message;

	/**
	  * The warning message of the result
	 **/
	std::string warning;

	/**
	  * The error message of the result
	 **/
	std::string error;

}; //end struct IndividualResult

/**
  * This class contains result information about a Backend command
 **/
class BackendResult
{

public:

	/**
	  * Default constructor
	 **/
	BackendResult() :
		individualResults(),
		str_result()
	{}

	/**
	  * Sets the result message for the given type
	 **/
	void message(BackendResultType type, const std::string &m)
	{
		individualResults[type].message = m;
	}

	/**
	  * Sets the warning result message for the given type
	 **/
	void warning(BackendResultType type, const std::string &w)
	{
		individualResults[type].warning = w;
		individualResults[type].errorCode = std::max(individualResults[type].errorCode, BackendErrorCode::Warning);
	}

	/**
	  * Sets the error result message for the given type
	 **/
	void error(BackendResultType type, const std::string &e)
	{
		individualResults[type].error = e;
		individualResults[type].errorCode = std::max(individualResults[type].errorCode, BackendErrorCode::Error);
	}

	/**
	  * Gets the result message for the given type
	 **/
	std::string message(BackendResultType type) const
	{
		auto found = individualResults.find(type);
		if(found != individualResults.end())return found->second.message;
		return "";
	}

	/**
	  * Gets the warning result message for the given type
	 **/
	std::string warning(BackendResultType type) const
	{
		auto found = individualResults.find(type);
		if(found != individualResults.end())return found->second.warning;
		return "";
	}

	/**
	  * Gets the error result message for the given type
	 **/
	std::string error(BackendResultType type) const
	{
		auto found = individualResults.find(type);
		if(found != individualResults.end())return found->second.error;
		return "";
	}

	/**
	  * Gets all result messages for the BackendResult
	 **/
	std::string messages() const
	{
		std::string ret;

		for(const auto &p : individualResults)
		{
			if(p.second.message.empty())continue;
			ret = utils::concat(ret, (ret.empty() ? "" : "\n"), "(",BackendResult::getTypeString(p.first),"): ",p.second.message);
		}

		return std::move(ret);
	}

	/**
	  * Gets all warning result messages for the BackendResult
	 **/
	std::string warnings() const
	{
		std::string ret;

		for(const auto &p : individualResults)
		{
			if(p.second.warning.empty())continue;
			ret = utils::concat(ret, (ret.empty() ? "" : "\n"), "(",BackendResult::getTypeString(p.first),"): ",p.second.warning);
		}

		return std::move(ret);
	}

	/**
	  * Gets all error result messages for the BackendResult
	 **/
	std::string errors() const
	{
		std::string ret;

		for(const auto &p : individualResults)
		{
			if(p.second.error.empty())continue;
			ret = utils::concat(ret, (ret.empty() ? "" : "\n"), "(",BackendResult::getTypeString(p.first),"): ",p.second.error);
		}

		return std::move(ret);
	}

	/**
	  * Gets the error code for the entire result which is
	  * the maximum code in all individual results
	 **/
	BackendErrorCode errorCode() const
	{
		BackendErrorCode code = BackendErrorCode::Success;

		for(const auto &res : individualResults)
		{
			code = std::max(res.second.errorCode, code);
		}

		return code;
	}

	/**
	  * Gets the error code for the given type
	 **/
	BackendErrorCode errorCode(BackendResultType type) const
	{
		auto found = individualResults.find(type);
		if(found != individualResults.end())return found->second.errorCode;
		return BackendErrorCode::Success;
	}

	/**
	  * Sets actual string result
	 **/
	void result(const std::string &r)
	{
		str_result = r;
	}

	/**
	  * Sets actual result
	 **/
	template <class T>
	void result(const T &t, const utils::ci_string &output_format)
	{
		try {
			std::stringstream ss;
			createResultString(ss, t, 0, output_format);
			str_result = ss.str();
		} catch (const FormatException &e) {
			error(BackendResultType::general, e.what());
		}
	}

	/**
	  * Gets the actual string result
	 **/
	const std::string& result() const
	{
		return str_result;
	}

	/**
	  * Returns the string representation of the given type
	 **/
	static std::string getTypeString(BackendResultType type)
	{
		switch(type)
		{
			case BackendResultType::general: return "General";
			case BackendResultType::driver: return "Driver";
			case BackendResultType::configfile: return "Configfile";
		}

		return utils::concat("[Invalid type: ]", (int)type);
	}

	/**
	  * Returns all available result types which are present in the BackendResult
	 **/
	std::vector<BackendResultType> getResultTypes() const
	{
		std::vector<BackendResultType> types;
		for(const auto &res : individualResults)
			types.push_back(res.first);
		return std::move(types);
	}

	/**
	  * Returns the individual result for the given type
	 **/
	const IndividualResult& getIndividualResult(BackendResultType type) const
	{
		auto found = individualResults.find(type);
		return found->second;
	}

private:

	/**
	  * This map is used to store the results
	 **/
	std::map<BackendResultType,IndividualResult> individualResults;

	/**
	  * This string represents the actual result of an operation
	 **/
	std::string str_result;

}; //end class backendResult

/**
  * Stringifies the given BackendErrorCode using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const BackendErrorCode &code, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	createResultString(ss, IndividualResult::getErrorStringCode(code), hierarchy, outputFormat);
}

/**
  * Stringifies the given BackendResultType using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const BackendResultType &type, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	createResultString(ss, BackendResult::getTypeString(type), hierarchy, outputFormat);
}

/**
  * Stringifies the given IndividualResult using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const IndividualResult &result, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, result, errorCode, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, result, message, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, result, warning, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, result, error, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, result, errorCode, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, result, message, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, result, warning, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, result, error, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //BACKENDRESULT_HPP
