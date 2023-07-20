#include "sqlite_helper.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>

int sqlite_init(const char *db_file, sqlite3 **db) {
  if (create_file_if_not_exists(db_file) != EXIT_SUCCESS) {
    syslog(LOG_ERR, "Error creating database file: %s", db_file);
    return SQLITE_CANTOPEN;
  }

  int rc = sqlite3_open_v2(db_file, db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to open database: %s", sqlite3_errmsg(*db));
    return rc;
  }

  const char *create_table_query =
      "CREATE TABLE IF NOT EXISTS events ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
      "topic TEXT NOT NULL,"
      "payload TEXT NOT NULL"
      ");";
  rc = sqlite3_exec(*db, create_table_query, 0, 0, 0);
  if (rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to create table: %s", sqlite3_errmsg(*db));
    sqlite3_close(*db);
    return rc;
  }

  return SQLITE_OK;
}

int sqlite_insert(sqlite3 *db, const char *payload, const char *topic) {
  sqlite3_stmt *stmt;
  const char *insert_query =
      "INSERT INTO events (topic, payload) VALUES (?, ?);";
  int rc = sqlite3_prepare_v2(db, insert_query, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to prepare insert statement: %s",
           sqlite3_errmsg(db));
    return rc;
  }

  rc = sqlite3_bind_text(stmt, 1, topic, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to bind topic parameter: %s", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

  rc = sqlite3_bind_text(stmt, 2, payload, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to bind payload parameter: %s", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    syslog(LOG_ERR, "Failed to execute insert statement: %s",
           sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
  }

  sqlite3_finalize(stmt);
  return SQLITE_OK;
}

void sqlite_deinit(sqlite3 *db) { sqlite3_close(db); }
