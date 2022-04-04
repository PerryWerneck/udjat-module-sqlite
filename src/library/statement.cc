/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #include <config.h>
 #include <udjat/sqlite/sql.h>
 #include <udjat/sqlite/statement.h>
 #include <iostream>
 #include <cstring>
 #include <cstdarg>

 using namespace std;

 namespace Udjat {

 	SQLite::Statement::Statement(const char *sql) : database(Database::getInstance()) {

 		if(!database.db) {
			throw runtime_error("Database is not available");
		}

		lock_guard<std::mutex> lock(database.guard);
		database.check(sqlite3_prepare_v2(
			database.db,		// Database handle
			sql,				// SQL statement, UTF-8 encoded
			-1,					// Maximum length of zSql in bytes.
			&stmt,				// OUT: Statement handle
			NULL				// OUT: Pointer to unused portion of zSql
		));

 	}

 	SQLite::Statement::~Statement() {
		lock_guard<std::mutex> lock(database.guard);
		sqlite3_finalize(stmt);
 	}

	void SQLite::Statement::reset() {
		lock_guard<std::mutex> lock(database.guard);
		sqlite3_reset(stmt);
	}

	int SQLite::Statement::step() {
		lock_guard<std::mutex> lock(database.guard);
		return sqlite3_step(stmt);
	}

	void SQLite::Statement::exec() {
		database.check(step());
	}

	void SQLite::Statement::get(int column, int64_t &value) {
		lock_guard<std::mutex> lock(database.guard);
		value = sqlite3_column_int64(stmt,column);
	}

	void SQLite::Statement::get(int column, string &value) {
		lock_guard<std::mutex> lock(database.guard);
		const char *str = (const char *) sqlite3_column_text(stmt,column);
		if(str)
			value = str;
		else
			value.clear();
	}

	SQLite::Statement & SQLite::Statement::bind(int column, const char *value) {
		lock_guard<std::mutex> lock(database.guard);
		database.check(
			sqlite3_bind_text(
				stmt,
				column,
				value,
				strlen(value)+1,
				SQLITE_TRANSIENT
			)	);
		return *this;
	}

	SQLite::Statement & SQLite::Statement::bind(int column, const int64_t value) {
		lock_guard<std::mutex> lock(database.guard);
		database.check(
			sqlite3_bind_int64(
				stmt,
				column,
				value
			)	);
		return *this;
	}

	SQLite::Statement & SQLite::Statement::bind(const char *arg,...) {

		size_t column = 0;

		va_list args;
		va_start(args, arg);
		while(arg) {

			if(sqlite3_bind_text(stmt,++column,arg,strlen(arg)+1,SQLITE_TRANSIENT) != SQLITE_OK) {
				va_end(args);
				throw runtime_error(sqlite3_errmsg(database.db));
			}

			arg = va_arg(args, const char *);
		}
		va_end(args);

		return *this;
	}

 }
