// Copyright (c) 2015, Lawrence Livermore National Security, LLC.  
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

/// @file CaliperMetadataDB
/// CaliperMetadataDB class declaration

#ifndef CALI_CALIPERMETADATADB_H
#define CALI_CALIPERMETADATADB_H

#include "RecordProcessor.h"

#include "Attribute.h"
#include "RecordMap.h"

#include <map>
#include <memory>
#include <string>

namespace cali
{

class Node;
class Variant;
    
typedef std::map<cali_id_t, cali_id_t> IdMap;

class CaliperMetadataDB
{
    struct CaliperMetadataDBImpl;
    std::unique_ptr<CaliperMetadataDBImpl> mP;

public:

    CaliperMetadataDB();

    ~CaliperMetadataDB();

    //
    // --- I/O API 
    // 

    bool        read(const char* filename);

    RecordMap   merge(const RecordMap& rec, IdMap& map);
    void        merge(const RecordMap& rec, IdMap& map, NodeProcessFn& node_fn, SnapshotProcessFn& snap_fn);

    //
    // --- Query API
    //

    const Node* node(cali_id_t id) const;
    
    Attribute   attribute(cali_id_t id) const;
    Attribute   attribute(const std::string& name) const;
    
    //
    // --- Manipulation
    //

    const Node* make_entry(size_t n, const Attribute* attr, const Variant* value);
    Attribute   create_attribute(const std::string& name, cali_attr_type type, int prop);

    // awful kludge till we find solution for node writing problem
    Node*       mutable_node(cali_id_t);
};

}

#endif
