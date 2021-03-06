#ifndef CALI_ENTRY_H
#define CALI_ENTRY_H

#include "Attribute.h"
#include "Variant.h"

namespace cali
{

//
// --- Class for either a node or immediate data element
//

class Entry 
{
    const Node* m_node;
    cali_id_t   m_attr_id;
    Variant     m_value;

public:

    Entry()
        : m_node(0), m_attr_id(CALI_INV_ID)
        { }

    Entry(const Node* node)
        : m_node(node), m_attr_id(CALI_INV_ID)
        { }

    Entry(const Attribute& attr, const Variant& val)
        : m_node(0), m_attr_id(attr.id()), m_value(val)
        { }

    const Node* node() const {
        return m_node;
    }

    size_t    hash() const;
    
    /// @brief Return top-level attribute of this entry
    cali_id_t attribute() const;

    /// @brief Count instances of attribute @param attr_id in this entry
    int       count(cali_id_t attr_id = CALI_INV_ID) const;
    int       count(const Attribute& attr) const {
        return count(attr.id());
    }

    /// @brief Return top-level data value of this entry
    Variant   value() const;

    /// @brief Extract data value for attribute @param attr_id from this entry
    Variant   value(cali_id_t attr_id) const;
    Variant   value(const Attribute& attr) const {
        return value(attr.id());
    }

    bool      is_empty() const {
        return m_node == 0 && m_attr_id == CALI_INV_ID;
    }
    bool      is_immediate() const {
        return m_node == 0 && m_attr_id != CALI_INV_ID;
    }

    // int       extract(cali_id_t attr, int n, Variant buf[]) const;

    static const Entry empty;

    friend bool operator == (const Entry&, const Entry&);
    friend bool operator <  (const Entry&, const Entry&);
};

bool operator == (const Entry& lhs, const Entry& rhs);

bool operator <  (const Entry& lhs, const Entry& rhs);    
    
} // namespace cali

#endif
