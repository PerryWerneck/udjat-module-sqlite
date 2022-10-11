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
 #include <memory>
 #include <udjat/sqlite/database.h>

 namespace Udjat {

	namespace SQLite {

		class UDJAT_API SQL {
		protected:
			std::shared_ptr<Database> database;
			const char * query;
			const char * args;

		public:
			SQL(std::shared_ptr<Database> db, const char *query, const char *args);
			~SQL();

			/// @brief Execute query, no arguments.
			void exec() const;

		};

	}

 }
