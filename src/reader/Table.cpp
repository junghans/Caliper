// Copyright (c) 2016, Lawrence Livermore National Security, LLC.  
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Caliper.
// Written by David Boehme, boehme3@llnl.gov.
// LLNL-CODE-678900
// All rights reserved.
//
// For details, see https://github.com/scalability-llnl/Caliper.
// Please also see the LICENSE file for our additional BSD notice.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the disclaimer below.
//  * Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the disclaimer (as noted below) in the documentation and/or other materials
//    provided with the distribution.
//  * Neither the name of the LLNS/LLNL nor the names of its contributors may be used to endorse
//    or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// LAWRENCE LIVERMORE NATIONAL SECURITY, LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// @file Table.cpp
/// Print human-readable table

#include "Table.h"

#include "CaliperMetadataDB.h"

#include "Attribute.h"
#include "ContextRecord.h"
#include "Node.h"

#include "util/split.hpp"

#include <algorithm>
#include <iterator>
#include <mutex>

using namespace cali;

struct Table::TableImpl
{
    struct Column {
        std::string name;
        std::size_t max_width;

        Attribute   attr;

        bool        print; // used for hidden sort columns
        
        Column(const std::string& n, std::size_t w, const Attribute& a, bool p)
            : name(n), max_width(w), attr(a), print(p)
            { }
    };
        
    std::vector<Column>                     m_cols;
    std::vector< std::vector<Variant> >     m_rows;

    std::mutex                              m_col_lock;
    std::mutex                              m_row_lock;
    
    bool                                    m_auto_column;
    std::size_t                             m_num_sort_columns;
    
    void parse(const std::string& field_string, const std::string& sort_string) {        
        std::vector<std::string> fields;

        // fill sort columns
        
        util::split(sort_string, ':', std::back_inserter(fields));

        for (const std::string& s : fields)
            if (s.size() > 0)
                m_cols.emplace_back(s, s.size(), Attribute::invalid, false);
        
        m_num_sort_columns = m_cols.size();
        fields.clear();
        
        // fill print columns

        if (field_string.empty()) {
            m_auto_column = true;
            return;
        } else
            m_auto_column = false;

        util::split(field_string, ':', std::back_inserter(fields));

        for (const std::string& s : fields)
            if (s.size() > 0)
                m_cols.emplace_back(s, s.size(), Attribute::invalid, true);
    }

    void update_column_attribute(CaliperMetadataDB& db, cali_id_t attr_id) {        
        auto it = std::find_if(m_cols.begin()+m_num_sort_columns, m_cols.end(),
                               [attr_id](const Column& c) {
                                   return c.attr.id() == attr_id;
                               });

        if (it != m_cols.end())
            return;
        
        Attribute attr   = db.attribute(attr_id);

        if (attr == Attribute::invalid)
            return;
        
        std::string name = attr.name();

        // Skip internal "cali." and ".event" attributes
        if (name.compare(0, 5, "cali." ) == 0 ||
            name.compare(0, 6, "event.") == 0)
            return;
                
        m_cols.emplace_back(name, name.size(), attr, true);
    }
    
    std::vector<Column> update_columns(CaliperMetadataDB& db, const EntryList& list) {
        std::lock_guard<std::mutex>
            g(m_col_lock);

        // Auto-generate columns from attributes in the snapshots. Used if no
        // field list was given. Skips some internal attributes.
        
        if (m_auto_column)
            for (Entry e : list) {
                if (e.node()) {
                    for (const Node* node = e.node(); node && node->attribute() != CALI_INV_ID; node = node->parent())
                        update_column_attribute(db, node->attribute());
                } else
                    update_column_attribute(db, e.attribute());
            }

        // Check if we can look up attribute object from name

        for (Column& col : m_cols)
            if (col.attr == Attribute::invalid)
                col.attr = db.attribute(col.name);

        return m_cols;
    }

    void add(CaliperMetadataDB& db, const EntryList& list) {
        std::vector<Column>  cols = update_columns(db, list);
        std::vector<Variant> row(cols.size());

        bool active = false;
        bool update_max_width = false;
        
        for (std::vector<Column>::size_type c = 0; c < cols.size(); ++c) {
            if (cols[c].attr == Attribute::invalid)
                continue;

            Variant val;
            std::size_t width = 0;
            
            for (Entry e : list) {
                if (e.node()) {
                    std::string str;
                    
                    for (const Node* node = e.node(); node; node = node->parent())
                        if (node->attribute() == cols[c].attr.id())
                            str = node->data().to_string().append(str.empty() ? "" : "/").append(str);

                    width = std::max(str.size(), width);
                    bool ok = true;
                    
                    if (!str.empty()) 
                        val = Variant(str).concretize(cols[c].attr.type(), &ok);
                    if (!ok)
                        val = Variant(str);
                } else if (e.attribute() == cols[c].attr.id()) {
                    bool ok;
                    val = e.value().concretize(cols[c].attr.type(), &ok);

                    if (!ok)
                        val = e.value();
                }
            }

            if (!val.empty()) {
                active = true;
                row[c] = val;

                if (width > cols[c].max_width) {
                    cols[c].max_width = width;
                    update_max_width  = true;
                }
            }
        }
        
        if (active) {
            std::lock_guard<std::mutex>
                g(m_row_lock);
            
            m_rows.push_back(std::move(row));
        }

        if (update_max_width) {
            std::lock_guard<std::mutex>
                g(m_col_lock);

            for (std::vector<Column>::size_type c = 0; c < cols.size(); ++c)
                if (cols[c].max_width > m_cols[c].max_width)
                    m_cols[c].max_width = cols[c].max_width;
        }
    }

    void flush(std::ostream& os) {
        // NOTE: No locking, assume flush() runs serially

        // sort rows

        for (std::vector<Column>::size_type c = 0; c < m_num_sort_columns; ++c)
            std::stable_sort(m_rows.begin(), m_rows.end(),
                             [c](const std::vector<Variant>& lhs, const std::vector<Variant>& rhs){
                                 if (c >= lhs.size() || c >= rhs.size())
                                     return lhs.size() < rhs.size();
                                 return lhs[c] < rhs[c];
                             });
        
        const char whitespace[120+1] =
            "                                        "
            "                                        "
            "                                        ";
            
        // print header

        for (const Column& col : m_cols)
            if (col.print)
                os << col.name << whitespace+(120 - std::min<std::size_t>(120, 1+col.max_width-col.name.size()));
        
        os << std::endl;

        // print rows

        for (auto row : m_rows) {
            for (std::vector<Column>::size_type c = m_num_sort_columns; c < row.size(); ++c) {
                if (!m_cols[c].print)
                    continue;

                std::string    str = row[c].to_string();
                cali_attr_type t   = m_cols[c].attr.type();
                bool           align_right = (t == CALI_TYPE_INT || t == CALI_TYPE_UINT || t == CALI_TYPE_DOUBLE);
                std::size_t    len = m_cols[c].max_width-str.size();
                
                if (align_right)
                    os << whitespace+(120 - std::min<std::size_t>(120, len)) << str << ' ';
                else
                    os << str << whitespace+(120 - std::min<std::size_t>(120, 1+len));
            }
            
            os << std::endl;
        }
    }
};


Table::Table(const std::string& fields, const std::string& sort_fields)
    : mP { new TableImpl }
{
    mP->parse(fields, sort_fields);
}

Table::~Table()
{
    mP.reset();
}

void 
Table::operator()(CaliperMetadataDB& db, const EntryList& list)
{
    mP->add(db, list);
}

void
Table::flush(CaliperMetadataDB&, std::ostream& os)
{
    mP->flush(os);
}
