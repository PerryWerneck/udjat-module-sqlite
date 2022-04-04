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

 #pragma once

 #include <udjat/defs.h>
 #include <sqlite3.h>
 #include <udjat/sqlite/database.h>
 #include <string>

 namespace Udjat {

	namespace SQLite {

		class UDJAT_API Statement {
		private:
			Database &database;
			sqlite3_stmt *stmt;

		public:
			Statement(const char *sql);
			~Statement();

			void reset();
			void exec();

			int step();

			void get(int column, int64_t &value);
			void get(int column, std::string &value);

			Statement & bind(int column, const char *value);
			Statement & bind(int column, const int64_t value);

			/// @brief Bind multiple columns.
			Statement & bind(const char *arg,...) UDJAT_GNUC_NULL_TERMINATED;

		};


	}

 }

