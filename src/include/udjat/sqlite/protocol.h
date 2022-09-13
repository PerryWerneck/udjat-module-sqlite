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

 namespace Udjat {

	namespace SQLite {

		class UDJAT_API Protocol : public Udjat::Protocol {
		private:

			int64_t value = 0;
			const char *ins = nullptr;
			const char *del = nullptr;
			const char *select = nullptr;
			const char *pending = nullptr;

			bool busy = false;

			/// @brief Interval between URL send.
			time_t send_delay = 1;

			/// @brief Sending queued URLs.
			void send() const;

		public:
			Protocol(const pugi::xml_node &node);
			virtual ~Protocol();

			/// @brief Try sending requests.
			bool retry();

			/// @brief Count pending requests.
			int64_t count() const;

			/// @brief Get State based on queue size.
			std::shared_ptr<Abstract::State> state() const;

			std::shared_ptr<Protocol::Worker> WorkerFactory() const override;

			/*
			inline std::ostream & info() const {
				return Object::NamedObject::info();
			}

			inline std::ostream & warning() const {
				return Object::NamedObject::warning();
			}

			inline std::ostream & error() const {
				return Object::NamedObject::error();
			}
			*/

		};

	}

 }
