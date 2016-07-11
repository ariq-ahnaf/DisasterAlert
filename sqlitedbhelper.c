#include <string.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <storage.h>
#include <app_common.h>
#include <stdio.h>
#include "sqlitedbhelper.h"

#define DB_NAME "sample.db"
#define TABLE_NAME "SampleTable"
#define COL_ID "QR_ID"
#define COL_DATA "QR_DATA"
#define COL_TYPE "QR_CODE"
#define COL_DATE "QR_DATE"

#define BUFLEN 500 /*assume buffer length for query string's size.*/

sqlite3 *sampleDb; /*name of database*/
int select_row_count = 0;

/*open database instance*/
int opendb()
{
     char * dataPath = app_get_data_path(); /*fetched package path available physically in the device*/
	 int size = strlen(dataPath)+10;

	 char * path = malloc(sizeof(char)*size);

	 strcpy(path,dataPath);
	 strncat(path, DB_NAME, size);

	 DBG("DB Path = [%s]", path); /*prepared full path, database will be stored there*/

	 int ret = sqlite3_open_v2( path , &sampleDb, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE, NULL);
	 if(ret != SQLITE_OK)
		ERR("DB Create Error! [%s]", sqlite3_errmsg(sampleDb));

	 free(dataPath);
	 free(path);
         /*didn't close database instance as this will be handled by caller e.g. insert, delete*/
	 return ret;
}

int initdb()
{
	if (opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

   int ret;
   char *ErrMsg;
   /*query preparation for table creation. it will not be created the table if it is exists already*/
   char *sql = "CREATE TABLE IF NOT EXISTS "\
		    TABLE_NAME" ("  \
			COL_DATA" TEXT NOT NULL, " \
			COL_TYPE" INTEGER NOT NULL, " \
			COL_DATE" TEXT NOT NULL, " \
			COL_ID" INTEGER PRIMARY KEY AUTOINCREMENT);"; /*id autoincrement*/

   DBG("crate table query : %s", sql);

   ret = sqlite3_exec(sampleDb, sql, NULL, 0, &ErrMsg); /*execute query*/
   if(ret != SQLITE_OK)
   {
	   ERR("Table Create Error! [%s]", ErrMsg);
	   sqlite3_free(ErrMsg);
	   sqlite3_close(sampleDb); /*close db instance as instance is still open*/

	   return SQLITE_ERROR;
   }
   DBG("Db Table created successfully!");
   sqlite3_close(sampleDb); /*close the db instance as operation is done here*/

   return SQLITE_OK;
}

/*callback for insert operation*/
static int insertcb(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      /*usually we do not need to do anything.*/
   }
   return 0;
}

int insertMsgIntoDb(int type, const char * msg_data)
{
	if(opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

	char sqlbuff[BUFLEN];
	char *ErrMsg;
	int ret;
	/*read system date time using sqlite function*/
	char* dateTime = "strftime('%Y-%m-%d  %H-%M','now')";

        /*prepare query for INSERT operation*/
	snprintf(sqlbuff, BUFLEN, "INSERT INTO "\
			TABLE_NAME" VALUES(\'%s\', %d, %s, NULL);", /*didn't include id as it is autoincrement*/
	            		msg_data, type, dateTime);
	DBG("Insert query = [%s]", sqlbuff);

	ret = sqlite3_exec(sampleDb, sqlbuff, insertcb, 0, &ErrMsg); /*execute query*/
	if (ret != SQLITE_OK)
	{
	   ERR("Insertion Error! [%s]", sqlite3_errmsg(sampleDb));
	   sqlite3_free(ErrMsg);
	   sqlite3_close(sampleDb); /*close db instance for failed case*/

	   return SQLITE_ERROR;
	}

	sqlite3_close(sampleDb); /*close db instance for success case*/

	return SQLITE_OK;
}

QueryData *qrydata;

/*this callback will be called for each row fetched from database. we need to handle retrieved elements for each row manually and store data for further use*/
static int selectAllItemcb(void *data, int argc, char **argv, char **azColName){
        /*
        * SQLite queries return data in argv parameter as  character pointer */
        /*prepare a temporary structure*/
	QueryData *temp = (QueryData*)realloc(qrydata, ((select_row_count + 1) * sizeof(QueryData)));

	if(temp == NULL){
		ERR("Cannot reallocate memory for QueryData");
		return SQLITE_ERROR;
	} else {
                /*store data into temp structure*/
		strcpy(temp[select_row_count].msg, argv[0]);
		temp[select_row_count].type = atoi(argv[1]);
		strcpy(temp[select_row_count].date, argv[2]);
		temp[select_row_count].qr_id = atoi(argv[3]);

                /*copy temp structure into main sturct*/
		qrydata = temp;
	}

	select_row_count ++; /*keep row count*/

   return SQLITE_OK;
}

int getAllMsgFromDb(QueryData **msg_data, int* num_of_rows)
{
	if(opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

	qrydata = (QueryData *) calloc (1, sizeof(QueryData)); /*preparing local querydata struct*/

   char *sql = "SELECT * FROM QueryData ORDER BY QR_ID DESC"; /*select query*/
   int ret;
   char *ErrMsg;
   select_row_count = 0;

    ret = sqlite3_exec(sampleDb, sql, selectAllItemcb, (void*)msg_data, &ErrMsg);
	if (ret != SQLITE_OK)
	{
	   DBG("select query execution error [%s]", ErrMsg);
	   sqlite3_free(ErrMsg);
	   sqlite3_close(sampleDb); /*close db for failed case*/

	   return SQLITE_ERROR;
	}

        /*assign all retrived values into caller's pointer*/
	*msg_data = qrydata;
        *num_of_rows = select_row_count;

	DBG("select query execution success!");
	sqlite3_close(sampleDb); /*close db for success case*/

   return SQLITE_OK;
}

int getMsgById(QueryData **msg_data, int id)
{
	if(opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

	qrydata = (QueryData *) calloc (1, sizeof(QueryData));

   char sql[BUFLEN];
   snprintf(sql, BUFLEN, "SELECT * FROM QueryData where QR_ID=%d;", id);

   int ret = 0;
   char *ErrMsg;

    ret = sqlite3_exec(sampleDb, sql, selectAllItemcb, (void*)msg_data, &ErrMsg);
	if (ret != SQLITE_OK)
	{
	   DBG("select query execution error [%s]", ErrMsg);
	   sqlite3_free(ErrMsg);
	   sqlite3_close(sampleDb);

	   return SQLITE_ERROR;
	}

	DBG("select query execution success!");

        /*assign fetched data into caller's struct*/
	*msg_data = qrydata;

	sqlite3_close(sampleDb); /*close db*/

   return SQLITE_OK;
}

static int deletecb(void *data, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
	/*no need to do anything*/
   }

   return 0;
}

int deleteMsgById(int id)
{
	if(opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

   char sql[BUFLEN];
   snprintf(sql, BUFLEN, "DELETE from QueryData where QR_ID=%d;", id);

   int counter = 0, ret = 0;
   char *ErrMsg;

   ret = sqlite3_exec(sampleDb, sql, deletecb, &counter, &ErrMsg);
	if (ret != SQLITE_OK)
	{
		ERR("Delete Error! [%s]", sqlite3_errmsg(sampleDb));
	   sqlite3_free(ErrMsg);
	   sqlite3_close(sampleDb);

	   return SQLITE_ERROR;
	}

	sqlite3_close(sampleDb);

   return SQLITE_OK;
}

int deleteMsgAll()
{
	if(opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

   char sql[BUFLEN];
   snprintf(sql, BUFLEN, "DELETE from QueryData;");

   int counter = 0, ret = 0;
   char *ErrMsg;

   ret = sqlite3_exec(sampleDb, sql, deletecb, &counter, &ErrMsg);
	if (ret != SQLITE_OK)
	{
		ERR("Delete Error! [%s]", sqlite3_errmsg(sampleDb));
	   sqlite3_free(ErrMsg);
	   sqlite3_close(sampleDb);

	   return SQLITE_ERROR;
	}

	sqlite3_close(sampleDb);

   return SQLITE_OK;
}

int g_row_count = 0;

static int row_count_cb(void *data, int argc, char **argv, char **azColName)
{
	g_row_count = atoi(argv[0]); /*number of rows*/

	return 0;
}
int getTotalMsgItemsCount(int* num_of_rows)
{
	if(opendb() != SQLITE_OK) /*create database instance*/
		return SQLITE_ERROR;

   char *sql = "SELECT COUNT(*) FROM QueryData;";
   char *ErrMsg;

   int ret = 0;

   ret = sqlite3_exec(sampleDb, sql, row_count_cb, NULL, &ErrMsg);
	if (ret != SQLITE_OK)
	{
		ERR("Count Error! [%s]", sqlite3_errmsg(sampleDb));
	    sqlite3_free(ErrMsg);
	    sqlite3_close(sampleDb);

	    return SQLITE_ERROR;
	}

	DBG("Total row found[%d]", g_row_count);

	sqlite3_close(sampleDb);

	*num_of_rows = g_row_count;
	g_row_count = 0;
   return SQLITE_OK;
}

#include "sqlitedbhelper.h"


QueryData* msgdata;

static Eina_Bool
layout_pop_cb(void *data, Elm_Object_Item *it)
{
    if(msgdata)
	free(msgdata); /*need to free this structure in pop_cb of current layout*/
    return EINA_TRUE;
}

/*allocate msgdata memory. this will be used for retrieving data from database*/
msgdata = (QueryData*) calloc (1, sizeof(QueryData));

int num_of_rows = 0;

/*retrieve all msgdata from database*/
ret = getAllMsgFromDb(&msgdata, &num_of_rows); /*sending msgdata reference to get data from database; num_of_rows reference  to get count of rows*/
if(!ret){
	DBG("Retrieved [%d] rows successfully!", num_of_rows);
} else {
	ERR("Data retrieval error!");
}

/*get number of row count*/
ret = getHistoryItemsCount(&num_of_rows);
if(!ret){
	DBG("Total rows found: [%d]", num_of_rows);
} else {
	ERR("row count error!");
}

/*delete msg item from db by msg_id*/
ret = deleteMsgById(id);
if(ret){
	ERR("Data delete error!");
} else {
	DBG("Data delete success!");
}
