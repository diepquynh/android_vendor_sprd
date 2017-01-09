#ifndef __ENG_SQLITE_H__
#define __ENG_SQLITE_H__

#define ENG_ENGTEST_DB			"/productinfo/engtest.db"
#define ENG_STRING2INT_TABLE	"str2int"
#define ENG_STRING2STRING_TABLE	"str2str"
#define SPRDENG_SQL_LEN 128
#define ENG_SQLSTR2INT_ERR		0x7FFFFFFF
#define ENG_SQLSTR2STR_ERR		"NO VALUE"

typedef struct eng_str2int_sqlresult_t {
    char *name;
    int result;
}eng_str2int_sqlresult;

typedef struct eng_str2str_sqlresult_t {
    char *id;
    char result[128];
}eng_str2str_sqlresult;

typedef struct eng_str2int_table_t {
    int groupid;
    int value;
    char name[128];
}eng_str2int_table;

typedef struct eng_str2int_table_sqlresult_t {
    int count;
    eng_str2int_table table[64];
}eng_str2int_table_sqlresult;

int eng_sqlite_create(void);
int eng_sql_string2int_set(char* name, int value);
int eng_sql_string2int_get(char *name);
char* eng_sql_string2string_get(char *id);
int eng_sql_string2string_set(char* id, char* value);
int eng_sql_string2int_table_get(eng_str2int_table_sqlresult* result);
#endif
