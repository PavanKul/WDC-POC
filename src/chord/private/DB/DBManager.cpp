/*
 * DBManager.cpp
 *
 *  Created on: Jan 27, 2023
 *      Author: Priyashree Guha Roy
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DB/DBManager.h"

#define DISPLAY_MYSQL_ERROR						0

DBManager :: DBManager()
{
	host = NULL;
	user = NULL;
	password = NULL;
	database = NULL;
	buff = NULL;
	port = 0;
}

DBManager :: ~DBManager()
{
	releaseMemory();
}

/**************************************************************
 * Database information read from the configuration file here
 **************************************************************/

int DBManager :: initDBManager()
{
	int rs = 0;
	/*char *temp;

	host = (char*) calloc(1, (MAX_CONFIG_PARAM_LEN * sizeof(char)));
	user = (char*) calloc(1, (MAX_CONFIG_PARAM_LEN * sizeof(char)));
	password = (char*) calloc(1, (MAX_CONFIG_PARAM_LEN * sizeof(char)));
	database = (char*) calloc(1, (MAX_CONFIG_PARAM_LEN * sizeof(char)));
	buff = (char*) calloc(1, (MAX_CONFIG_PARAM_LEN * sizeof(char)));

	FILE *fp = fopen(MYSQL_CONFIG_FILE, "r");

	if(!fp)
	{
		perror("\n---------- No Database Configuration File Found -----------\n");
		rs = 1;
		fflush(stdout);
	}
	else
	{
		fscanf(fp, "%s\n", buff);

		temp = strstr(buff, "HOST");
		if(temp)
		{
			temp++;
			temp = strstr(temp, "=");
			temp++;
			strcpy(host, temp);
		}
		else
		{
			rs = 1;
		}

		memset(buff, 0, MAX_CONFIG_PARAM_LEN);
		fscanf(fp, "%s\n", buff);

		temp = strstr(buff, "USERNAME");
		if(temp)
		{
			temp++;
			temp = strstr(temp, "=");
			temp++;
			strcpy(user, temp);
		}
		else
		{
			rs = 1;
		}

		memset(buff, 0, MAX_CONFIG_PARAM_LEN);
		fscanf(fp, "%s\n", buff);

		temp = strstr(buff, "PASSWORD");
		if(temp)
		{
			temp++;
			temp = strstr(temp, "=");
			temp++;
			strcpy(password, temp);
		}
		else
		{
			rs = 1;
		}

		memset(buff, 0, MAX_CONFIG_PARAM_LEN);
		fscanf(fp, "%s\n", buff);

		temp = strstr(buff, "PORT");
		if(temp)
		{
			temp++;
			temp = strstr(temp, "=");
			temp++;
			port = atoi(temp);
		}
		else
		{
			rs = 1;
		}

		memset(buff, 0, MAX_CONFIG_PARAM_LEN);
		fscanf(fp, "%s\n", buff);

		temp = strstr(buff, "DATABASE");
		if(temp)
		{
			temp++;
			temp = strstr(temp, "=");
			temp++;
			strcpy(database, temp);
		}
		else
		{
			rs = 1;
		}

		fclose(fp);

	}*/

    host = "172.30.44.119";
    user = "root";
    password = "Master@1234";
    database = "File";
    port = 3306;
	return rs;
}

/**********************************
 * Create database connection here
 **********************************/

MYSQL* DBManager :: createMySQLConnection()
{
	MYSQL * conn = mysql_init(NULL);
	if (!mysql_real_connect(conn, host,
				user, password, database, port, NULL, 0))
	{
		fprintf(stderr, "%s\n", mysql_error(conn));
		fflush(stdout);
	}
	return conn;
}


/************************************************
 * Check the fileName already exist in table here
 ************************************************/

int DBManager :: ifFilePresent(char* fileName, uint& key)
{
	int rs = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(query, "SELECT * FROM FileList fl where fl.FileName='%s';", fileName);
	conn = createMySQLConnection();
	if(conn != NULL)
		rs = mysql_query(conn, query);
	if(!rs)
	{
		res = mysql_store_result(conn);
		if((row = mysql_fetch_row(res)) != NULL)
		{
			rs = 1;
			key = (uint)atoi(row[0]);
			closeMySQLConnection(conn);
			conn = NULL;
		}
	}
	else
	{
		closeMySQLConnection(conn);
		conn = NULL;
	}

	return rs;
}

/************************************************
 * Check the hash key already exist in table here
 ************************************************/

int DBManager :: ifHashPresent(uint hash)
{
	int rs = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(query, "SELECT * FROM FileList fl where fl.key=%d;", hash);
	conn = createMySQLConnection();
	if(conn != NULL)
		rs = mysql_query(conn, query);
	if(!rs)
	{
		res = mysql_store_result(conn);
		if((row = mysql_fetch_row(res)) != NULL)
		{
			rs = 1;
			closeMySQLConnection(conn);
			conn = NULL;
		}
	}
	else
	{
		closeMySQLConnection(conn);
		conn = NULL;
	}
	return rs;
}

/****************************************
 * Insert file name and Key in table here
 ****************************************/

int DBManager :: insertToFileList(char* fileName, uint hash)
{
	int rs = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	sprintf(query, "INSERT INTO `FileList` (`key`, `FileName`) VALUES (%d, '%s');", hash, fileName);
	conn = createMySQLConnection();
	if(conn != NULL)
	{
		rs = mysql_query(conn, query);
		closeMySQLConnection(conn);
		conn = NULL;
	}

	return rs;
}

/************************************************
 * Update number of file copies field in table here
************************************************/

bool DBManager :: updatefilecopies(uint& Parent_Key, int File_Copies)
{
	int rs = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	if ( (ifHashPresent(Parent_Key)) != NULL )
	{
		sprintf(query, "UPDATE `FileList` SET `File_Copies` = '%d' WHERE `Parent_Key` = '%d';", File_Copies, Parent_Key);
		conn = createMySQLConnection();
		if(conn != NULL)
		{
			rs = mysql_query(conn, query);
			closeMySQLConnection(conn);
			conn = NULL;
		}
	}
	else 
	{
		fprintf(stderr, "Parent Key '%d' does not exist\n", Parent_Key);
		fflush(stdout);
		closeMySQLConnection(conn);
		conn = NULL;
	}
	return rs;
}

/************************************************
 * Show number of file copies field in table here
************************************************/

int DBManager :: selectfilecopies(char* fileName)
{
	int rs = 0;
	int num_fields = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	sprintf(query, "SELECT `File_Copies` FROM `FileList` WHERE `FileName` = '%s';", fileName);
	conn = createMySQLConnection();
	if(conn != NULL)
		rs = mysql_query(conn, query);
	if(!rs)
	{
		res = mysql_store_result(conn);
		num_fields = mysql_num_fields(res);
		closeMySQLConnection(conn);
		conn = NULL;
	}
	else
	{
		closeMySQLConnection(conn);
		conn = NULL;
	}

	return num_fields;
}

/***********************************************************************
 * Insert file name, parent key, file size and file copies in file table here
***********************************************************************/

bool DBManager :: insertIntoToFileList(uint& Parent_Key, char* File_Name, int File_Size, int File_Copies)
{
        int rs = 0;
        char query[100] = {'\0'};
        MYSQL *conn = NULL;
        sprintf(query, "INSERT INTO `FileList` (`Parent_Key`, `File_Name`, `File_Size`,`File_Copies`) VALUES (%d, '%s', '%d');", Parent_Key, File_Name, File_Size,File_Copies);
        conn = createMySQLConnection();
        if(conn != NULL)
        {
                rs = mysql_query(conn, query);
                closeMySQLConnection(conn);
                conn = NULL;
        }
	bool result = ( rs <= 0 ) ? false : true;
        return result;
}

/***********************************************************************
 * Insert file name, parent key, chunk key and order id in Chunk table here
 ***********************************************************************/

bool DBManager :: insertIntoChunkList(uint& Parent_Key, uint& Chunk_Key, char* File_Name, char* Order_Id)
{
        int rs = 0;
        char query[100] = {'\0'};
        MYSQL *conn = NULL;
        sprintf(query, "INSERT INTO `ChunkList` (`Parent_Key`, `Chunk_Key`, `File_Name`, `Order_Id`) VALUES (%d, ,%d ,'%s','%s');", Parent_Key, Chunk_Key, File_Name, Order_Id);
        conn = createMySQLConnection();
        if(conn != NULL)
        {
                rs = mysql_query(conn, query);
                closeMySQLConnection(conn);
                conn = NULL;
        }
	bool result = ( rs <= 0 ) ? false : true;
        return result;
}


/******************************************
 * Delete mentioned row from the table here
 ******************************************/

int DBManager :: deleteFromFileList(char* fileName)
{
	int rs = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	sprintf(query, "DELETE FROM `FileList` WHERE `FileName` = '%s';", fileName);
	conn = createMySQLConnection();
	if(conn != NULL)
	{
		rs = mysql_query(conn, query);
		closeMySQLConnection(conn);
		conn = NULL;
	}

	return rs;
}

/************************************************
 * Check the hash key already exist in table here
 ************************************************/

int DBManager :: showFileList()
{
	int rs = 0;
	char query[100] = {'\0'};
	MYSQL *conn = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(query, "SELECT * FROM FileList;");
	conn = createMySQLConnection();
	if(conn != NULL)
		rs = mysql_query(conn, query);
	if(!rs)
	{
		res = mysql_store_result(conn);
		while((row = mysql_fetch_row(res)) != NULL)
		{
			std::cout << "key :: " << row[0] << " " << "FileName :: " << row[1] << std::endl;
			closeMySQLConnection(conn);
			conn = NULL;
		}
	}
	else
	{
		closeMySQLConnection(conn);
		conn = NULL;
	}
	return rs;
}

/*********************************
 * Close database connection here
 *********************************/

void DBManager :: closeMySQLConnection(MYSQL * conn)
{
	if(conn)
		mysql_close(conn);
	conn = NULL;
}

/***********************************
 * Release all allocated memory here
 ***********************************/

int DBManager :: releaseMemory()
{
	int rs = 0;

	/*if(host)
	{
		free(host);
		host = NULL;
	}
	if(user)
	{
		free(user);
		user = NULL;
	}
	if(password)
	{
		free(password);
		password = NULL;
	}
	if(database)
	{
		free(database);
		database = NULL;
	}
	if(buff)
	{
		free(buff);
		buff = NULL;
	}*/
	return rs;
}
