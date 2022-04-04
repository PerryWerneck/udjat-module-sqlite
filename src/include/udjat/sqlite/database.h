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
 #include <mutex>

 namespace Udjat {

	namespace SQLite {

		class Statement;

		/// @brief SQLite database.
		class UDJAT_API Database {
		private:
			friend class Statement;

			sqlite3 *db = NULL;
			std::mutex guard;

			void check(int rc);

		protected:
			static Database * instance;

			void open(const char *dbname);
			void close();

		public:
			Database();
			~Database();

			static Database & getInstance();

			void exec(const char *sql);

			sqlite3_stmt * prepare(const char *sql);

		};

	}
 }

