#ifndef _XT_SQL_CLIENT_H_
#define _XT_SQL_CLIENT_H_

#include <mysql/mysql.h>
#include <string>

#include <vector>
using namespace std;

class BetBox;

///mysql¿Í»§¶Ë
class XtSqlClient 
{
	public:
		XtSqlClient();
		~XtSqlClient();

	public:
		int connect(const char* ip,int port,const char* username,const char* passwd,const char* dbname);
		int query(const char* sql);

		//Get the record and store it int 'row'
		vector<MYSQL_ROW> GetRecord(const char* str);

	

	public:
		int connectSql();



	private:
		std::string m_host;
		int  m_port;
		std::string m_username;
		std::string m_passwd;
		std::string m_dbname;

		MYSQL* m_mysql;
		MYSQL_RES* m_result;
		MYSQL_ROW m_row;

};



#endif /*_XT_SQL_CLIENT_H_*/


/*
#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
using namespace std;

main()
{
	MYSQL mysql;
	MYSQL_RES *result = NULL;
	MYSQL_FIELD *field = NULL;
	mysql_init(&mysql);
	mysql_real_connect(&mysql, 
		"127.0.0.1", 
		"root", 
		"wszgame",
		"wszdn",
		3306, 
		NULL, 
		0);
	string str = "select lottery_money, box_type, get_flag from bet_lottery_daily";
	mysql_query(&mysql, str.c_str());
	result = mysql_store_result(&mysql);
	int rowcount = mysql_num_rows(result);
	cout << rowcount << endl;
	int fieldcount = mysql_num_fields(result);
	cout << fieldcount << endl;
	for(int i = 0; i < fieldcount; i++)
	{
		field = mysql_fetch_field_direct(result,i);
		cout << field->name << "\t\t";
	}
	cout << endl;
	MYSQL_ROW row = NULL;
	row = mysql_fetch_row(result);
	while(NULL != row)
	{
		for(int i=0; i<fieldcount; i++)
		{
			cout << row[i] << "\t\t";
		}
		cout << endl;
		row = mysql_fetch_row(result);
	}
	mysql_close(&mysql);
}
*/
