/*
 * FILE:
 * opencog/persist/automap/ForeignStorage.h
 *
 * FUNCTION:
 * Foreign SQL interfaces.
 *
 * HISTORY:
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _ATOMSPACE_FOREIGN_STORAGE_H
#define _ATOMSPACE_FOREIGN_STORAGE_H

#include <atomic>
#include <map>
#include <mutex>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>

#include "llapi.h"

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class ForeignStorage : public StorageNode
{
	private:
		std::string _uri;

		// Pool of shared connections
		concurrent_stack<LLConnection*> conn_pool;
		int _initial_conn_pool_size;
		void enlarge_conn_pool(int, const char*);
		void close_conn_pool(void);

		// Utility for handling responses (on stack).
		class Response;

		bool _is_open;
		int _server_version;
		void get_server_version(void);

		// Stats for a given session.
		size_t _num_queries;
		size_t _num_tables;
		size_t _num_rows;

		// Loading of table definitions
		Handle load_one_table(const std::string&);
		Handle get_row_desc(const Handle&);
		std::string make_select(const Handle&);
		void load_selected_rows(const Handle&, const std::string&);
		void load_table_data(const Handle&);
		void load_column(const Handle&);
		void load_one_row(const Handle&, const Handle&, const Handle&);
		void load_join(const Handle&, const Handle&);
		void load_joined_rows(const Handle&);

	public:
		ForeignStorage(std::string uri);
		ForeignStorage(const ForeignStorage&) = delete; // disable copying
		ForeignStorage& operator=(const ForeignStorage&) = delete; // disable assignment
		virtual ~ForeignStorage();

		void open(void);
		void close(void);
		bool connected(void); // connection to DB is alive

		void create(void) {}
		void destroy(void) { kill_data(); /* TODO also delete the db */ }
		void erase(void) { kill_data(); }

		void kill_data(void) {} // destroy DB contents

		// AtomStorage interface
		void getAtom(const Handle&);
		Handle getLink(Type, const HandleSeq&);
		void fetchIncomingSet(AtomSpace*, const Handle&);
		void fetchIncomingByType(AtomSpace*, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(AtomSpace*, const Handle&, bool recursive);
		void storeValue(const Handle& atom, const Handle& key);
		void updateValue(const Handle&, const Handle&, const ValuePtr&);
		void loadValue(const Handle& atom, const Handle& key);
		void loadType(AtomSpace*, Type);
		void loadAtomSpace(AtomSpace*); // Load entire contents
		void storeAtomSpace(const AtomSpace*); // Store entire contents
		HandleSeq loadFrameDAG(void) {return HandleSeq(); }
		void storeFrameDAG(AtomSpace*) {}
		void deleteFrame(AtomSpace*) {}
		void barrier(AtomSpace* = nullptr);
		std::string monitor();

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.

		// Extra functions
		HandleSeq load_tables(void);
		Handle load_row(const Handle&, const Handle&, const Handle&);
};

class ForeignStorageNode : public ForeignStorage
{
	public:
		ForeignStorageNode(Type t, const std::string&& uri) :
			ForeignStorage(std::move(uri))
		{}
		ForeignStorageNode(const std::string&& uri) :
			ForeignStorage(std::move(uri))
		{}

		void setAtomSpace(AtomSpace* as)
		{
			// This is called with a null pointer when this
			// Atom is extracted from the AtomSpace.
			if (nullptr == as) close();
			Atom::setAtomSpace(as);
		}
		static Handle factory(const Handle&);
};

NODE_PTR_DECL(ForeignStorageNode)
#define createForeignStorageNode CREATE_DECL(ForeignStorageNode)

/** @}*/
} // namespace opencog

#endif // _ATOMSPACE_FOREIGN_STORAGE_H
