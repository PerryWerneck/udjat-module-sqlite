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
 #include "private.h"
 #include <pugixml.hpp>
 #include <udjat/tools/object.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/application.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/sqlite/database.h>
 #include <udjat/tools/quark.h>
 #include <udjat/module.h>
 #include <udjat/sqlite/sql.h>

 using namespace std;

 namespace Udjat {

	/// @brief Get attribute from configuration file.
	static String getAttribute(const char *name, const char *def) {
		return String{Config::Value<string>("sql",name,def)};
	}

	/// @brief Get attribute from xml with fallback to configuration file.
	static String getAttribute(const pugi::xml_node &node, const char *name, const char *def) {
		auto attribute = Object::getAttribute(node, name, false);
		if(attribute) {
			return String{attribute.as_string(def)}.expand(node);
		}
		return getAttribute(name,def).expand(node);
	}

	/// @brief Create module from configuration file.
	static const Udjat::ModuleInfo moduleinfo{"SQLite " SQLITE_VERSION " module"};

	SQLite::Module::Module() : Udjat::Module("sqlite",moduleinfo), Udjat::Factory("sql",moduleinfo) {

		// Open SQLite database
#ifdef DEBUG
		open(Application::DataFile(getAttribute("dbname","./sqlite.db").c_str()).c_str());
#else
		open(Application::DataFile(getAttribute("dbname","sqlite.db").c_str()).c_str());
#endif // DEBUG

	}

	/// @brief Create module from XML definition with fallback to configuration file.
	SQLite::Module::Module(const pugi::xml_node &node) : Udjat::Module("sqlite",moduleinfo), Udjat::Factory("sql",moduleinfo) {

		// Open SQLite database
		open(Application::DataFile(getAttribute(node,"dbname","sqlite.db").c_str()).c_str());

	}

	SQLite::Module::~Module() {
		for(auto worker : workers) {
			delete worker;
		}
	}

	void SQLite::Module::push_back(DynamicWorker *worker) const {
		const_cast<SQLite::Module *>(this)->workers.push_back(worker);
	}

	bool SQLite::Module::push_back(const pugi::xml_node &node) const {

		Database &database = Database::getInstance();

		String sql{node.child_value()};
		sql.strip();
		sql.expand(node);

#ifdef DEBUG
		cout << ":\n" << sql << endl;
#endif // DEBUG

		const char *type = node.attribute("type").as_string();
		if(!(type && *type)) {
			throw runtime_error("The required 'type' attribute is not available");
		}

		if(!strcasecmp(type,"init")) {
			//
			// Execute SQL on initialization.
			//
			database.exec(sql.c_str());
			return true;
		}

		if(!strcasecmp(type,"url-scheme")) {
			//
			// Register SQL as protocol handler.
			//
			class Scheme : public Udjat::Protocol, public SQL, public DynamicWorker {
			public:
				Scheme(const char *sql, const pugi::xml_node &node)
					: Protocol(Quark(node,"name","sql",false).c_str(),moduleinfo),SQL(sql,node.attribute("args").as_string()) {
				}

				std::shared_ptr<Worker> WorkerFactory() const override {
					throw runtime_error("Not implemented");
				}

			};

			push_back(new Scheme(sql.c_str(),node));
			return true;
		}


		return false;
	}

 }
