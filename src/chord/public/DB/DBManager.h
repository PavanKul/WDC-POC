/*
 * DBManager.h
 *
 *  Created on: Jan 27, 2023
 *      Author: Priyashree Guha Roy
 */

#ifndef DBMANAGER_H_
#define DBMANAGER_H_

#include <iostream>
#include <stdlib.h>
#include <cstdint>
#include <sys/types.h>
#include <mysql/mysql.h>

#define MAX_CONFIG_PARAM_LEN				50

class DBManager
{
private:
	char *host;
	char *user;
	char *password;
	char *database;
	char *buff;
	int port;

	MYSQL* createMySQLConnection();
	void closeMySQLConnection(MYSQL *conn);

public:
	DBManager();
	~DBManager();
	int initDBManager();
	int releaseMemory();
	int ifFilePresent(char* fileName, uint& key);
	int ifHashPresent(uint hash);
	int insertToFileList(char* fileName, uint hash);
	int deleteFromFileList(char* fileName);
	int showFileList();
};


#endif /* DBMANAGER_H_ */
