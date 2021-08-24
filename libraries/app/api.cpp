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

#include <graphene/utilities/key_conversion.hpp>
#include <graphene/app/api_access.hpp>
#include <graphene/app/exceptions.hpp>
#include <graphene/app/application.hpp>
#include <graphene/app/balance.hpp>
#include <graphene/app/impacted.hpp>
#include <fc/crypto/base64.hpp>
#include <fc/thread/thread.hpp>

namespace graphene { namespace app {

   const int CURRENT_OUTPUT_LIMIT_100 = 100;

   login_api::login_api(application& a)
   :_app(a)
   {
   }

   login_api::~login_api()
   {
   }

   bool login_api::login(const std::string& user, const std::string& password)
   {
      fc::optional<api_access_info> acc = _app.get_api_access_info( user );
      if( !acc.valid() )
         return false;
      if( acc->password_hash_b64 != "*" )
      {
         std::string password_salt = fc::base64_decode( acc->password_salt_b64 );
         std::string acc_password_hash = fc::base64_decode( acc->password_hash_b64 );

         fc::sha256 hash_obj = fc::sha256::hash( password + password_salt );
         if( hash_obj.data_size() != acc_password_hash.length() )
            return false;
         if( memcmp( hash_obj.data(), acc_password_hash.c_str(), hash_obj.data_size() ) != 0 )
            return false;
      }

      for( const std::string& api_name : acc->allowed_apis )
         enable_api( api_name );
      return true;
   }

   void login_api::enable_api( const std::string& api_name )
   {
      if( api_name == database_api::get_api_name() )
      {
         _database_api = std::make_shared< database_api >( std::ref( *_app.chain_database() ) );
      }
      else if( api_name == network_broadcast_api::get_api_name() )
      {
         _network_broadcast_api = std::make_shared< network_broadcast_api >( std::ref( _app ) );
      }
      else if( api_name == history_api::get_api_name() )
      {
         _history_api = std::make_shared< history_api >( _app );
      }
      else if( api_name == network_node_api::get_api_name() )
      {
         _network_node_api = std::make_shared< network_node_api >( std::ref(_app) );
      }
      else if( api_name == crypto_api::get_api_name() )
      {
         _crypto_api = std::make_shared< crypto_api >( std::ref(_app) );
      }
      else if (api_name == messaging_api::get_api_name() )
      {
         _messaging_api = std::make_shared< messaging_api >( std::ref(_app) );
      }
      else if (api_name == monitoring_api::get_api_name() )
      {
         _monitoring_api = std::make_shared< monitoring_api >();
      }
   }

   fc::api<network_broadcast_api> login_api::network_broadcast()const
   {
      FC_VERIFY_AND_THROW(_network_broadcast_api.valid(), api_not_available_exception);
      return *_network_broadcast_api;
   }

   fc::api<network_node_api> login_api::network_node()const
   {
      FC_VERIFY_AND_THROW(_network_node_api.valid(), api_not_available_exception);
      return *_network_node_api;
   }

   fc::api<database_api> login_api::database()const
   {
      FC_VERIFY_AND_THROW(_database_api.valid(), api_not_available_exception);
      return *_database_api;
   }

   fc::api<history_api> login_api::history() const
   {
      FC_VERIFY_AND_THROW(_history_api.valid(), api_not_available_exception);
      return *_history_api;
   }

   fc::api<crypto_api> login_api::crypto() const
   {
      FC_VERIFY_AND_THROW(_crypto_api.valid(), api_not_available_exception);
      return *_crypto_api;
   }

   fc::api<graphene::app::messaging_api> login_api::messaging() const
   {
      FC_VERIFY_AND_THROW(_messaging_api.valid(), api_not_available_exception);
      return *_messaging_api;
   }

   fc::api<monitoring_api> login_api::monitoring() const
   {
      FC_VERIFY_AND_THROW(_monitoring_api.valid(), api_not_available_exception);
      return *_monitoring_api;
   }

   network_broadcast_api::network_broadcast_api(application& a):_app(a)
   {
      _applied_block_connection = _app.chain_database()->applied_block.connect([this](const chain::signed_block& b){ on_applied_block(b); });
   }

   void network_broadcast_api::on_applied_block( const chain::signed_block& b )
   {
      if( _callbacks.size() )
      {
         /// we need to ensure the database_api is not deleted for the life of the async operation
         auto capture_this = shared_from_this();
         for( uint32_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
         {
            const auto& trx = b.transactions[trx_num];
            auto id = trx.id();
            auto itr = _callbacks.find(id);
            if( itr != _callbacks.end() )
            {
               auto block_num = b.block_num();
               auto& callback = _callbacks.find(id)->second;
               transaction_confirmation conf{ id, block_num, trx_num, trx};
               fc::variant confv(conf);

               fc::async( [capture_this,confv, callback](){ callback( confv ); } );
            }
         }
      }
   }

   void network_broadcast_api::broadcast_transaction(const chain::signed_transaction& trx) const
   {
      trx.validate();
      _app.chain_database()->push_transaction(trx);
      _app.p2p_node()->broadcast_transaction(trx);
   }

   void network_broadcast_api::broadcast_block(const chain::signed_block& b) const
   {
      _app.chain_database()->push_block(b, 0, false);
      _app.p2p_node()->broadcast( net::block_message( b ));
   }

   void network_broadcast_api::broadcast_transaction_with_callback(confirmation_callback cb, const chain::signed_transaction& trx)
   {
      trx.validate();
      _callbacks[trx.id()] = cb;
      _app.chain_database()->push_transaction(trx);
      _app.p2p_node()->broadcast_transaction(trx);
   }

   network_node_api::network_node_api( application& a ) : _app( a )
   {
   }

   network_node_info network_node_api::get_info() const
   {
      network_node_info result;
      fc::variant_object info = _app.p2p_node()->network_get_info();
      result.connection_count = _app.p2p_node()->get_connection_count();
      result.node_id = info["node_id"].as<graphene::net::node_id_t>();
      result.firewalled = info["firewalled"].as<graphene::net::firewalled_state>();
      result.listening_on = info["listening_on"].as<fc::ip::endpoint>();
      result.node_public_key = info["node_public_key"].as<graphene::net::node_id_t>();
      return result;
   }

   void network_node_api::add_node(const fc::ip::endpoint& ep) const
   {
      _app.p2p_node()->add_node(ep);
   }

   std::vector<net::peer_status> network_node_api::get_connected_peers() const
   {
      return _app.p2p_node()->get_connected_peers();
   }

   std::vector<net::potential_peer_record> network_node_api::get_potential_peers() const
   {
      return _app.p2p_node()->get_potential_peers();
   }

   advanced_node_parameters network_node_api::get_advanced_node_parameters() const
   {
      fc::variant_object result_variant = _app.p2p_node()->get_advanced_node_parameters();
      advanced_node_parameters result;
      result.peer_connection_retry_timeout = result_variant["peer_connection_retry_timeout"].as<uint32_t>();
      result.desired_number_of_connections = result_variant["desired_number_of_connections"].as<uint32_t>();
      result.maximum_number_of_connections = result_variant["maximum_number_of_connections"].as<uint32_t>();
      result.maximum_number_of_blocks_to_handle_at_one_time = result_variant["maximum_number_of_blocks_to_handle_at_one_time"].as<unsigned>();
      result.maximum_number_of_sync_blocks_to_prefetch = result_variant["maximum_number_of_sync_blocks_to_prefetch"].as<unsigned>();
      result.maximum_blocks_per_peer_during_syncing = result_variant["maximum_blocks_per_peer_during_syncing"].as<unsigned>();
      return result;
   }

   void network_node_api::set_advanced_node_parameters(const advanced_node_parameters& params) const
   {
      fc::mutable_variant_object params_variant;
      params_variant["peer_connection_retry_timeout"] = params.peer_connection_retry_timeout;
      params_variant["desired_number_of_connections"] = params.desired_number_of_connections;
      params_variant["maximum_number_of_connections"] = params.maximum_number_of_connections;
      params_variant["maximum_number_of_blocks_to_handle_at_one_time"] = params.maximum_number_of_blocks_to_handle_at_one_time;
      params_variant["maximum_number_of_sync_blocks_to_prefetch"] = params.maximum_number_of_sync_blocks_to_prefetch;
      params_variant["maximum_blocks_per_peer_during_syncing"] = params.maximum_blocks_per_peer_during_syncing;
      return _app.p2p_node()->set_advanced_node_parameters(params_variant);
   }

   std::vector<chain::operation_history_object> history_api::get_account_history(
      chain::account_id_type account, chain::operation_history_id_type stop, unsigned limit, chain::operation_history_id_type start ) const
   {
      FC_VERIFY_AND_THROW(_app.chain_database(), database_not_available_exception);
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));

      std::vector<chain::operation_history_object> result;
      const auto& db = *_app.chain_database();
      const auto& stats = account(db).statistics(db);
      const chain::account_transaction_history_object* node = db.find(stats.most_recent_op);
      if( nullptr == node ) { return result; }
      if( start == chain::operation_history_id_type() || start.instance.value > node->operation_id.instance.value )
         start = node->operation_id;

      const auto& hist_idx = db.get_index_type<chain::account_transaction_history_index>();
      const auto& by_op_idx = hist_idx.indices().get<chain::by_op>();
      auto index_start = by_op_idx.begin();
      auto itr = by_op_idx.lower_bound(boost::make_tuple(account, start));

      while( itr != index_start && itr->account == account && itr->operation_id.instance.value > stop.instance.value && result.size() < limit )
      {
         if( itr->operation_id.instance.value <= start.instance.value )
            result.push_back(itr->operation_id(db));
         --itr;
      }

      if( stop.instance.value == 0 && itr->account == account && itr->operation_id.instance.value <= start.instance.value && result.size() < limit )
         result.push_back(itr->operation_id(db));

      return result;
   }

   std::vector<chain::operation_history_object> history_api::get_relative_account_history(chain::account_id_type account, uint32_t stop, unsigned limit, uint32_t start) const
   {
      FC_VERIFY_AND_THROW(_app.chain_database(), database_not_available_exception);
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));

      std::vector<chain::operation_history_object> result;
      const auto& db = *_app.chain_database();
      if( start == 0 )
        start = account(db).statistics(db).total_ops;
      else start = std::min( account(db).statistics(db).total_ops, start );

      if( start >= stop )
      {
         const auto& hist_idx = db.get_index_type<chain::account_transaction_history_index>();
         const auto& by_seq_idx = hist_idx.indices().get<chain::by_seq>();
         auto itr = by_seq_idx.upper_bound( boost::make_tuple( account, start ) );
         auto itr_stop = by_seq_idx.lower_bound( boost::make_tuple( account, stop ) );

         while( itr != itr_stop && result.size() < limit )
         {
            --itr;
            result.push_back( itr->operation_id(db) );
         }
      }

      return result;
   }

   std::vector<balance_change_result> history_api::search_account_balance_history(
      chain::account_id_type account_id, const boost::container::flat_set<chain::asset_id_type>& assets_list, fc::optional<chain::account_id_type> partner_account_id,
      uint32_t from_block, uint32_t to_block, uint32_t start_offset, unsigned limit) const
   {
      std::vector<balance_change_result> result;
      std::vector<chain::operation_history_object> current_history;
      chain::operation_history_id_type start;
      uint32_t current_history_offset = 0;
      uint32_t current_offset = 0;
      bool account_history_query_required = true;
      result.reserve(limit);

      FC_VERIFY_AND_THROW(_app.chain_database(), database_not_available_exception);
      const auto& db = *_app.chain_database();

      try
      {
         if (limit > 0)
         {
            do
            {
               if (account_history_query_required)
               {
                  current_history = this->get_account_history(account_id, chain::operation_history_id_type(), 100, start);
                  account_history_query_required = false;
               }

               // access and store the current account history object
               if (current_history_offset < current_history.size())
               {
                  auto &o = current_history.at(current_history_offset);

                  // if no block range is specified or if the current block is within the block range
                  if ((from_block == 0 && to_block == 0) || (o.block_num >= from_block && o.block_num <= to_block))
                  {
                     // create the balance change result object
                     balance_change_result info;
                     info.hist_object = o;
                     graphene::app::operation_get_balance_history(o.op, account_id, info.balance, info.fee);
                     fc::optional<chain::signed_block> block = db.fetch_block_by_number(o.block_num);
                     if (block) {
                        info.timestamp = block->timestamp;
                        if (!block->transactions.empty() && o.trx_in_block < block->transactions.size()) {
                           info.transaction_id = block->transactions[o.trx_in_block].id();
                        }
                     }

                     if (info.balance.asset0.amount != 0ll || info.balance.asset1.amount != 0ll || info.fee.amount != 0ll)
                     {
                        bool is_non_zero_balance_for_asset0 = (info.balance.asset0.amount != 0ll && assets_list.find(info.balance.asset0.asset_id) != assets_list.end());
                        bool is_non_zero_balance_for_asset1 = (info.balance.asset1.amount != 0ll && assets_list.find(info.balance.asset1.asset_id) != assets_list.end());
                        bool is_non_zero_fee = (info.fee.amount != 0ll && assets_list.find(info.fee.asset_id) != assets_list.end());

                        if (assets_list.empty() || is_non_zero_balance_for_asset0 || is_non_zero_balance_for_asset1 || is_non_zero_fee)
                        {
                           bool skip_due_to_partner_account_id = false;

                           if (partner_account_id)
                           {
                              if (o.op.which() == chain::operation::tag<chain::transfer_obsolete_operation>::value)
                              {
                                 const chain::transfer_obsolete_operation& top = o.op.get<chain::transfer_obsolete_operation>();
                                 if (! top.is_partner_account_id(*partner_account_id))
                                    skip_due_to_partner_account_id = true;
                              }
                              else if (o.op.which() == chain::operation::tag<chain::transfer_operation>::value)
                              {
                                 const chain::transfer_operation& top = o.op.get<chain::transfer_operation>();
                                 if (! top.is_partner_account_id(*partner_account_id))
                                    skip_due_to_partner_account_id = true;
                              }
                           }

                           if (! skip_due_to_partner_account_id)
                           {
                              // store the balance change result object
                              if (current_offset >= start_offset)
                                 result.push_back(info);
                              current_offset++;
                           }
                        }
                     }
                  }
               }
               // rolling in the account transaction history
               else if (! current_history.empty())
               {
                  account_history_query_required = true;
                  current_history_offset = 0;
                  start = current_history.back().id;
                  if (start != chain::operation_history_id_type())
                     start = start + (-1);
               }

               if (! account_history_query_required)
                  current_history_offset++;
            }
            // while the limit is not reached and there are potentially more entries to be processed
            while (result.size() < limit && ! current_history.empty() && current_history_offset <= current_history.size() && (current_history_offset != 0 || start != chain::operation_history_id_type()));
         }

         return result;
      }
      FC_CAPTURE_AND_RETHROW((account_id)(assets_list)(partner_account_id)(from_block)(to_block)(start_offset)(limit));
   }

   fc::optional<balance_change_result> history_api::get_account_balance_for_transaction(chain::account_id_type account_id, chain::operation_history_id_type operation_history_id) const
   {
       std::vector<chain::operation_history_object> operation_list = this->get_account_history(account_id, chain::operation_history_id_type(), 1, operation_history_id);
       if (operation_list.empty())
           return fc::optional<balance_change_result>();

       balance_change_result result;
       result.hist_object = operation_list.front();

       graphene::app::operation_get_balance_history(result.hist_object.op, account_id, result.balance, result.fee);

       return result;
   }

   crypto_api::crypto_api(application& a) : _app(a)
   {
   }

   chain::public_key_type crypto_api::wif_to_public_key(const std::string &wif) const
   {
      return wif_to_private_key(wif).get_public_key();
   }

   chain::private_key_type crypto_api::wif_to_private_key(const std::string &wif) const
   {
       fc::optional<chain::private_key_type> key = graphene::utilities::wif_to_key(wif);
       FC_VERIFY_AND_THROW(key.valid(), malformed_private_key_exception);
       return *key;
   }

   chain::signed_transaction crypto_api::sign_transaction(const chain::transaction& trx, const chain::private_key_type &key) const
   {
       FC_VERIFY_AND_THROW(_app.chain_database(), database_not_available_exception);
       chain::signed_transaction signed_trx(trx);
       signed_trx.sign(key, _app.chain_database()->get_chain_id());
       return signed_trx;
   }

   chain::memo_data crypto_api::encrypt_message(const std::string &message, const chain::private_key_type &key, const chain::public_key_type &pub, uint64_t nonce) const
   {
       return chain::memo_data(message, key, pub, nonce);
   }

   std::string crypto_api::decrypt_message(const chain::memo_data::message_type &message, const chain::private_key_type &key, const chain::public_key_type &pub, uint64_t nonce) const
   {
      return chain::memo_data::decrypt_message(message, key, pub, nonce);
   }

   messaging_api::messaging_api(application& a) : _app(a)
   {
   }

   template<typename ID, typename COMP>
   void find_message_objects(std::vector<chain::message_object>& result, const ID& ids, const std::set<graphene::db::object_id_type>& objs, uint32_t max_count, const COMP& cmp)
   {
      for (auto it = objs.crbegin(); max_count > 0 && it != objs.crend(); ++it) {
         auto msg_itr = ids.find(*it);
         if (msg_itr != ids.end()) {
            const chain::message_object& msg_obj = *msg_itr;
            if (cmp(msg_obj.sender)) {
               result.emplace_back(msg_obj);
               --max_count;
            }
         }
      }
   }

   std::vector<chain::message_object> messaging_api::get_message_objects(fc::optional<chain::account_id_type> sender, fc::optional<chain::account_id_type> receiver, uint32_t max_count) const
   {
      FC_VERIFY_AND_THROW(_app.chain_database(), database_not_available_exception);
      FC_VERIFY_AND_THROW(sender.valid() || receiver.valid(), at_least_one_account_needs_to_be_specified_exception);

      const auto& db = *_app.chain_database();
      const auto& idx = db.get_index_type<chain::message_index>();

      std::vector<chain::message_object> result;
      result.reserve(max_count);
      if (receiver) {
         try {
            (*receiver)(db);
         }
         FC_REWRAP_EXCEPTIONS(account_does_not_exist_exception, error, "Receiver account: ${receiver}", ("receiver", receiver));

         const auto& ids = idx.indices().get<graphene::db::by_id>();
         const auto& midx = dynamic_cast<const graphene::db::primary_index<chain::message_index>&>(idx);
         const auto& refs = midx.get_secondary_index<graphene::chain::message_receiver_index>();
         auto itr = refs.message_to_receiver_memberships.find(*receiver);

         if (itr != refs.message_to_receiver_memberships.end())
         {
            result.reserve(itr->second.size());
            if (sender) {
               try {
                  (*sender)(db);
               }
               FC_REWRAP_EXCEPTIONS(account_does_not_exist_exception, error, "Sender account: ${sender}", ("sender", sender));

               find_message_objects(result, ids, itr->second, max_count, [&](chain::account_id_type s) { return s == *sender; });
            }
            else {
               find_message_objects(result, ids, itr->second, max_count, [](chain::account_id_type) { return true; });
            }
         }
      }
      else if (sender) {
         try {
            (*sender)(db);
         }
         FC_REWRAP_EXCEPTIONS(account_does_not_exist_exception, error, "Sender account: ${sender}", ("sender", sender));

         auto range = idx.indices().get<chain::by_sender>().equal_range(*sender);
         while (range.first != range.second && max_count-- > 0) {
            result.emplace_back(*(--range.second));
         }
      }

      return result;
   }

   std::vector<fc::optional<chain::message_object>> messaging_api::get_messages(const std::vector<chain::message_id_type>& message_ids) const
   {
      FC_VERIFY_AND_THROW(_app.chain_database(), database_not_available_exception);
      return _app.chain_database()->get_objects(message_ids);
   }

   monitoring_api::monitoring_api()
   {
   }

   void monitoring_api::reset_counters(const std::vector<std::string>& names) const
   {
      monitoring::monitoring_counters_base::reset_counters(names);
   }

   std::vector<monitoring::counter_item> monitoring_api::get_counters(const std::vector<std::string>& names) const
   {
      std::vector<monitoring::counter_item> result;
      monitoring::monitoring_counters_base::get_counters(names, result);
      return result;
   }

} } // graphene::app
