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

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/protocol/transaction.hpp>

namespace graphene { namespace chain {

   class proposal_create_evaluator : public evaluator<proposal_create_operation, proposal_create_evaluator>
   {
      public:
         operation_result do_evaluate( const operation_type& o );
         operation_result do_apply( const operation_type& o );

      private:
         transaction _proposed_trx;
   };

   class proposal_update_evaluator : public evaluator<proposal_update_operation, proposal_update_evaluator>
   {
      public:
         operation_result do_evaluate( const operation_type& o );
         operation_result do_apply( const operation_type& o );

      private:
         const proposal_object* _proposal = nullptr;
         processed_transaction _processed_transaction;
         bool _executed_proposal = false;
         bool _proposal_failed = false;
   };

   class proposal_delete_evaluator : public evaluator<proposal_delete_operation, proposal_delete_evaluator>
   {
      public:
         operation_result do_evaluate( const operation_type& o );
         operation_result do_apply(const operation_type&);

      private:
         const proposal_object* _proposal = nullptr;
   };

} } // graphene::chain
