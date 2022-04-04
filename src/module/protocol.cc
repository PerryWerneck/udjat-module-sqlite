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
 #include <udjat/tools/quark.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/sqlite/database.h>
 #include <udjat/sqlite/statement.h>
 #include <string>
 #include "private.h"

 using namespace std;

 namespace Udjat {

	static const char * child_value(const pugi::xml_node &node, const char *name) {
		auto child = node.child(name);
		if(!child) {
			throw runtime_error(string{"Required child '"} + name + "' not found");
		}
		String sql{child.child_value()};
		sql.strip();
		sql.expand(node);

		return Quark(sql).c_str();
	}

	SQLite::Protocol::Protocol(const pugi::xml_node &node) : Udjat::Protocol(Quark(node,"name","sql",false).c_str(),SQLite::Module::moduleinfo), ins(child_value(node,"insert")), del(child_value(node,"delete")), select(child_value(node,"select")) {

		for(pugi::xml_node child = node.child("init"); child; child = child.next_sibling("init")) {

			String sql{child.child_value()};
			sql.strip();
			sql.expand(child);

#ifdef DEBUG
			cout << sql << endl;
#endif // DEBUG

			Database::getInstance().exec(sql.c_str());

		}

	}

	SQLite::Protocol::~Protocol() {
	}

	std::shared_ptr<Protocol::Worker> SQLite::Protocol::WorkerFactory() const {

		class Worker : public Udjat::Protocol::Worker {
		private:
			const char *sql;

		public:
			Worker(const char *s) : sql(s) {
			}

			virtual ~Worker() {
			}

			String get(const std::function<bool(double current, double total)> &progress) override {

#ifdef DEBUG
				cout << "Inserting " << method() << " '" << url() << "'" << endl;
#endif // DEBUG

				progress(0,0);

				// Get SQL
				String sql{this->sql};
				sql.expand(true,true);

				// Prepare.
				Statement stmt(sql.c_str());

				// Arguments: URL, VERB, Payload
				stmt.bind(
					url().c_str(),
					std::to_string(method()),
					payload(),
					nullptr
				);

				// Force as complete.
				progress(1,1);
				return "";
			}

		};

		return make_shared<Worker>(ins);
	}


 }


