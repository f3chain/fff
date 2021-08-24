/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <fc/io/raw.hpp>

#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/db/index.hpp>
#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/hashed_index.hpp>

namespace graphene { namespace chain {
   /**
    * The purpose of this object is to enable the detection of duplicate transactions. When a transaction is included
    * in a block a transaction_object is added. At the end of block processing all transaction_objects that have
    * expired can be removed from the index.
    */
   class transaction_object : public graphene::db::abstract_object<implementation_ids, impl_transaction_object_type, transaction_object>
   {
      public:
         fc::time_point_sec expiration;
         transaction_id_type trx_id;
   };

   struct by_expiration;
   struct by_trx_id;
   typedef boost::multi_index_container<
      transaction_object,
      db::mi::indexed_by<
         db::object_id_index,
         db::mi::hashed_unique<db::mi::tag<by_trx_id>,
            BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_id_type, trx_id), std::hash<transaction_id_type>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_expiration>,
            BOOST_MULTI_INDEX_MEMBER(transaction_object, fc::time_point_sec, expiration)
         >
      >
   > transaction_multi_index_type;

   typedef graphene::db::generic_index<transaction_object, transaction_multi_index_type> transaction_index;

} }

FC_REFLECT_DERIVED( graphene::chain::transaction_object, (graphene::db::object), (expiration)(trx_id) )