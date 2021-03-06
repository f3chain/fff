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

#include <graphene/chain/get_config.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

configuration get_configuration()
{
   return {
      GRAPHENE_SYMBOL,
      GRAPHENE_MIN_ACCOUNT_NAME_LENGTH,
      GRAPHENE_MAX_ACCOUNT_NAME_LENGTH,
      GRAPHENE_MIN_ASSET_SYMBOL_LENGTH,
      GRAPHENE_MAX_ASSET_SYMBOL_LENGTH,
      GRAPHENE_MAX_SHARE_SUPPLY,
      GRAPHENE_MAX_SIG_CHECK_DEPTH,
      GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT,
      GRAPHENE_MIN_BLOCK_INTERVAL,
      GRAPHENE_MAX_BLOCK_INTERVAL,
      GRAPHENE_DEFAULT_BLOCK_INTERVAL,
      GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE,
      GRAPHENE_DEFAULT_MAX_BLOCK_SIZE,
      GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION,
      GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL,
      GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS,
      GRAPHENE_MIN_UNDO_HISTORY,
      GRAPHENE_MAX_UNDO_HISTORY,
      GRAPHENE_MIN_BLOCK_SIZE_LIMIT,
      GRAPHENE_BLOCKCHAIN_PRECISION,
      GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS,
      GRAPHENE_MAX_INSTANCE_ID,
      GRAPHENE_100_PERCENT,
      GRAPHENE_1_PERCENT,
      GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME,
      GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP,
      GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS,
      GRAPHENE_DEFAULT_MAX_MINERS,
      GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC,
      GRAPHENE_DEFAULT_MINER_PROPOSAL_REVIEW_PERIOD_SEC,
      GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC,
      GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD,
      GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE,
      GRAPHENE_MAX_URL_LENGTH,
      GRAPHENE_DEFAULT_MINER_PAY_PER_BLOCK,
      GRAPHENE_DEFAULT_MINER_PAY_VESTING_SECONDS,
      GRAPHENE_MINER_ACCOUNT,
      GRAPHENE_NULL_ACCOUNT,
      GRAPHENE_TEMP_ACCOUNT
   };
}

} } // graphene::chain
