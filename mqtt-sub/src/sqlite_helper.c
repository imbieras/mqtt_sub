#include <sqlite3.h>
#include <stdlib.h>
#include <syslog.h>

int sqlite_init(const char *db_file, sqlite3 **db) {
  int rc = sqlite3_open_v2(db_file, db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to open or create database: %s",
           sqlite3_errmsg(*db));
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
