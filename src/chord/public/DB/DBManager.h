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
	bool insertIntoChunkList(uint& Parent_Key, uint& Chunk_Key, char* File_Name, char* order_Id);
	bool insertIntoToFileList(uint& Parent_Key, char* File_Name, int File_Size, int File_Copies);
	bool updatefilecopies(uint& Parent_Key, int File_Copies);
	int selectfilecopies(char* fileName);

	/*int updateHeartBeatBankDb(RegVerification *reg);
	int insertDataAlertDb(RegVerification *reg);
	int insertDataHealthDb(RegVerification *reg);
	int insertDataEventDb(evntData *data);
	int checkUUID(char *reg, char **uuid);
	int checkUUID(char *uuid);
	int checkRegistrationModeChanged(char *brID, int16 &regMode);
	int chnageRegModeOnSuccessfulRegistration(char *brID);
	int getConfigValueIsis(char *brId, int32 index, char *value);
	int getConfigValue(char *token, char *uuid, char *tableName, char *value, int16 index);
	int getConfigValue(char *token, char *uuid, char *tableName, int32 &value, int16 index);
	int getImageNameForMail(char *brId, Int64 timeStamp, int32 index, char *value);
	int getEmailIdForMail(char *brId, int32 index, char *value);
	int getPhoneNumberForMail(char *brId, int32 index, char *value);
	int getBranchName(char *brId, char *value);
	int updateImageDB(char *data, char *tableName);
	int updateImageEventDB(char *data);
	int getLastUpdatedHealth(char *brId, Int64 &timeStamp);
	int retrieveRegisteredBranch(char *brId, int16 index);
	int countRow(char *table, char *field, int32 &count);*/
};


#endif /* DBMANAGER_H_ */
