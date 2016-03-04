#ifndef HISTORYBUFFER_HPP
#define HISTORYBUFFER_HPP

#include <set>
#include <vector>

namespace td
{

struct HistoryElement
{

	HistoryElement(unsigned long long llu_pos) :
		pos(llu_pos),
		data(),
		accessCount()
	{}

	HistoryElement(unsigned long long llu_pos, const char *c_data, std::size_t length, unsigned int ui_accessCount=0) :
		pos(llu_pos),
		data(c_data, c_data+length),
		accessCount(ui_accessCount)
	{}

	bool operator< (const HistoryElement &other) const
	{
		return pos < other.pos;
	}

	bool operator< (const unsigned long long &other) const
	{
		return pos < other;
	}

	unsigned long long pos;
	std::vector<char> data;
	unsigned int accessCount;

}; //end struct HistoryElement

class HistoryBuffer
{

public:
	HistoryBuffer(unsigned int ui_maxHistory) :
		maxHistory(ui_maxHistory),
		history()
	{}

	std::multiset<HistoryElement>::iterator findIntersecting(unsigned long long pos, std::size_t length)
	{
		for(auto it = history.begin(); it != history.end() && it->pos < pos+length; ++it)
		{
			if(it->pos + it->data.size() > pos)
				return it;
		}

		return history.end();
	}

	std::multiset<HistoryElement>::iterator find(unsigned long long pos, std::size_t length)
	{
		for(auto it = history.begin(); it != history.end() && it->pos <= pos; ++it)
		{
			if(it->data.size() >= (pos-it->pos) && it->data.size() - (pos-it->pos) >= length)
				return it;
		}

		return history.end();
	}

	std::multiset<HistoryElement>::const_iterator find(unsigned long long pos, std::size_t length) const
	{
		for(auto it = history.cbegin(); it != history.cend() && it->pos <= pos; ++it)
		{
			if(it->data.size() >= (pos-it->pos) && it->data.size() - (pos-it->pos) >= length)
				return it;
		}

		return history.cend();
	}

	bool get(unsigned long long pos, char *out, std::size_t length) const
	{
		auto it = find(pos, length);
		if(it != history.cend())
		{
			memcpy(out, &it->data[std::size_t(pos-it->pos)], length);
			return true;
		}

		return false;
	}

	void set(unsigned long long pos, const char *data, std::size_t length)
	{
		if(maxHistory == 0)return;

		unsigned int accessCount = 0;
		auto it = history.end();
		while((it=findIntersecting(pos, length)) != history.end())
		{
			accessCount += it->accessCount;
			history.erase(it);
		}

		if(history.size() >= maxHistory)removeHistory(history.size()-maxHistory + 1);
		history.emplace(pos, data, length, accessCount);
	}

	void removeHistory(std::size_t amount)
	{
		while(amount > 0 && !history.empty())
		{
			auto smallest = history.begin();

			for(auto it = history.begin(); it != history.end(); ++it)
				if(it->accessCount < smallest->accessCount)smallest = it;

			history.erase(smallest);
		}
	}

private:
	unsigned int maxHistory;
	std::multiset<HistoryElement> history;

}; //end class HistoryBuffer

} //end namespace td

#endif //HISTORYBUFFER_HPP
