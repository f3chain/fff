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

#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

class database;

/**
 *  @brief tracks the approval of a partially approved transaction
 *  @ingroup object
 *  @ingroup protocol
 */
class proposal_object : public graphene::db::abstract_object<protocol_ids, proposal_object_type, proposal_object>
{
   public:
      fc::time_point_sec expiration_time;
      fc::optional<fc::time_point_sec> review_period_time;
      transaction proposed_transaction;
      boost::container::flat_set<account_id_type> required_active_approvals;
      boost::container::flat_set<account_id_type> available_active_approvals;
      boost::container::flat_set<account_id_type> required_owner_approvals;
      boost::container::flat_set<account_id_type> available_owner_approvals;
      boost::container::flat_set<public_key_type> available_key_approvals;

      bool is_authorized_to_execute(database& db)const;
};

/**
 *  @brief tracks all of the proposal objects that requrie approval of
 *  an individual account.
 *
 *  @ingroup object
 *  @ingroup protocol
 *
 *  This is a secondary index on the proposal_index
 *
 *  @note the set of required approvals is constant
 */
class required_approval_index : public graphene::db::secondary_index
{
   public:
      virtual void object_inserted( const graphene::db::object& obj ) override;
      virtual void object_removed( const graphene::db::object& obj ) override;
      virtual void about_to_modify( const graphene::db::object& before ) override{};
      virtual void object_modified( const graphene::db::object& after  ) override{};

      void remove( account_id_type a, proposal_id_type p );

      std::map<account_id_type, std::set<proposal_id_type>> _account_to_proposals;
};

struct by_expiration;
typedef boost::multi_index_container<
   proposal_object,
   db::mi::indexed_by<
      db::object_id_index,
      db::mi::ordered_non_unique<db::mi::tag<by_expiration>,
         db::mi::member<proposal_object, fc::time_point_sec, &proposal_object::expiration_time>
      >
   >
> proposal_multi_index_container;

typedef graphene::db::generic_index<proposal_object, proposal_multi_index_container> proposal_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::proposal_object, (graphene::db::object),
                    (expiration_time)(review_period_time)(proposed_transaction)(required_active_approvals)
                    (available_active_approvals)(required_owner_approvals)(available_owner_approvals)
                    (available_key_approvals) )
