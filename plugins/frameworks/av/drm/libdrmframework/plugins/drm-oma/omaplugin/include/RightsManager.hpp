#ifndef __RIGHTS_MANAGER_H__
#define __RIGHTS_MANAGER_H__

#include <drm/DrmConstraints.h>
#include <RightsParser.hpp>
#include <common.hpp>
#include <sqlite3.h>

namespace android {
    class RightsManager {

    public:
        RightsManager();
        virtual ~RightsManager();

    private:
        bool insertOrUpdate(RightsParser&);
        bool remove(const String8& cid);
        sqlite3* openDatabase();
        RightsParser makeFakeFLRights();

        sqlite3* db;

    public:
        String8 getFakeKey();
        RightsParser query(const String8& cid);
        DrmConstraints* getConstraints(String8& cid, int action);
        status_t saveRights(RightsParser&);
        int checkRightsStatus(String8& cid, int action = Action::DEFAULT);
        String8 getFakeUid();
    };
};
#endif // __RIGHTS_MANAGER_H__
