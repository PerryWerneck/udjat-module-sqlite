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
 #include <udjat/sqlite/protocol.h>
 #include <udjat/agent.h>
 #include <string>
 #include <list>
 #include <vector>
 #include <udjat/moduleinfo.h>

 namespace Udjat {

	namespace SQLite {

		class UDJAT_PRIVATE Module : public Udjat::Module, public Udjat::Factory, private Database {
		public:

			static const ModuleInfo moduleinfo;

			// List of active protocols.
			std::vector<std::shared_ptr<Protocol>> protocols;

		public:
			Module();
			Module(const pugi::xml_node &node);
			virtual ~Module();

			std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object &parent, const XML::Node &node) const;

			bool push_back(const pugi::xml_node &node) override;

		};


	}


 }
