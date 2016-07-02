/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <vector>

#include <convert.hpp>

namespace td
{

struct Table
{
	Table(const std::string &str_name, const std::vector<std::string> &v_columns) :
		name(str_name),
		columns(v_columns)
	{}

	std::string name;
	std::vector<std::string> columns;
}; //end struct table

class Database;

class QueryResult
{

public:
	QueryResult() :
		result(SQLITE_ERROR),
		statement(nullptr)
	{}

	QueryResult(const QueryResult &other) = delete;

	QueryResult(QueryResult &&other) :
		result(other.result),
		statement(other.statement)
	{
		other.result = SQLITE_ERROR;
		other.statement = nullptr;
	}

	~QueryResult()
	{
		if(statement)sqlite3_finalize(statement);
	}

	QueryResult& operator=(const QueryResult &other) = delete;

	QueryResult& operator=(QueryResult &&other)
	{
		if(statement)sqlite3_finalize(statement);

		result = other.result;
		statement = other.statement;

		other.result = SQLITE_ERROR;
		other.statement = nullptr;

		return (*this);
	}

	operator bool() const
	{
		return (result == SQLITE_ROW);
	}

	void next()
	{
		result = sqlite3_step(statement);
	}

	int columnsCount()
	{
		return sqlite3_column_count(statement);
	}

	void getColumn(int col, std::string &out)
	{
		out = (const char*)sqlite3_column_text(statement, col);
	}

	template <class T>
	T get(int col)
	{
		T t;
		std::string value;
		getColumn(col, value);
		utils::convertTo(value, t);
		return std::move(t);
	}

	friend class Database;

private:
	int result;
	sqlite3_stmt *statement;

}; //end class QueryResult

class Database
{

public:
	Database(const std::string &str_path="", const std::vector<Table> &tables=std::vector<Table>()) :
		path(str_path),
		handle(nullptr),
		lastQuery()
	{
		if(path != "")
		{
			open();
			createTables(tables);
		}
	}

	Database(const Database &other) = delete;

	Database(Database &&other) = delete;

	~Database()
	{
		if(handle)close();
	}

	Database& operator=(const Database &other) = delete;

	Database& operator=(Database &&other) = delete;

	bool open(const std::string &str_path)
	{
		this->path = str_path;
		return open();
	}

	bool open()
	{
		if(handle)close();

		int result = sqlite3_open(path.c_str(), &handle);
		return (result == SQLITE_OK);
	}

	bool close()
	{
		int result = sqlite3_close(handle);
		handle = nullptr;
		return (result == SQLITE_OK);
	}

	bool createTable(const Table &table)
	{
		std::stringstream ss;
		ss<<"CREATE TABLE IF NOT EXISTS "<<table.name<<"(";

		bool first = true;
		for(const std::string &column : table.columns)
		{
			if(first)first = false;
			else ss<<",";

			ss<<column;
		}

		ss<<")";

		return execute(ss.str());
	}

	bool createTables(const std::vector<Table> &tables)
	{
		for(const Table &table : tables)
		{
			bool success = createTable(table);
			if(!success)return false;
		}

		return true;
	}

	QueryResult select(const std::string &table, const std::map<std::string,std::string> &filter=std::map<std::string,std::string>())
	{
		std::vector<std::string> parameters;
		std::stringstream ss;
		QueryResult result;

		ss<<"SELECT * FROM "<<table;

		bool first = true;
		for(const std::pair<std::string,std::string> &where : filter)
		{
			if(first)
			{
				first = false;
				ss<<" WHERE ";
			}
			else ss<<" AND ";

			//ss<<where.first<<"="<<where.second;
			parameters.push_back(where.second);
			ss<<where.first<<"=?";
		}

		execute(ss.str(), parameters, &result);
		return std::move(result);
	}

	QueryResult select(const Table &t, const std::map<std::string,std::string> &filter=std::map<std::string,std::string>())
	{
		return select(t.name, filter);
	}

	bool save(const std::string &table, const std::map<std::string,std::string> &columns)
	{
		std::vector<std::string> parameters;
		std::stringstream ss;

		ss<<"INSERT INTO "<<table<<"(";

		bool first = true;
		for(const std::pair<std::string,std::string> &column : columns)
		{
			if(first)first = false;
			else ss<<",";

			ss<<column.first;
		}

		ss<<") VALUES(";

		first = true;
		for(const std::pair<std::string,std::string> &column : columns)
		{
			if(first)first = false;
			else ss<<",";

			//ss<<column.second;
			parameters.push_back(column.second);
			ss<<"?";
		}
		ss<<")";

		return execute(ss.str(), parameters);
	}

	bool save(const Table &t, const std::map<std::string,std::string> &columns)
	{
		return save(t.name, columns);
	}

	bool execute(const std::string &query, QueryResult *out=nullptr)
	{
		return execute(query, std::vector<std::string>(), out);
	}

	bool execute(const std::string &query, const std::vector<std::string> &parameters, QueryResult *out=nullptr)
	{
		if(!handle)return false;
		lastQuery = query;

		sqlite3_stmt *statement;

		int result = sqlite3_prepare(handle, query.c_str(), query.length(), &statement, nullptr);
		if(result != SQLITE_OK)return false;

		for(std::size_t i = 0; i < parameters.size(); ++i)
		{
			sqlite3_bind_text(statement, (int)(i+1), parameters[i].c_str(), (int)parameters[i].length(), /*SQLITE_TRANSIENT*/SQLITE_STATIC);
		}

		result = sqlite3_step(statement);
		if(result != SQLITE_DONE && result != SQLITE_ROW && result != SQLITE_OK)return false;

		if(out)
		{
			out->statement = statement;
			out->result = result;
		}
		else
		{
			sqlite3_finalize(statement);
		}

		return true;
	}

	std::string getError()
	{
		return std::string("SQLite Error with query ") + lastQuery + ": " + sqlite3_errmsg(handle);
	}

private:
	std::string path;

	sqlite3 *handle;

	std::string lastQuery;

}; //end class Database

} //end namespace td

#endif //DATABASE_HPP
