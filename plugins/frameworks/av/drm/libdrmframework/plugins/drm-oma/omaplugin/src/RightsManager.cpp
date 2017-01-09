#include <RightsManager.hpp>
#include <UUID.hpp>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <utils/Log.h>
#include <common.hpp>

#define SUNWAY_NOISY(x) x

using namespace android;

static String8 sFakeUid;
static String8 sFakeKey;

RightsManager::RightsManager() {
    db = openDatabase();

    sqlite3_stmt *stmt;
    const char * tail;

    int rc = sqlite3_prepare_v2(db, "select * from rights where uid like \"sprd_fl_uid_%\"", -1, &stmt, &tail);
    if (rc != SQLITE_OK) {
        SUNWAY_NOISY(ALOGE("RightsManager::RightsManager: failed: %s", sqlite3_errmsg(db)));
        return;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        RightsParser fakeFlRights = makeFakeFLRights();
        insertOrUpdate(fakeFlRights);
    }
    sqlite3_finalize(stmt);
#if 0
    RightsParser fakeFlRights = query(String8(FAKE_UID));
    if (! fakeFlRights.parse()) {
        fakeFlRights = makeFakeFLRights();
        insertOrUpdate(fakeFlRights);
    }
#endif
}

RightsManager::~RightsManager() {
    if (db) {
        sqlite3_close(db);
    }
}

String8 RightsManager::getFakeKey() {
    if (sFakeKey.length()) return sFakeKey;
        SUNWAY_NOISY(ALOGD("RightsManager::getFakeKey: sFakeKey is null, try 2 query db"));

    RightsParser flRights = query(getFakeUid());
    if (flRights.parse()) {
        sFakeKey = flRights.key;
        return sFakeKey;
    } else {
        return (String8)FAKE_KEY;
    }
}

String8 RightsManager::getFakeUid() {
    if (sFakeUid.length()) return sFakeUid;
        SUNWAY_NOISY(ALOGD("RightsManager::getFakeUid: sFakeUid is null, try 2 query db"));

    if (!db) {
        SUNWAY_NOISY(ALOGD("RightsManager::getFakeUid: db is null"));
        return sFakeUid;
    }

    sqlite3_stmt *stmt;
    const char * tail;

    int rc = sqlite3_prepare_v2(db, "select * from rights where uid like \"sprd_fl_uid_%\"", -1, &stmt, &tail);

    if (rc != SQLITE_OK) {
        SUNWAY_NOISY(ALOGD("RightsManager::getFakeUid: failed: %s", sqlite3_errmsg(db)));
        return sFakeUid;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
        sFakeUid = String8((char*)sqlite3_column_text(stmt, 2));
    sqlite3_finalize(stmt);
    return sFakeUid;
}

RightsParser RightsManager::makeFakeFLRights() {
    // set `constraints` for FL `fake` rights
    RightsParser ret = RightsParser(RightsParser::OK);
    ret.version=String8("1.0");
    String8 uid(FL_PREFIX);
    uid.append(UUID::generate());
    ret.uid = uid;
//    ret.uid=String8(FAKE_UID);
    ret.key=String8(UUID::generate());
    // for FL, TRANSFER is not allowed
    for (int i=0; i<ACTION_NUM; ++i) {
        for (int j=0; j<CONSTRAINT_NUM; ++j) {
            if (i == Action::TRANSFER) {
                ret.constraints[i][j] = RightsParser::EXPIRED;
            } else {
                ret.constraints[i][j] = RightsParser::INFINITY;
            }
        }
    }
    return ret;
}

DrmConstraints* RightsManager::getConstraints(String8& uid, int action) {
    if (! db) {
        return new DrmConstraints();
    }
    RightsParser parser = query(uid);
    if (! parser.parse()) {
        return new DrmConstraints();
    }
    return parser.getConstraintsForAction(action);
}

status_t RightsManager::saveRights(RightsParser& parser) {
    bool ret = insertOrUpdate(parser);
    if (ret) {
        return DRM_NO_ERROR;
    } else {
        return DRM_ERROR_CANNOT_HANDLE;
    }
}

int RightsManager::checkRightsStatus(String8& uid, int action) {
    if (! db || -1 == action) {
        return RightsStatus::RIGHTS_INVALID;
    }
    RightsParser parser = query(uid);
    if (! parser.parse()) {
        return RightsStatus::RIGHTS_INVALID;
    }
    if (parser.state == RightsParser::NOT_ACQUIRED) {
        return RightsStatus::RIGHTS_NOT_ACQUIRED;
    }

    if (action == Action::DEFAULT) {
        if (parser.checkRightsStatusForAction(Action::DISPLAY) == RightsStatus::RIGHTS_VALID ||
            parser.checkRightsStatusForAction(Action::PLAY) == RightsStatus::RIGHTS_VALID) {
            return RightsStatus::RIGHTS_VALID;
        } else {
            return RightsStatus::RIGHTS_EXPIRED;
        }
    } else {
        return parser.checkRightsStatusForAction(action);
    }
}

RightsParser RightsManager::query(const String8& uid) {
    SUNWAY_NOISY(ALOGD("RightsManager::query for uid %s", uid.string()));
    RightsParser ret = RightsParser(RightsParser::ERROR);
    if (!db) {
        SUNWAY_NOISY(ALOGE("RightsManager::query: db is null"));
        return ret;
    }

    sqlite3_stmt *stmt;
    const char * tail;

    int rc = sqlite3_prepare_v2(db,"select * from rights where uid=?1", -1, &stmt, &tail);
    sqlite3_bind_text(stmt, 1, uid.string(), strlen(uid.string()), SQLITE_TRANSIENT);

    if (rc != SQLITE_OK) {
        SUNWAY_NOISY(ALOGE("RightsManager::query: failed: %s", sqlite3_errmsg(db)));
        return ret;
    }
    rc=sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        ret = RightsParser(RightsParser::OK);
        ret.version = String8 ((char*)sqlite3_column_text(stmt, 1));
        ret.uid = String8 ((char*)sqlite3_column_text(stmt, 2));
        ret.key = String8 ((char*)sqlite3_column_text(stmt, 3));
        SUNWAY_NOISY(ALOGD("RightsManager::query: version: %s, uid: %s, key: %s", ret.version.string(), ret.uid.string(), ret.key.string()));
        for (int i=0; i<ACTION_NUM; ++i) {
            for (int j=0; j<CONSTRAINT_NUM; ++j) {
                ret.constraints[i][j]=sqlite3_column_int64(stmt,4+i*CONSTRAINT_NUM+j);
            }
        }
        SUNWAY_NOISY(ALOGD("RightsManager::query: ok: %s", ret.uid.string()));
    } else {
        SUNWAY_NOISY(ALOGE("RightsManager::query: NOT_ACQUIRED, err code: %d", rc));
        ret = RightsParser(RightsParser::NOT_ACQUIRED);
    }
    sqlite3_finalize(stmt);
    return ret;
}

sqlite3* RightsManager::openDatabase() {
    sqlite3* db = NULL;
    int rc = sqlite3_open_v2(RIGHTS_DB, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,0);
    if (rc) {
        SUNWAY_NOISY(ALOGE("RightsManager::openDatabase: failed: %s", sqlite3_errmsg(db)));
        return NULL;
    }
    char* sql = "create table rights(\
           _id integer primary key, version text, uid text unique, key text,\
           c00 integer, c01 integer, c02 integer,c03 integer, c04 integer,\
           c10 integer, c11 integer, c12 integer,c13 integer, c14 integer,\
           c20 integer, c21 integer, c22 integer,c23 integer, c24 integer,\
           c30 integer, c31 integer, c32 integer,c33 integer, c34 integer,\
           c40 integer, c41 integer, c42 integer,c43 integer, c44 integer,\
           c50 integer, c51 integer, c52 integer,c53 integer, c54 integer,\
           c60 integer, c61 integer, c62 integer,c63 integer, c64 integer,\
           c70 integer, c71 integer, c72 integer,c73 integer, c74 integer\
          )";

    sqlite3_exec(db,sql,0,0,0);
    return db;
}

bool RightsManager::insertOrUpdate(RightsParser& parser) {
    if (!db) {
        return false;
    }
    bool ret = true;
    String8 sql = String8("insert or replace into rights values (");
    sql.appendFormat("null,'%s','%s','%s'",parser.version.string(), parser.uid.string(), parser.key.string());
    for (int i=0; i<ACTION_NUM; ++i) {
        for (int j=0; j<CONSTRAINT_NUM; ++j) {
            sql.appendFormat(",%lld", parser.constraints[i][j]);
        }
    }
    sql.append(")");
    SUNWAY_NOISY(ALOGD("RightsManager::insertOrUpdate: %s", sql.string()));
    char * zerr;
    int rc = sqlite3_exec(db,sql.string(),0,0,&zerr);
        SUNWAY_NOISY(ALOGD("RightsManager::insertOrUpdate: end"));
    if (rc != SQLITE_OK) {
        SUNWAY_NOISY(ALOGE("RightsManager::insertOrUpdate: sql writes failed!"));
        ret = false;
    }
    return ret;
}

bool RightsManager::remove(const String8& uid) {
    if (!db) {
        return false;
    }
    bool ret = true;
    String8 sql = String8::format("delete from rights where uid='%s'",uid.string());
    int rc = sqlite3_exec(db,sql.string(),0,0,0);
    if (rc != SQLITE_OK) {
        ret = false;
    }
    return ret;
}
