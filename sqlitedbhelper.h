#ifndef __sqlitedbhelper_H__
#define __sqlitedbhelper_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "sqlitedbhelper"

#if !defined(PACKAGE)
#define PACKAGE "org.example.sqlitedbhelper"
#endif

/*this structure will be commonly used in both database and application layer*/
typedef struct
{
    int id;
    int type;
    char msg[MAX_LEN];
    char date[MAX_LEN];
} QueryData;

/*inset type, msg in the database. Date will be stored from system and id is autoincrement*/
int insertMsgIntoDb(int type, const char * msg_data);

/*fetch all stored message form database. This API will return total number of rows found in this call*/
int getAllMsgFromDb(QueryData **msg_data, int* num_of_rows);

/*fetch stored message form database based on given ID. Application needs to send desired ID*/
int getMsgById(QueryData **msg_data, int id);

/*delete stored message form database based on given ID. Application needs to send desired ID*/
int deleteMsgById(int id);

/*fetch all stored message form database*/
int deleteMsgAll();

/*count number of stored msg in the database and will return the total number*/
int getTotalMsgItemsCount(int* num_of_rows);

#endif /* __sqlitedbhelper_H__ */
