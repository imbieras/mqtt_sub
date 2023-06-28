#ifndef SQLITE_HELPER_H
#define SQLITE_HELPER_H

#include <sqlite3.h>

#define SQLITE_DATABASE_PATH "/var/mqtt_sub/logs.db"

int sqlite_init(const char *db_file, sqlite3 **db);
int sqlite_insert(sqlite3 *db, const char *payload, const char *topic);
void sqlite_deinit(sqlite3 *db);

#endif // SQLITE_HELPER_H
