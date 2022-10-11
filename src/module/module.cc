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

	/// @brief Create module from configuration file.
	const Udjat::ModuleInfo SQLite::Module::moduleinfo{"SQLite " SQLITE_VERSION " module"};

	std::shared_ptr<SQLite::Database> DatabaseFactory() {
#ifdef DEBUG
		#define DBNAME "./sqlite.db"
#else
		#define DBNAME "sqlite.db"
#endif // DEBUG

		return make_shared<SQLite::Database>(Config::Value<string>("sql","dbname",DBNAME).c_str());

	}

	std::shared_ptr<SQLite::Database> DatabaseFactory(const pugi::xml_node &node) {
		return make_shared<SQLite::Database>(Application::DataFile(node,"dbname",true).c_str());
	}

	SQLite::Module::Module() : Udjat::Module("sqlite",moduleinfo), Udjat::Factory("sql",moduleinfo), database(DatabaseFactory()) {
	}

	/// @brief Create module from XML definition with fallback to configuration file.
	SQLite::Module::Module(const pugi::xml_node &node) : Udjat::Module("sqlite",moduleinfo), Udjat::Factory("sql",moduleinfo), database(DatabaseFactory(node)) {
	}

	SQLite::Module::~Module() {
		auto count = database.use_count();
		if(count > 2) {
			Udjat::Factory::warning() << "Closing module with " << count << " active database instance(s) " << endl;
		} else {
			Udjat::Factory::info() << "Closing module with " << count << " active database instance(s) " << endl;
		}
	}

	bool SQLite::Module::push_back(const XML::Node &node) {

		if(String{node,"type"} == "init") {
			//
			// Execute SQL on initialization.
			//
			String sql{node.child_value()};
			sql.strip();
			sql.expand(node);
			database->exec(sql.c_str());
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
			auto protocol = make_shared<Protocol>(database,node);

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
			class Agent : public Udjat::Agent<unsigned int> {
			private:
				shared_ptr<Protocol> protocol;

			public:
				Agent(shared_ptr<Protocol> p, const XML::Node &node) : Udjat::Agent<unsigned int>(node), protocol(p) {
					protocol->insert(this);
				}

				virtual ~Agent() {
					protocol->remove(this);
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

				void get(const Request UDJAT_UNUSED(&request), Report &report) override {
					protocol->get(report);
				}

				void start() override {
					set(protocol->count());
				}

				bool refresh() override {
					protocol->send();
					set((unsigned int) protocol->count());
					return true;
				}

				std::shared_ptr<Abstract::State> stateFromValue() const override {

					unsigned int value = Udjat::Agent<unsigned int>::get();

					for(auto state : states) {
						if(state->compare(value))
							return state;
					}

					return protocol->state();

				}

			};

			return make_shared<Agent>(protocol,node);
		}

		return Udjat::Factory::AgentFactory(parent,node);

	}


 }
