/* (c) 2016, 2021 FFF Services. For details refers to LICENSE.txt */
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
#include <graphene/chain/withdraw_permission_evaluator.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

operation_result withdraw_permission_create_evaluator::do_evaluate(const operation_type& op)
{ try {
   database& d = db();
   FC_ASSERT(d.find_object(op.withdraw_from_account));
   FC_ASSERT(d.find_object(op.authorized_account));
   FC_ASSERT(d.find_object(op.withdrawal_limit.asset_id));
   FC_ASSERT(op.period_start_time > d.head_block_time());
   FC_ASSERT(op.period_start_time + op.periods_until_expiration * op.withdrawal_period_sec > d.head_block_time());
   FC_ASSERT(op.withdrawal_period_sec >= d.get_global_properties().parameters.block_interval);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_create_evaluator::do_apply(const operation_type& op)
{ try {
   return db().create<withdraw_permission_object>([&op](withdraw_permission_object& p) {
      p.withdraw_from_account = op.withdraw_from_account;
      p.authorized_account = op.authorized_account;
      p.withdrawal_limit = op.withdrawal_limit;
      p.withdrawal_period_sec = op.withdrawal_period_sec;
      p.expiration = op.period_start_time + op.periods_until_expiration * op.withdrawal_period_sec;
      p.period_start_time = op.period_start_time;
   }).id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_claim_evaluator::do_evaluate(const withdraw_permission_claim_evaluator::operation_type& op)
{ try {
   const database& d = db();

   const withdraw_permission_object& permit = op.withdraw_permission(d);
   FC_ASSERT(permit.expiration > d.head_block_time() );
   FC_ASSERT(permit.authorized_account == op.withdraw_to_account);
   FC_ASSERT(permit.withdraw_from_account == op.withdraw_from_account);
   FC_ASSERT(op.amount_to_withdraw <= permit.available_this_period( d.head_block_time() ) );
   FC_ASSERT(d.get_balance(op.withdraw_from_account, op.amount_to_withdraw.asset_id) >= op.amount_to_withdraw);

   op.amount_to_withdraw.asset_id(d);
   op.withdraw_to_account(d);
   permit.authorized_account(d);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_claim_evaluator::do_apply(const withdraw_permission_claim_evaluator::operation_type& op)
{ try {
   database& d = db();

   const withdraw_permission_object& permit = d.get(op.withdraw_permission);
   d.modify(permit, [&](withdraw_permission_object& p) {
      auto periods = (d.head_block_time() - p.period_start_time).to_seconds() / p.withdrawal_period_sec;
      p.period_start_time += static_cast<uint32_t>(periods * p.withdrawal_period_sec);
      if( periods == 0 )
         p.claimed_this_period += op.amount_to_withdraw.amount;
      else
         p.claimed_this_period = op.amount_to_withdraw.amount;
   });

   d.adjust_balance(op.withdraw_from_account, -op.amount_to_withdraw);
   d.adjust_balance(op.withdraw_to_account, op.amount_to_withdraw);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_update_evaluator::do_evaluate(const withdraw_permission_update_evaluator::operation_type& op)
{ try {
   database& d = db();

   const withdraw_permission_object& permit = op.permission_to_update(d);
   FC_ASSERT(permit.authorized_account == op.authorized_account);
   FC_ASSERT(permit.withdraw_from_account == op.withdraw_from_account);
   FC_ASSERT(d.find_object(op.withdrawal_limit.asset_id));
   FC_ASSERT(op.period_start_time >= d.head_block_time());
   FC_ASSERT(op.period_start_time + op.periods_until_expiration * op.withdrawal_period_sec > d.head_block_time());
   FC_ASSERT(op.withdrawal_period_sec >= d.get_global_properties().parameters.block_interval);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_update_evaluator::do_apply(const withdraw_permission_update_evaluator::operation_type& op)
{ try {
   database& d = db();

   d.modify(op.permission_to_update(d), [&op](withdraw_permission_object& p) {
      p.period_start_time = op.period_start_time;
      p.expiration = op.period_start_time + op.periods_until_expiration * op.withdrawal_period_sec;
      p.withdrawal_limit = op.withdrawal_limit;
      p.withdrawal_period_sec = op.withdrawal_period_sec;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_delete_evaluator::do_evaluate(const withdraw_permission_delete_evaluator::operation_type& op)
{ try {
   database& d = db();

   const withdraw_permission_object& permit = op.withdrawal_permission(d);
   FC_ASSERT(permit.authorized_account == op.authorized_account);
   FC_ASSERT(permit.withdraw_from_account == op.withdraw_from_account);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result withdraw_permission_delete_evaluator::do_apply(const withdraw_permission_delete_evaluator::operation_type& op)
{ try {
   db().remove(db().get(op.withdrawal_permission));
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
