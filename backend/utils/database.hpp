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

class Database;

/**
  * This struct represents the structure of a database table
 **/
struct Table
{
	/**
	  * The constructor takes the name and the columns
	  * of the table
	 **/
	Table(const std::string &str_name, const std::vector<std::string> &v_columns) :
		name(str_name),
		columns(v_columns)
	{}

	/**
	  * The name of the database table
	 **/
	std::string name;

	/**
	  * The columns of the table
	 **/
	std::vector<std::string> columns;

}; //end struct table

/**
  * This class represents a query result from a database query
 **/
class QueryResult
{

public:

	/**
	  * Default constructor which initializes an invalid QueryResult
	 **/
	QueryResult() :
		result(SQLITE_ERROR),
		statement(nullptr)
	{}

	/**
	  * Query results can't be copied since they hold
	  * references to the queries
	 **/
	QueryResult(const QueryResult &other) = delete;

	/**
	  * Move constructor
	 **/
	QueryResult(QueryResult &&other) :
		result(other.result),
		statement(other.statement)
	{
		other.result = SQLITE_ERROR;
		other.statement = nullptr;
	}

	/**
	  * The destructor closes the sqlite statement
	 **/
	~QueryResult()
	{
		if(statement)sqlite3_finalize(statement);
	}

	/**
	  * Query results can't be assigned
	 **/
	QueryResult& operator=(const QueryResult &other) = delete;

	/**
	  * Move operator
	 **/
	QueryResult& operator=(QueryResult &&other)
	{
		if(statement)sqlite3_finalize(statement);

		result = other.result;
		statement = other.statement;

		other.result = SQLITE_ERROR;
		other.statement = nullptr;

		return (*this);
	}

	/**
	  * A QueryResult can be casted to bool to be used in a for loop, e.g
	  * for(QueryResult r = db.execute(...); r; r.next())...
	 **/
	operator bool() const
	{
		return (result == SQLITE_ROW);
	}

	/**
	  * A QueryResult can be used somehow like an iterator
	  * to be used in a for loop, e.g
	  * for(QueryResult r = db.execute(...); r; r.next())...
	 **/
	void next()
	{
		result = sqlite3_step(statement);
	}

	/**
	  * Returns the amount of columns in the result
	 **/
	int columnsCount()
	{
		return sqlite3_column_count(statement);
	}

	/**
	  * Returns the string representation of the column with the given index
	 **/
	void getColumn(int col, std::string &out)
	{
		out = (const char*)sqlite3_column_text(statement, col);
	}

	/**
	  * Returns the value of the column with the given index,
	  * converted to the desired value type
	 **/
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

	/**
	  * The SQLite result code
	 **/
	int result;

	/**
	  * The SQLite statement
	 **/
	sqlite3_stmt *statement;

}; //end class QueryResult

/**
  * This class represents a very simple SQLite database interface
  * which provides most of the functionality
 **/
class Database
{

public:

	/**
	  * The constructor takes an optional filename and
	  * optional tables. If the filename is given, the database
	  * is created and opened. If the tables are given too, they
	  * are created as well
	 **/
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

	/**
	  * A database can't be copied
	 **/
	Database(const Database &other) = delete;

	/**
	  * Move constructor
	 **/
	Database(Database &&other) :
		path(std::move(other.path)),
		handle(other.handle),
		lastQuery(std::move(other.lastQuery))
	{
		other.handle = nullptr;
	}

	/**
	  * The destructor closes the SQLite databse connection
	 **/
	~Database()
	{
		if(handle)close();
	}

	/**
	  * Assigning of databases is not allowed
	 **/
	Database& operator=(const Database &other) = delete;

	/**
	  * Move operator
	 **/
	Database& operator=(Database &&other)
	{
		if(handle)close();

		path = std::move(other.path);
		handle = other.handle;
		lastQuery = std::move(other.lastQuery);

		other.handle = nullptr;

		return (*this);
	}

	/**
	  * Sets the path and opens the connection to the database
	 **/
	bool open(const std::string &str_path)
	{
		this->path = str_path;
		return open();
	}

	/**
	  * Opens the connection to the database
	 **/
	bool open()
	{
		if(handle)close();

		int result = sqlite3_open(path.c_str(), &handle);
		return (result == SQLITE_OK);
	}

	/**
	  * Closes the database connection
	 **/
	bool close()
	{
		int result = sqlite3_close(handle);
		handle = nullptr;
		return (result == SQLITE_OK);
	}

	/**
	  * Creates the given table
	 **/
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

	/**
	  * Creates the given list of tables
	 **/
	bool createTables(const std::vector<Table> &tables)
	{
		for(const Table &table : tables)
		{
			bool success = createTable(table);
			if(!success)return false;
		}

		return true;
	}

	/**
	  * Selects from the table with the given name
	  * and uses an optional filter, e.g
	  * db.select("mytable", { { "id", "1" } });
	 **/
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

			parameters.push_back(where.second);
			ss<<where.first<<"=?";
		}

		execute(ss.str(), parameters, &result);
		return std::move(result);
	}

	/**
	  * Selects from the table with the given name
	  * and uses an optional filter, e.g
	  * db.select(mytable, { { "id", "1" } });
	 **/
	QueryResult select(const Table &t, const std::map<std::string,std::string> &filter=std::map<std::string,std::string>())
	{
		return select(t.name, filter);
	}

	/**
	  * Removes from the table with the given name
	  * and uses an optional filter, e.g
	  * db.remove("mytable", { { "id", "1" } });
	 **/
	bool remove(const std::string &table, const std::map<std::string,std::string> &filter=std::map<std::string,std::string>())
	{
		std::vector<std::string> parameters;
		std::stringstream ss;

		ss<<"DELETE FROM "<<table;

		bool first = true;
		for(const std::pair<std::string,std::string> &where : filter)
		{
			if(first)
			{
				first = false;
				ss<<" WHERE ";
			}
			else ss<<" AND ";

			parameters.push_back(where.second);
			ss<<where.first<<"=?";
		}

		return execute(ss.str(), parameters);
	}

	/**
	  * Saves a row to the given table
	 **/
	bool save(const std::string &table, const std::map<std::string,std::string> &columns)
	{
		std::vector<std::string> parameters;
		std::stringstream ss;

		ss<<"INSERT OR REPLACE INTO "<<table<<"(";

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

	/**
	  * Saves a row to the given table
	 **/
	bool save(const Table &t, const std::map<std::string,std::string> &columns)
	{
		return save(t.name, columns);
	}

	/**
	  * Executes the given query without query parameters
	 **/
	bool execute(const std::string &query, QueryResult *out=nullptr)
	{
		return execute(query, std::vector<std::string>(), out);
	}

	/**
	  * Executes the given query
	 **/
	bool execute(const std::string &query, const std::vector<std::string> &parameters, QueryResult *out=nullptr)
	{
		if(!handle)return false;
		lastQuery = query;

		sqlite3_stmt *statement;

		int result = sqlite3_prepare(handle, query.c_str(), (int)query.length(), &statement, nullptr);
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

	/**
	  * Returns the last error
	 **/
	std::string getError()
	{
		return std::string("SQLite Error with query ") + lastQuery + ": " + sqlite3_errmsg(handle);
	}

private:

	/**
	  * The path to the database
	 **/
	std::string path;

	/**
	  * The SQLite handle
	 **/
	sqlite3 *handle;

	/**
	  * The last query is recorded for debuggin purposes
	 **/
	std::string lastQuery;

}; //end class Database

} //end namespace td

#endif //DATABASE_HPP
