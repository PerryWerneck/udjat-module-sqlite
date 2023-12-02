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
 #include <pugixml.hpp>
 #include <udjat/sqlite/sql.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/quark.h>
 #include <udjat/tools/http/client.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/sqlite/database.h>
 #include <udjat/sqlite/statement.h>
 #include <udjat/sqlite/protocol.h>
 #include <udjat/tools/mainloop.h>
 #include <udjat/tools/timestamp.h>
 #include <udjat/tools/systemservice.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/threadpool.h>
 #include <udjat/tools/intl.h>
 #include <string>

#ifndef _WIN32
	#include <unistd.h>
#endif // _WIN32

 using namespace std;

 namespace Udjat {

	static const char * child_value(const pugi::xml_node &node, const char *name, bool required = true) {
		auto child = node.child(name);
		if(!child) {
			if(required) {
				throw runtime_error(string{"Required child '"} + name + "' not found");
			}
			return "";
		}
		String sql{child.child_value()};
		sql.strip();
		sql.expand(node);

		return Quark(sql).c_str();
	}

	int64_t SQLite::Protocol::count() const {
		int64_t pending_messages = 0;
		if(pending && *pending) {
			Statement sql{database,pending};
			sql.step();
			sql.get(0,pending_messages);
		}
		return pending_messages;
	}

	static const Udjat::ModuleInfo moduleinfo{"SQLite " SQLITE_VERSION " custom protocol module"};

	SQLite::Protocol::Protocol(	std::shared_ptr<Database> db, const pugi::xml_node &node) :
		Udjat::Protocol{Quark(node,"name","sql",false).c_str(),moduleinfo},
		database{db},
		ins{child_value(node,"insert")},
		del{child_value(node,"delete")},
		select{child_value(node,"select")},
		list{child_value(node,"report",false)},
		pending{child_value(node,"pending",false)} {

		send_delay = Object::getAttribute(node, "sqlite", "retry-delay", (unsigned int) send_delay);

		for(pugi::xml_node child = node.child("init"); child; child = child.next_sibling("init")) {

			String sql{child.child_value()};
			sql.strip();
			sql.expand(child);

			debug(sql.c_str());

			database->exec(sql.c_str());

		}

	}

	SQLite::Protocol::~Protocol() {
		if(busy) {
			info() << "Waiting for workers" << endl;
			ThreadPool::getInstance().wait();
		}
		info() << "Disabling " << (busy ? "an active" : "inactive") << " protocol handler" << endl;
	}

	void SQLite::Protocol::insert(Abstract::Agent *listener) {
		lock_guard<mutex> lock(guard);
		listeners.push_back(listener);
	}

	void SQLite::Protocol::remove(Abstract::Agent *listener) {
		lock_guard<mutex> lock(guard);
		listeners.remove(listener);
	}

	void SQLite::Protocol::refresh() {
		time_t delay = (busy ? 60 : 0);

		lock_guard<mutex> lock(guard);
		for(auto listener : listeners) {
			listener->sched_update(delay);
		}

	}

	std::shared_ptr<Abstract::State> SQLite::Protocol::state() const {

		if(pending && *pending) {

			/// @brief Message based state.
			class StringState : public Abstract::State {
			private:
				std::string message;

			public:
				StringState(const char *name, Level level, const std::string &msg) : Abstract::State(name,level), message{msg} {
					Object::properties.summary = message.c_str();
				}
			};

			//
			// Create default states.
			//
			std::shared_ptr<Abstract::State> state;

			auto value = count();
			const char * name = Protocol::c_str();

			if(!value) {

				state = make_shared<StringState>(
								"empty",
								Level::unimportant,
								Message{ _("{} output queue is empty"), name }
							);

			} else if(value == 1) {

				state = make_shared<StringState>(
								"pending",
								Level::warning,
								Message{ _("One pending request in the {} queue"), name }
							);

			} else {

				state = make_shared<StringState>(
								"pending",
								Level::warning,
								Message{ _("{} pending requests in the {} queue"), value, name }
							);

			}

			info() << state->summary() << endl;

			return state;

		}

		return make_shared<Abstract::State>("none", Level::unimportant, _( "No pending requests") );
	}

	bool SQLite::Protocol::send() noexcept {

		size_t success = false;

		debug("start ", __FUNCTION__);

		static mutex guard;

		{
			lock_guard<mutex> lock(guard);
			if(busy) {
				debug("Worker is busy");
				return 0;
			}
			busy = true;
		}

		try {

			Statement del(database,this->del);
			Statement select(database,this->select);
			MainLoop &mainloop = MainLoop::getInstance();

			if(select.step() == SQLITE_ROW && mainloop && Protocol::verify(this)) {

				int64_t id;
				Udjat::URL url;
				string action, payload;

				select.get(0,id);
				select.get(1,url);
				select.get(2,action);
				select.get(3,payload);

				info() << "Sending " << action << " " << url << " (" << id << ")" << endl;
				Logger::write(Logger::Trace,Protocol::c_str(),payload.c_str());

				HTTP::Client client(url);

				switch(HTTP::MethodFactory(action.c_str())) {
				case HTTP::Get:
					{
						auto response = client.get();
						info() << url << endl;
						Logger::write(Logger::Trace,response);
						success = true;
					}
					break;

				case HTTP::Post:
					{
						auto response = client.post(payload.c_str());
						Logger::write(Logger::Trace,response);
						success = true;
					}
					break;

				default:
					error() << "Unexpected verb '" << action << "' sending queued request, ignoring" << endl;
					success = false;
				}

				info() << "Removing request '" << id << "' from URL queue" << endl;
				del.bind(1,id).exec();

				del.reset();
				select.reset();

			}

		} catch(const std::exception &e) {

			warning() << "Error sending queued message: " << e.what() << endl;
			success = false;

		} catch(...) {

			warning() << "Unexpected error sending queued messages" << endl;
			success = false;

		}

		{
			lock_guard<mutex> lock(guard);
			busy = false;
		}

		debug(__FUNCTION__," complete (", (success ? "Message sent" : "Message NOT sent"), ")");

		return success;
	}

	std::shared_ptr<Protocol::Worker> SQLite::Protocol::WorkerFactory() const {

		class Worker : public Udjat::Protocol::Worker {
		private:
			const char *sql;
			const Protocol *protocol = nullptr;

		public:
			Worker(const Protocol *p, const char *s) : sql(s), protocol(p) {
			}

			virtual ~Worker() {
			}

			String get(const std::function<bool(double current, double total)> &progress) override {

				progress(0,0);

				// Get SQL
				String sql{this->sql};
				sql.expand(true,true);

				// Prepare.
				Statement stmt(protocol->database,sql.c_str());

				// Arguments: URL, VERB, Payload
				stmt.bind(
					url().c_str(),
					std::to_string(method()),
					payload(),
					nullptr
				);

				stmt.exec();

				{
					Protocol *prot = const_cast<Protocol *>(this->protocol);
					if(prot) {
						prot->refresh();
					}
				}

				// Force as complete.
				progress(1,1);
				return "";
			}

		};

		return make_shared<Worker>(this,ins);
	}

	bool SQLite::Protocol::get(const char *path, Response::Table &report) {

		throw system_error(ENOTSUP,system_category(),_( "Not implemented" ));

	}

 }


