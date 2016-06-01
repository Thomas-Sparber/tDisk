#ifndef WRITEBUFFER_HPP
#define WRITEBUFFER_HPP

#include <ctime>
#include <deque>
#include <vector>

#include <historybuffer.hpp>

namespace td
{

typedef HistoryElement WriteBufferElement;

class WriteBuffer
{

public:
	WriteBuffer(unsigned long long llu_maxBytes) :
		maxBytes(llu_maxBytes),
		buffer(),
		lastAccessTime()
	{}

	bool empty() const
	{
		if(buffer.empty())return true;

		for(const WriteBufferElement &e : buffer)
			if(!e.data.empty())return false;

		return true;
	}

	unsigned long long size() const
	{
		unsigned long long s = 0;
		for(const WriteBufferElement &e : buffer)
			s += e.data.size();
		return s;
	}

	std::time_t age() const
	{
		return (std::time(nullptr) - lastAccessTime);
	}

	bool canBeCombined(unsigned long long offset, const char */*data*/, std::size_t length)
	{
		if(buffer.empty())return true;
		if(offset + length == buffer.front().pos)return true;
		if((buffer.back().pos + buffer.back().data.size()) == offset)return true;
		return false;
	}

	bool append(unsigned long long offset, const char *data, std::size_t length)
	{
		if(offset + length == buffer.front().pos)
			buffer.emplace_front(offset, data, length);
		else
			buffer.emplace_back(offset, data, length);

		lastAccessTime = std::time(nullptr);
		return size() >= maxBytes;
	}

	std::vector<char> getAndPop(unsigned long long &outPos)
	{
		std::vector<char> ret;

		if(buffer.empty())
		{
			outPos = 0;
			return std::move(ret);
		}
		else outPos = buffer.front().pos;

		for(const WriteBufferElement &e : buffer)
			ret.insert(ret.end(), e.data.begin(), e.data.end());

		lastAccessTime = std::time(nullptr);
		buffer.clear();
		return std::move(ret);
	}

	bool read(unsigned long long offset, char *data, std::size_t length)
	{
		if(buffer.empty())return false;
		if(buffer.front().pos > offset || buffer.back().pos+buffer.back().data.size() < offset+length)return false;

		for(const WriteBufferElement &e : buffer)
		{
			std::size_t dataOffset = (std::size_t)(offset-e.pos);

			for(std::size_t i = dataOffset; i < e.data.size() && length > 0; ++i)
			{
				(*data) = e.data[i];
				data++;
				length--;
			}

			if(length == 0)break;
		}

		return true;
	}

private:
	unsigned long long maxBytes;
	std::deque<WriteBufferElement> buffer;
	std::time_t lastAccessTime;

}; //end class WriteBuffer

} //end namespace td

#endif //WRITEBUFFER_HPP
