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
#pragma once
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/protocol/assert.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>
#include <graphene/chain/protocol/custom.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/protocol/transfer.hpp>
#include <graphene/chain/protocol/vesting.hpp>
#include <graphene/chain/protocol/withdraw_permission.hpp>
#include <graphene/chain/protocol/miner.hpp>
#include <graphene/chain/protocol/decent.hpp>
#include <graphene/chain/protocol/subscription.hpp>
#include <graphene/chain/protocol/non_fungible_token.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    *
    * Defines the set of valid operations as a discriminated union type.
    */
   typedef fc::static_variant<
            transfer_obsolete_operation,
            account_create_operation,
            account_update_operation,
            asset_create_operation,
            asset_issue_operation,
            asset_publish_feed_operation,   //5
            miner_create_operation,
            miner_update_operation,
            miner_update_global_parameters_operation,
            proposal_create_operation,
            proposal_update_operation,      //10
            proposal_delete_operation,
            withdraw_permission_create_operation,
            withdraw_permission_update_operation,
            withdraw_permission_claim_operation,
            withdraw_permission_delete_operation,   //15
            vesting_balance_create_operation,
            vesting_balance_withdraw_operation,
            custom_operation,
            assert_operation,
            content_submit_operation,       //20
            request_to_buy_operation,
            leave_rating_and_comment_operation,
            ready_to_publish_obsolete_operation,
            proof_of_custody_operation,
            deliver_keys_operation,                 //25
            subscribe_operation,
            subscribe_by_author_operation,
            automatic_renewal_of_subscription_operation,
            report_stats_operation,
            set_publishing_manager_operation, //30
            set_publishing_right_operation,
            content_cancellation_operation,
            asset_fund_pools_operation,
            asset_reserve_operation,
            asset_claim_fees_operation,     //35
            update_user_issued_asset_operation,
            update_monitored_asset_operation,
            ready_to_publish_operation,
            transfer_operation,
            update_user_issued_asset_advanced_operation,      //40
            non_fungible_token_create_definition_operation,
            non_fungible_token_update_definition_operation,
            non_fungible_token_issue_operation,
            non_fungible_token_transfer_operation,
            non_fungible_token_update_data_operation,         //45
            disallow_automatic_renewal_of_subscription_operation,  // VIRTUAL
            return_escrow_submission_operation,                    // VIRTUAL
            return_escrow_buying_operation,                        // VIRTUAL
            pay_seeder_operation,                                  // VIRTUAL
            finish_buying_operation,                          //50 // VIRTUAL
            renewal_of_subscription_operation                      // VIRTUAL
         > operation;

   /// @} // operations group

   /**
    *  Appends required authorites to the result vector.  The authorities appended are not the
    *  same as those returned by get_required_auth
    *
    *  @return a set of required authorities for \c op
    */
   void operation_get_required_authorities( const operation& op,
                                            boost::container::flat_set<account_id_type>& active,
                                            boost::container::flat_set<account_id_type>& owner,
                                            std::vector<authority>&  other );

   void operation_validate( const operation& op );

   /**
    *  @brief necessary to support nested operations inside the proposal_create_operation
    */
   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

} } // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::operation )
FC_REFLECT( graphene::chain::op_wrapper, (op) )
