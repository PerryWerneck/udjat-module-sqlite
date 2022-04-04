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

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/module.h>
 #include <udjat/factory.h>
 #include <udjat/sqlite/database.h>
 #include <udjat/sqlite/sql.h>
 #include <udjat/tools/protocol.h>
 #include <string>
 #include <list>
 #include <udjat/moduleinfo.h>

 namespace Udjat {

	namespace SQLite {

		class UDJAT_PRIVATE Module : public Udjat::Module, public Udjat::Factory, private Database {
		public:

			static const ModuleInfo moduleinfo;

			class DynamicWorker {
			public:
				constexpr DynamicWorker() {
				}

				virtual ~DynamicWorker() {
				}

			};

		private:

			std::list<DynamicWorker *> workers;
			void push_back(DynamicWorker *worker) const;

		public:
			Module();
			Module(const pugi::xml_node &node);
			virtual ~Module();

			bool push_back(const pugi::xml_node &node) const override;
		};

		class UDJAT_PRIVATE Protocol : public Udjat::Protocol, public Module::DynamicWorker {
		private:
			const char *ins;
			const char *del;
			const char *select;

		public:
			Protocol(const pugi::xml_node &node);
			virtual ~Protocol();

			std::shared_ptr<Protocol::Worker> WorkerFactory() const override;

		};


	}


 }
