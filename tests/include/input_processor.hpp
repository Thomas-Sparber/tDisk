#ifndef INPUT_PROCESSOR_HPP
#define INPUT_PROCESSOR_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include <utils.hpp>

struct InputDefinition
{

	InputDefinition(const std::string &str_name, const std::string &str_question) :
		name(str_name),
		question(str_question),
		defaultValue(""),
		value(""),
		choices()
	{}

	InputDefinition(const std::string &str_name, const std::string &str_question, const std::string &str_defaultValue) :
		name(str_name),
		question(str_question),
		defaultValue(str_defaultValue),
		value(""),
		choices()
	{}

	InputDefinition(const std::string &str_name, const std::string &str_question, const std::vector<std::string> &v_choices) :
		name(str_name),
		question(str_question),
		defaultValue(""),
		value(""),
		choices(v_choices)
	{}

	InputDefinition(const std::string &str_name, const std::string &str_question, const std::vector<std::string> &v_choices, const std::string &str_defaultValue) :
		name(str_name),
		question(str_question),
		defaultValue(str_defaultValue),
		value(""),
		choices(v_choices)
	{}

	std::string name;
	std::string question;
	std::string defaultValue;
	std::string value;
	std::vector<std::string> choices;

}; //end struct InputDefinition

struct InputException
{
	template <class ...T> InputException(T ...t) : message(td::concat(t...)) {}
	std::string message;
}; //end struct InputException

inline void askInputQuestion(InputDefinition &input)
{
	bool repeat = true;

	while(repeat)
	{
		std::string answer;
		std::cout<<input.question<<std::endl;

		for(std::size_t i = 0; i < input.choices.size(); ++i)
			std::cout<<"  ("<<i+1<<"): "<<input.choices[i]<<std::endl;

		std::cout<<"> ";
		if(input.defaultValue != "")std::cout<<"["<<input.defaultValue<<"] "<<std::endl;
		std::getline(std::cin, answer);

		if(input.choices.empty())
		{
			if(answer == "")
			{
				if(input.defaultValue == "")
				{
					std::cout<<"Please enter a value"<<std::endl;
					std::cout<<std::endl;
					continue;
				}

				input.value = input.defaultValue;
			}
			else
			{
				input.value = answer;
			}

			repeat = false;
		}
		else
		{
			char *end;
			if(answer == "")
			{
				if(input.defaultValue == "")
				{
					std::cout<<"Please enter a value"<<std::endl;
					std::cout<<std::endl;
					continue;
				}

				input.value = input.defaultValue;
			}
			else
			{
				int choice = strtol(answer.c_str(), &end, 10);
				if(end != &answer[answer.length()])
				{
					std::cout<<"Invalid answer: "<<answer<<std::endl;
					std::cout<<std::endl;
					continue;
				}

				if(choice <= 0 || choice > (int)input.choices.size())
				{
					std::cout<<"Please choose a valid answer (1-"<<input.choices.size()<<")"<<std::endl;
					std::cout<<std::endl;
					continue;
				}

				input.value = input.choices[choice-1];
			}

			repeat = false;
		}
	}

	std::cout<<std::endl;
}

inline void askRemainingInputQuestions(std::vector<InputDefinition> &inputs)
{
	for(InputDefinition &input : inputs)
	{
		if(input.value == "")
		{
			askInputQuestion(input);
		}
	}
}

inline void setInputQuestions(std::vector<InputDefinition> &inputs, int argc, char *args[])
{
	for(int i = 1; i+1 < argc; ++i)
	{
		const std::string name(args[i]);
		const std::string value(args[i+1]);

		for(InputDefinition &input : inputs)
		{
			if(std::string("--") + input.name == name)
			{
				input.value = value;
				break;
			}
		}
	}
}

template <class T>
inline T getValueInternal(const InputDefinition &input)
{
	throw InputException("InputProcessor can't handle type ", typeid(T).name());
}

template <>
inline char getValueInternal(const InputDefinition &input)
{
	if(input.value.length() != 1)throw InputException("Can't convert value \"", input.value, "\" to type char");
	return input.value[0];
}

template <>
inline std::string getValueInternal(const InputDefinition &input)
{
	return input.value;
}

template <>
inline unsigned long long getValueInternal(const InputDefinition &input)
{
	char *end;
	unsigned long long ret = strtoull(input.value.c_str(), &end, 10);
	if(end != &input.value[input.value.length()])throw InputException("Can't convert value \"", input.value, "\" to type unsigned long");
	return ret;
}

template <>
inline long long getValueInternal(const InputDefinition &input)
{
	char *end;
	long long ret = strtoll(input.value.c_str(), &end, 10);
	if(end != &input.value[input.value.length()])throw InputException("Can't convert value \"", input.value, "\" to type long");
	return ret;
}

template <>
inline unsigned long getValueInternal(const InputDefinition &input)
{
	char *end;
	long ret = strtoul(input.value.c_str(), &end, 10);
	if(end != &input.value[input.value.length()])throw InputException("Can't convert value \"", input.value, "\" to type unsigned long");
	return ret;
}

template <>
inline long getValueInternal(const InputDefinition &input)
{
	char *end;
	long ret = strtol(input.value.c_str(), &end, 10);
	if(end != &input.value[input.value.length()])throw InputException("Can't convert value \"", input.value, "\" to type long");
	return ret;
}

template <>
inline int getValueInternal(const InputDefinition &input)
{
	return getValueInternal<long>(input);
}

template <>
inline unsigned int getValueInternal(const InputDefinition &input)
{
	return getValueInternal<unsigned long>(input);
}

template <class T>
inline T getValue(const std::vector<InputDefinition> &v_inputs, const std::string &name)
{
	for(const InputDefinition &input : v_inputs)
	{
		if(input.name == name)
		{
			return getValueInternal<T>(input);
		}
	}

	throw InputException("No input value ", name, " defined");
}

#endif //INPUT_PROCESSOR_HPP
