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

	bool SQLite::Module::generic(const XML::Node &node) {

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

				struct {
					time_t success = 2;		///< @brief How many seconds to wait after a successfull send.
					time_t failed = 14400;	///< @brief How many seconds to wait when failed.
					time_t empty = 14400;	///< @brief How many seconds to wait when queue is empty.
				} timers;

				struct {
					size_t count = 0;
					size_t max = 3;
					time_t timer = 1800;
				} retry;

			public:
				Agent(shared_ptr<Protocol> p, const XML::Node &node) : Udjat::Agent<unsigned int>(node), protocol(p) {
					protocol->insert(this);
				}

				virtual ~Agent() {
					protocol->remove(this);
				}

				void start() override {
					Udjat::Agent<unsigned int>::start(protocol->count());
				}

				void setup(const pugi::xml_node &node, bool upsearch) override {

					Abstract::Agent::setup(node,upsearch);

					retry.max = Object::getAttribute(node, "sqlite", "max-retries", (unsigned int) retry.max);
					retry.timer = Object::getAttribute(node, "sqlite", "retry-timer", (unsigned int) retry.timer);
					timers.success = Object::getAttribute(node, "sqlite", "wait-after-send", (unsigned int) timers.success);
					timers.empty = Object::getAttribute(node, "sqlite", "update-timer", (unsigned int) timers.empty);

					auto seconds = timer();
					if(!seconds) {
						seconds = Config::Value<time_t>("sqlite","refresh-timer",600);
						warning() << "No update-timer, using default value of " << seconds << " seconds" << endl;
						timer(seconds);
					} else {
						info() << "Retry timer set to " << seconds << " seconds" << endl;
					}

					timers.failed = Object::getAttribute(node, "sqlite", "wait-after-fail", (unsigned int) timers.empty);

				}

				void get(const Request UDJAT_UNUSED(&request), Report &report) override {
					protocol->get(report);
				}

				bool refresh() override {

					retry.count++;
					trace() << "Sending pending requests (" << retry.count << "/" << retry.max << ")" << endl;

					if(protocol->send()) {

						// Data was sent, if still have messages wait a few seconds.
						retry.count = 0;
						unsigned int count = protocol->count();
						set(count);
						if(count > 0) {
							sched_update(timers.success);
						} else {
							sched_update(timers.empty);
						}

					} else {

						// No data was sent, just set value and keep the original timer.
						set((unsigned int) protocol->count());

						if(retry.count >= retry.max) {

							trace() << "Reach maximum number of retries, sleeping for " << timers.failed << " seconds" << endl;
							sched_update(timers.failed);
							retry.count = 0;

						} else {

							trace() << "Failed, will retry on " << retry.timer << " seconds" << endl;
							sched_update(retry.timer);

						}

					}

					return true;

				}

				std::shared_ptr<Abstract::State> computeState() override {

					unsigned int value = Udjat::Agent<unsigned int>::get();

					debug("value=",value);

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
