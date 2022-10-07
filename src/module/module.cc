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
 #include <udjat/agent.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/application.h>
 #include <udjat/tools/object.h>
 #include <udjat/sqlite/database.h>
 #include <udjat/tools/quark.h>
 #include <udjat/tools/logger.h>
 #include <udjat/module.h>
 #include <udjat/sqlite/sql.h>

 using namespace std;

 namespace Udjat {

	/*
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
	*/

	/// @brief Create module from configuration file.
	const Udjat::ModuleInfo SQLite::Module::moduleinfo{"SQLite " SQLITE_VERSION " module"};

	SQLite::Module::Module() : Udjat::Module("sqlite",moduleinfo), Udjat::Factory("sql",moduleinfo) {

		// Open SQLite database
#ifdef DEBUG
		open(Application::DataFile(Config::Value<string>("sql","dbname","./sqlite.db").c_str(),true).c_str());
#else
		open(Application::DataFile(Config::Value<string>("sql","dbname","sqlite.db").c_str(),true).c_str());
#endif // DEBUG

	}

	/// @brief Create module from XML definition with fallback to configuration file.
	SQLite::Module::Module(const pugi::xml_node &node) : Udjat::Module("sqlite",moduleinfo), Udjat::Factory("sql",moduleinfo) {

		// Open SQLite database
		open(Application::DataFile(node,"dbname",true).c_str());

	}

	SQLite::Module::~Module() {
	}


	bool SQLite::Module::push_back(const XML::Node &node) {

		if(String{node,"type"} == "init") {
			//
			// Execute SQL on initialization.
			//
			String sql{node.child_value()};
			sql.strip();
			sql.expand(node);

			Database::getInstance().exec(sql.c_str());
			return true;

		}

		return false;

	}

	std::shared_ptr<Abstract::Agent> SQLite::Module::AgentFactory(const Abstract::Object &parent, const XML::Node &node) const {

		String type{node,"type"};

		if( type == "url-scheme" || type == "url-queue") {
			//
			// Register SQL as protocol handler and queue status agent.
			//
			auto protocol = make_shared<Protocol>(node);

			{
				SQLite::Module * module = const_cast<SQLite::Module *>(this);
				if(!module) {
					throw runtime_error("Cant cast module as volatile");
				}
				module->protocols.push_back(protocol);
			}

			//
			// @brief Agent interface for protocol.
			//
			class Agent : public Udjat::Agent<size_t> {
			private:
				shared_ptr<Protocol> protocol;

			public:
				Agent(shared_ptr<Protocol> p, const XML::Node &node) : Udjat::Agent<size_t>(node), protocol(p) {
				}

				void setup(const pugi::xml_node &node) override {

					Abstract::Agent::setup(node);

					auto seconds = timer();
					if(!seconds) {
						seconds = Config::Value<time_t>("sqlite","refresh-timer",600);
						warning() << "No update-timer, using default value of " << seconds << " seconds" << endl;
						timer(seconds);
					} else {
						info() << "Retry timer set to " << seconds << " seconds" << endl;
					}

				}

				void start() override {
					set(protocol->count());
				}

				bool refresh() override {
					if(protocol->send()) {
						set(protocol->count());
					}
					return true;
				}

				std::shared_ptr<Abstract::State> stateFromValue() const override {
					if(states.empty()) {
						return protocol->state();
					}
					return Udjat::Agent<size_t>::stateFromValue();
				}

			};

			return make_shared<Agent>(protocol,node);
		}

		return Udjat::Factory::AgentFactory(parent,node);

	}


 }
