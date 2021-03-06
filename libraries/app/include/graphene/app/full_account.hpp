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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

namespace graphene { namespace app {

   struct full_account
   {
      chain::account_object account;
      chain::account_statistics_object statistics;
      std::string registrar_name;
      std::vector<fc::optional<chain::miner_object>> votes;
      fc::optional<chain::vesting_balance_object> cashback_balance;
      std::vector<chain::account_balance_object>  balances;
      std::vector<chain::vesting_balance_object>  vesting_balances;
      std::vector<chain::proposal_object>         proposals;
   };

} }

FC_REFLECT( graphene::app::full_account,
            (account)
            (statistics)
            (registrar_name)
            (votes)
            (cashback_balance)
            (balances)
            (vesting_balances)
            (proposals)
          )
