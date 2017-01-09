#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "eng_sqlite.h"
#include "sqlite3.h"
#include "engopt.h"

static int str2intcallback;
static int str2strcallback;
static int str2inttablecallback;

static int eng_str2intcallback(void* param,int argc,char** argv,char** cname)
{
    int i;
    eng_str2int_sqlresult *sqlresult = (eng_str2int_sqlresult *)param;
    for(i=0;i<argc;i++){
        if(strcmp("value", cname[i])==0){
            str2intcallback = 1;
            ENG_LOG("%s: value=%s\n",__FUNCTION__, argv[i]);
            sqlresult->result = atoi(argv[i]);
        }
    }
    return 0;
}

static int eng_str2strcallback(void* param,int argc,char** argv,char** cname)
{
    int i;
    eng_str2str_sqlresult *sqlresult = (eng_str2str_sqlresult *)param;
    for(i=0;i<argc;i++){
        if(strcmp("value", cname[i])==0){
            str2strcallback = 1;
            ENG_LOG("%s: result=%s\n",__FUNCTION__, argv[i]);
            memset(sqlresult->result, 0, sizeof(sqlresult->result));
            memcpy(sqlresult->result, argv[i], strlen(argv[i])+1);
        }
    }
    return 0;
}

static int eng_str2inttablecallback(void* param,int argc,char** argv,char** cname)
{
    int i, finished = 0;
    eng_str2int_table_sqlresult *sqlresult = (eng_str2int_table_sqlresult *)param;

    for(i=0;i<argc;i++){
        if(strcmp("value", cname[i])==0){
            ENG_LOG("%s: value=%s\n",__FUNCTION__, argv[i]);
            finished |= 0x1;
            sqlresult->table[sqlresult->count].value = atoi(argv[i]);
        }

        if(strcmp("name", cname[i])==0){
            ENG_LOG("%s: name=%s\n",__FUNCTION__, argv[i]);
            finished |= 0x10;
            memcpy(sqlresult->table[sqlresult->count].name, argv[i], strlen(argv[i]));
        }

        if(strcmp("groupid", cname[i])==0){
            ENG_LOG("%s: groupid=%s\n",__FUNCTION__, argv[i]);
            finished |= 0x100;
            sqlresult->table[sqlresult->count].groupid = atoi(argv[i]);
        }

        if(0x111 == finished){
            sqlresult->count ++;
            str2inttablecallback = 1;
        }
    }

    ENG_LOG("%s: sqlresult->count: %d\n", __FUNCTION__, sqlresult->count);
    return 0;
}

int eng_sqlite_create(void)
{
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sql_createtable[SPRDENG_SQL_LEN];
    int rc,ret=0;

    //create db
    rc = sqlite3_open(ENG_ENGTEST_DB, &db);

    if(rc != 0) {
        ENG_LOG("%s: open %s fail [%d:%s]\n",__FUNCTION__, ENG_ENGTEST_DB, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = -1;
        goto out;
    } else {
        ENG_LOG("%s: open %s success\n",__FUNCTION__, ENG_ENGTEST_DB);
    }

    //create str2int table
    memset(sql_createtable, 0, SPRDENG_SQL_LEN);
    sprintf(sql_createtable, "CREATE TABLE %s(id INTEGER PRIMARY KEY, groupid INTEGER,name VARCHAR(32),value INTEGER);",ENG_STRING2INT_TABLE);

    rc = sqlite3_exec(db, sql_createtable, NULL, NULL, &errmsg);
    if(rc==1) {
        ENG_LOG("%s: %s already exists\n",__FUNCTION__, ENG_STRING2INT_TABLE);
    } else if(rc != 0) {
        ENG_LOG("%s: create table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = -1;
        goto out;
    } else {
        ENG_LOG("%s: create table %s success\n",__FUNCTION__, ENG_STRING2INT_TABLE);
    }

    //create str2str table
    memset(sql_createtable, 0, SPRDENG_SQL_LEN);
    sprintf(sql_createtable, "CREATE TABLE %s(id VARCHAR(32) PRIMARY KEY,value VARCHAR(32));",ENG_STRING2STRING_TABLE);

    rc = sqlite3_exec(db, sql_createtable, NULL, NULL, &errmsg);
    if(rc==1) {
        ENG_LOG("%s: %s already exists\n",__FUNCTION__, ENG_STRING2STRING_TABLE);
    } else if(rc != 0) {
        ENG_LOG("%s: create table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = -1;
        goto out;
    } else {
        ENG_LOG("%s: create table %s success\n",__FUNCTION__, ENG_STRING2STRING_TABLE);
    }

    memset(sql_createtable, 0, sizeof(sql_createtable));
    sprintf(sql_createtable, "chmod 777 %s",ENG_ENGTEST_DB);
    system(sql_createtable);
    sync();

out:
    sqlite3_close(db);
    return ret;
}

/*
 *there are two items in table string2int,
 *string item is primary key,
 *int item is value
 */
int eng_sql_string2int_set(char* name, int value)
{
    char ** result;
    int rownum;
    int colnum;
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sqlbuf[SPRDENG_SQL_LEN];
    int rc, ret=0;

    ENG_LOG("%s: name=%s; value=%d\n",__FUNCTION__, name, value);

    //open db
    rc = sqlite3_open(ENG_ENGTEST_DB, &db);
    if(rc != 0) {
        ENG_LOG("%s: open %s fail [%d:%s]\n",__FUNCTION__, ENG_ENGTEST_DB, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = -1;
        goto out;
    } else {
        ENG_LOG("%s: open %s success\n",__FUNCTION__, ENG_ENGTEST_DB);
    }

    //check if item exists
    memset(sqlbuf, 0, SPRDENG_SQL_LEN);
    sprintf(sqlbuf, "SELECT * FROM %s WHERE name='%s'",ENG_STRING2INT_TABLE, name);
    rc = sqlite3_get_table(db, sqlbuf, &result, &rownum, &colnum, &errmsg);
    sqlite3_free_table(result);

    ENG_LOG("%s: rownum=%d\n",__FUNCTION__, rownum);

    if(rownum > 0) { //update item
        memset(sqlbuf, 0, SPRDENG_SQL_LEN);
        sprintf(sqlbuf, "UPDATE %s SET value=%d where name='%s';",ENG_STRING2INT_TABLE, value, name);
        ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

        rc = sqlite3_exec(db, sqlbuf, NULL, NULL, &errmsg);
        if(rc != 0) {
            ENG_LOG("%s: update table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                    sqlite3_errcode(db), sqlite3_errmsg(db));
            ret = -1;
            goto out;
        } else {
            ENG_LOG("%s: update table %s success\n",__FUNCTION__,ENG_STRING2INT_TABLE);
        }
    } else { //insert item
        memset(sqlbuf, 0, SPRDENG_SQL_LEN);
        sprintf(sqlbuf, "INSERT INTO %s VALUES(1,0,'%s',%d);", ENG_STRING2INT_TABLE, name, value);
        ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

        rc = sqlite3_exec(db, sqlbuf, NULL, NULL, &errmsg);
        if(rc != 0) {
            ENG_LOG("%s: insert table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                    sqlite3_errcode(db), sqlite3_errmsg(db));
            ret = -1;
            goto out;
        } else {
            ENG_LOG("%s: insert table %s success\n",__FUNCTION__, ENG_STRING2INT_TABLE);
        }
    }

out:
    sqlite3_close(db);
    return ret;

}

int eng_sql_string2int_get(char *name)
{
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sqlbuf[SPRDENG_SQL_LEN];
    eng_str2int_sqlresult sqlresult;
    int rc, ret=0;

    ENG_LOG("%s: name=%s;\n",__FUNCTION__, name);

    memset(&sqlresult, 0, sizeof(sqlresult));
    sqlresult.name = name;
    sqlresult.result = 0;

    rc = sqlite3_open(ENG_ENGTEST_DB, &db);
    if(rc != 0) {
        ENG_LOG("%s: open %s fail [%d:%s]\n",__FUNCTION__, ENG_ENGTEST_DB, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = ENG_SQLSTR2INT_ERR;
        goto out;
    } else {
        ENG_LOG("%s: open %s success\n",__FUNCTION__, ENG_ENGTEST_DB);
    }

    memset(sqlbuf, 0, SPRDENG_SQL_LEN);
    sprintf(sqlbuf, "SELECT * FROM %s WHERE name='%s'",ENG_STRING2INT_TABLE, name);
    ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

    str2intcallback = 0;
    rc=sqlite3_exec(db,sqlbuf,&eng_str2intcallback,&sqlresult,&errmsg);
    if(rc != 0) {
        ENG_LOG("%s: select table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = ENG_SQLSTR2INT_ERR;
        goto out;
    }
    if(str2intcallback==1)
        ret = sqlresult.result;
    else
        ret = ENG_SQLSTR2INT_ERR;

    ENG_LOG("%s: select table %s success, ret=0x%x\n",__FUNCTION__, ENG_STRING2INT_TABLE, ret);

out:
    sqlite3_close(db);
    return ret;

}

/*
 *there are two items in table string2string,
 *they are both strings, one is primary key,
 *the other is value
 */
int eng_sql_string2string_set(char* id, char* value)
{
    char ** result;
    int rownum;
    int colnum;
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sqlbuf[SPRDENG_SQL_LEN];
    int rc, ret=0;

    ENG_LOG("%s: id=%s; value=%s\n",__FUNCTION__, id, value);

    //open db
    rc = sqlite3_open(ENG_ENGTEST_DB, &db);
    if(rc != 0) {
        ENG_LOG("%s: open %s fail [%d:%s]\n",__FUNCTION__, ENG_ENGTEST_DB, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = -1;
        goto out;
    } else {
        ENG_LOG("%s: open %s success\n",__FUNCTION__, ENG_ENGTEST_DB);
    }

    //check if item exists
    memset(sqlbuf, 0, SPRDENG_SQL_LEN);
    sprintf(sqlbuf, "SELECT * FROM %s WHERE id='%s'",ENG_STRING2STRING_TABLE, id);
    rc = sqlite3_get_table(db, sqlbuf, &result, &rownum, &colnum, &errmsg);
    sqlite3_free_table(result);

    ENG_LOG("%s: rownum=%d\n",__FUNCTION__, rownum);

    if(rownum > 0) { //update item
        memset(sqlbuf, 0, SPRDENG_SQL_LEN);
        sprintf(sqlbuf, "UPDATE %s SET value='%s' where id='%s';", ENG_STRING2STRING_TABLE, value, id);
        ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

        rc = sqlite3_exec(db, sqlbuf, NULL, NULL, &errmsg);
        if(rc != 0) {
            ENG_LOG("%s: update table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                    sqlite3_errcode(db), sqlite3_errmsg(db));
            ret = -1;
            goto out;
        } else {
            ENG_LOG("%s: update table %s success\n",__FUNCTION__, ENG_STRING2STRING_TABLE);
        }
    } else { //insert item
        memset(sqlbuf, 0, SPRDENG_SQL_LEN);
        sprintf(sqlbuf, "INSERT INTO %s VALUES('%s','%s');",ENG_STRING2STRING_TABLE, id, value);
        ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

        rc = sqlite3_exec(db, sqlbuf, NULL, NULL, &errmsg);
        if(rc != 0) {
            ENG_LOG("%s: insert table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                    sqlite3_errcode(db), sqlite3_errmsg(db));
            ret = -1;
            goto out;
        } else {
            ENG_LOG("%s: insert table %s success\n",__FUNCTION__, ENG_STRING2STRING_TABLE);
        }
    }

out:
    sqlite3_close(db);
    return ret;

}

char* eng_sql_string2string_get(char *id)
{
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char *ret=NULL;
    char sqlbuf[SPRDENG_SQL_LEN];
    eng_str2str_sqlresult sqlresult;
    int rc;

    ENG_LOG("%s: id=%s;\n",__FUNCTION__, id);

    memset(&sqlresult, 0, sizeof(sqlresult));
    sqlresult.id = id;
    memset(sqlresult.result, 0, sizeof(sqlresult.result));

    rc = sqlite3_open(ENG_ENGTEST_DB, &db);
    if(rc != 0) {
        ENG_LOG("%s: open %s fail [%d:%s]\n",__FUNCTION__, ENG_ENGTEST_DB, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = ENG_SQLSTR2STR_ERR;
        goto out;
    } else {
        ENG_LOG("%s: open %s success\n",__FUNCTION__, ENG_ENGTEST_DB);
    }

    memset(sqlbuf, 0, SPRDENG_SQL_LEN);
    sprintf(sqlbuf, "SELECT * FROM %s WHERE id='%s'",ENG_STRING2STRING_TABLE, id);
    ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

    str2strcallback = 0;
    rc=sqlite3_exec(db,sqlbuf,&eng_str2strcallback,&sqlresult,&errmsg);
    if(rc != 0) {
        ENG_LOG("%s: select table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = ENG_SQLSTR2STR_ERR;
        goto out;
    }

    if(str2strcallback == 1)
        ret = sqlresult.result;
    else
        ret = ENG_SQLSTR2STR_ERR;

    ENG_LOG("%s: select table %s success, result is %s\n",__FUNCTION__, ENG_STRING2STRING_TABLE, ret);

out:
    sqlite3_close(db);
    return ret;

}

int eng_sql_string2int_table_get(eng_str2int_table_sqlresult* result)
{
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sqlbuf[SPRDENG_SQL_LEN];
    int rc, ret=0;

    memset(result, 0, sizeof(eng_str2int_table_sqlresult));

    rc = sqlite3_open(ENG_ENGTEST_DB, &db);
    if(rc != 0) {
        ENG_LOG("%s: open %s fail [%d:%s]\n",__FUNCTION__, ENG_ENGTEST_DB, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = ENG_SQLSTR2INT_ERR;
        goto out;
    } else {
        ENG_LOG("%s: open %s success\n",__FUNCTION__, ENG_ENGTEST_DB);
    }

    memset(sqlbuf, 0, SPRDENG_SQL_LEN);
    sprintf(sqlbuf, "SELECT * FROM %s",ENG_STRING2INT_TABLE);
    ENG_LOG("%s: sqlbuf=%s\n",__FUNCTION__, sqlbuf);

    str2inttablecallback = 0;
    rc=sqlite3_exec(db,sqlbuf,&eng_str2inttablecallback,result,&errmsg);
    if(rc != 0) {
        ENG_LOG("%s: select table fail, errmsg=%s [%d:%s]\n",__FUNCTION__, errmsg, \
                sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = ENG_SQLSTR2INT_ERR;
        goto out;
    }

    if(!str2inttablecallback)
        ret = ENG_SQLSTR2INT_ERR;

    ENG_LOG("%s: select table %s success, ret=0x%x\n",__FUNCTION__, ENG_STRING2INT_TABLE, ret);

out:
    sqlite3_close(db);
    return ret;
}
