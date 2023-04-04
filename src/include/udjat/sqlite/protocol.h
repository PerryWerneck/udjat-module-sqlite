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
 #include <udjat/tools/protocol.h>
 #include <list>
 #include <mutex>

 namespace Udjat {

	namespace SQLite {

		class UDJAT_API Protocol : public Udjat::Protocol {
		protected:

			std::shared_ptr<Database> database;

		private:
			const char *ins = nullptr;
			const char *del = nullptr;
			const char *select = nullptr;
			const char *list = nullptr;
			const char *pending = nullptr;

			bool busy = false;

			std::mutex guard;

			/// @brief Interval between URL send.
			time_t send_delay = 1;

			std::list<Abstract::Agent *> listeners;

		public:
			Protocol(std::shared_ptr<Database> db, const pugi::xml_node &node);
			virtual ~Protocol();

			/// @brief Send one queued URL.
			/// @return true if the first URL was sent.
			bool send() noexcept;

			/// @brief Count pending requests.
			int64_t count() const;

			/// @brief Insert listener agent.
			void insert(Abstract::Agent *listener);

			/// @brief Remove listener agent.
			void remove(Abstract::Agent *listener);

			/// @brief Refresh listeners.
			void refresh();

			/// @brief Get report.
			/// @param path The report path.
			/// @param report The report object to get the results.
			/// @retval true The report was found and processed.
			/// @retval false Report not found.
			bool get(const char *path, Report &report);

			/// @brief Get State based on queue size.
			std::shared_ptr<Abstract::State> state() const;

			std::shared_ptr<Protocol::Worker> WorkerFactory() const override;

		};

	}

 }
