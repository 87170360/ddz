#include <mysql/errmsg.h>
#include <stdio.h>
#include "XtSqlClient.h"


XtSqlClient::XtSqlClient()
{
	m_host="";
	m_port=0;
	m_username="";
	m_passwd="";
	m_dbname="";

	m_mysql=NULL;
	m_result=NULL;
}


XtSqlClient::~XtSqlClient()
{
	if(m_mysql)
	{
		mysql_close(m_mysql);
		m_mysql=NULL;
	}
}




int XtSqlClient::connect(const char* host,int port,const char* username,const char* passwd,const char* db)
{
	m_host=host;
	m_port=port;
	m_username=username;
	m_passwd=passwd;
	m_dbname=db;
	return connectSql();
}


int XtSqlClient::query(const char* sql)
{
	if(!m_mysql)
	{
		int re_ret=connectSql();
		if(re_ret!=0)
		{
			return -1;
		}
	}
	int ret=mysql_query(m_mysql,sql);

	if(ret!=0)
	{
		int re_ret=connectSql();
		if(re_ret==0)
		{
			ret=mysql_query(m_mysql,sql);
		}
	}

	if(ret==0)
	{
		return 0;
	}
	return -1;
}

//Get the record and store it int 'row'
vector<MYSQL_ROW> XtSqlClient::GetRecord(const char* str)
{
	std::vector<MYSQL_ROW> vecBetBox;

	MYSQL_RES *result = NULL;

	if (!m_mysql)
	{
		int re_ret = connectSql();
		if(re_ret !=0 )
		{
			return vecBetBox;
		}
	}

	int ret= mysql_query(m_mysql, str);
	if (ret!=0)
	{
		int re_ret = connectSql();
		if (0 == re_ret)
		{
			ret = mysql_query(m_mysql,str);
		}
	}

	if(ret==0)
	{
		result = mysql_store_result(m_mysql);
		//int rowcount = mysql_num_rows(result);
		//int fieldcount = mysql_num_fields(result);

	

		MYSQL_ROW row = NULL;
		row = mysql_fetch_row(result);
		while(NULL != row)
		{
			vecBetBox.push_back(row);
			row = mysql_fetch_row(result);
		}

		//mysql_free_result(result);
		//mysql_close(m_mysql);
	}

    return vecBetBox;
	
}

/*
MYSQL_ROW XtSqlClient::GetRecord(const char* sql)
{
SQL_FIELD *fd;
char column[32][32];
if(!m_mysql)
{
int re_ret=connectSql();
if(re_ret!=0)
{
return NULL;
}
}

int ret=mysql_query(m_mysql,sql);

if(ret!=0)
{
int re_ret=connectSql();
if(re_ret==0)
{
ret=mysql_query(m_mysql,sql);
}
}

if (0 == ret)
{
m_result=mysql_store_result(m_mysql);//保存查询到的数据到result

if (m_result)
{
int i,j;
cout<<"number of result: "<<(unsigned long)mysql_num_rows(m_result)<<endl;

for(i=0; fd=mysql_fetch_field(m_result); i++)//获取列名
{
strcpy(column[i],fd->name);
}

j=mysql_num_fields(m_result);
for(i=0;i<j;i++)
{
printf("%s\t",column[i]);
}
printf("\n");
while(m_row=mysql_fetch_row(m_result))//获取具体的数据
{
for(i=0;i<j;i++)
{
printf("%s\n",m_row[i]);
}
printf("\n");
}
}
}

return NULL;
}
*/
int XtSqlClient::connectSql()
{

	if(m_mysql)
	{
		mysql_close(m_mysql);
		m_mysql=NULL;
	}


	MYSQL* sql=mysql_init(NULL);
	if(sql==NULL)
	{
		return -1;
	}

	m_mysql=mysql_real_connect(sql,
		m_host.c_str(),
		m_username.c_str(),
		m_passwd.c_str(),
		m_dbname.c_str(),
		m_port,
		NULL,
		0);
	if(m_mysql==NULL)
	{
		mysql_close(sql);
		return -1;
	}

	return 0;
}



