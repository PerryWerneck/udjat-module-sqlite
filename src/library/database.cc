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
 #include <udjat/defs.h>
 #include <udjat/sqlite/database.h>
 #include <iostream>

 using namespace std;

 namespace Udjat {

	SQLite::Database * SQLite::Database::instance = nullptr;

	SQLite::Database::Database() {
		lock_guard<std::mutex> lock(guard);
		if(!instance) {
			instance = this;
		}
	}

	SQLite::Database::~Database() {
		{
			lock_guard<std::mutex> lock(guard);
			if(instance == this) {
				instance = nullptr;
			}
		}
		close();
	}

	SQLite::Database & SQLite::Database::getInstance() {
		if(!instance) {
			throw runtime_error("No available database instance");
		}
		return *instance;
	}

	void SQLite::Database::open(const char *dbname) {

		lock_guard<std::mutex> lock(guard);
		if(db) {
			throw runtime_error("Database is already open");
		}

		cout << "sqlite\tUsing database on '" << dbname << "'" << endl;

		// Open database.
		int rc = sqlite3_open(dbname, &db);
		if(rc != SQLITE_OK) {
			throw runtime_error(string{"Error opening '"} + dbname + "'");
        }

	}

	void SQLite::Database::close() {

		lock_guard<std::mutex> lock(guard);
		if(db) {
			switch(sqlite3_close(db)) {
			case SQLITE_OK:
					break;

			case SQLITE_BUSY:
					cerr << "sqlite\tClosing database with unfinished operations" << endl;
					break;

			default:
					cerr << "sqlite\tUnexpected error closing database" << endl;
			}

			db = nullptr;

		}

	}

	void SQLite::Database::exec(const char *sql) {

		char *errMsg = nullptr;

		if(!db) {
			throw runtime_error("Database is not available");
		}

		lock_guard<std::mutex> lock(guard);
		if(sqlite3_exec(db,sql,NULL,NULL,&errMsg) != SQLITE_OK) {
				string message{errMsg};
				sqlite3_free(errMsg);
				throw runtime_error(message);
		}

	}


 }

