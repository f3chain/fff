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
#include <graphene/app/application.hpp>
#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>

#include "../common/tempdir.hpp"

#include <graphene/account_history/account_history_plugin.hpp>

#include <fc/thread/thread.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE Test Application
#include <boost/test/included/unit_test.hpp>

using namespace graphene;

BOOST_AUTO_TEST_CASE( two_node_network )
{
   using namespace graphene::chain;
   using namespace graphene::app;
   try {
      BOOST_TEST_MESSAGE( "Creating temporary files" );

      fc::temp_directory app_dir( graphene::utilities::temp_directory_path() );
      fc::temp_directory app2_dir( graphene::utilities::temp_directory_path() );
      fc::temp_file genesis_json( graphene::utilities::temp_directory_path() );

      BOOST_TEST_MESSAGE( "Creating and initializing app1" );

      using test_plugins = graphene::app::plugin_set<
         account_history::account_history_plugin
      >;

      graphene::app::application app1;
      test_plugins::create(app1);
      boost::program_options::variables_map cfg;
      cfg.emplace("p2p-endpoint", boost::program_options::variable_value(std::string("127.0.0.1:3939"), false));
      app1.initialize(app_dir.path(), cfg);

      BOOST_TEST_MESSAGE( "Creating and initializing app2" );

      graphene::app::application app2;
      test_plugins::create(app2);
      auto cfg2 = cfg;
      cfg2.erase("p2p-endpoint");
      cfg2.emplace("p2p-endpoint", boost::program_options::variable_value(std::string("127.0.0.1:4040"), false));
      cfg2.emplace("seed-node", boost::program_options::variable_value(std::vector<std::string>{"127.0.0.1:3939"}, false));
      app2.initialize(app2_dir.path(), cfg2);

      BOOST_TEST_MESSAGE( "Starting app1 and waiting 500 ms" );
      app1.startup();
      fc::usleep(fc::milliseconds(500));
      BOOST_TEST_MESSAGE( "Starting app2 and waiting 500 ms" );
      app2.startup();
      fc::usleep(fc::milliseconds(500));

      BOOST_REQUIRE_EQUAL(app1.p2p_node()->get_connection_count(), 1);
      BOOST_CHECK_EQUAL(std::string(app1.p2p_node()->get_connected_peers().front().host.get_address()), "127.0.0.1");
      BOOST_TEST_MESSAGE( "app1 and app2 successfully connected" );

      std::shared_ptr<chain::database> db1 = app1.chain_database();
      std::shared_ptr<chain::database> db2 = app2.chain_database();

      BOOST_CHECK_EQUAL( db1->get_balance( GRAPHENE_NULL_ACCOUNT, asset_id_type() ).amount.value, 0 );
      BOOST_CHECK_EQUAL( db2->get_balance( GRAPHENE_NULL_ACCOUNT, asset_id_type() ).amount.value, 0 );

      BOOST_TEST_MESSAGE( "Creating transfer tx" );
      graphene::chain::signed_transaction trx;
      {
         account_id_type nathan_id = db2->get_index_type<account_index>().indices().get<by_name>().find( "nathan" )->id;
         fc::ecc::private_key nathan_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));
         transfer_obsolete_operation xfer_op;
         xfer_op.from = nathan_id;
         xfer_op.to = GRAPHENE_NULL_ACCOUNT;
         xfer_op.amount = asset( 1000000 );
         trx.operations.push_back( xfer_op );
         db1->current_fee_schedule().set_fee( trx.operations.back() );

         trx.set_expiration( db1->get_slot_time( 10 ) );
         trx.sign( nathan_key, db1->get_chain_id() );
         trx.validate();
      }

      BOOST_TEST_MESSAGE( "Pushing tx locally on db1" );
      processed_transaction ptrx = db1->push_transaction(trx);

      BOOST_CHECK_EQUAL( db1->get_balance( GRAPHENE_NULL_ACCOUNT, asset_id_type() ).amount.value, 1000000 );
      BOOST_CHECK_EQUAL( db2->get_balance( GRAPHENE_NULL_ACCOUNT, asset_id_type() ).amount.value, 0 );

      BOOST_TEST_MESSAGE( "Broadcasting tx" );
      app1.p2p_node()->broadcast(graphene::net::trx_message(trx));

      fc::usleep(fc::milliseconds(500));

      BOOST_CHECK_EQUAL( db1->get_balance( GRAPHENE_NULL_ACCOUNT, asset_id_type() ).amount.value, 1000000 );
      BOOST_CHECK_EQUAL( db2->get_balance( GRAPHENE_NULL_ACCOUNT, asset_id_type() ).amount.value, 1000000 );

      BOOST_TEST_MESSAGE( "Generating block on db2" );
      fc::ecc::private_key committee_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));

      auto block_1 = db2->generate_block(
         db2->get_slot_time(1),
         db2->get_scheduled_miner(1),
         committee_key,
         database::skip_nothing);

      BOOST_TEST_MESSAGE( "Broadcasting block" );
      app2.p2p_node()->broadcast(graphene::net::block_message( block_1 ));

      fc::usleep(fc::milliseconds(500));
      BOOST_TEST_MESSAGE( "Verifying nodes are still connected" );
      BOOST_CHECK_EQUAL(app1.p2p_node()->get_connection_count(), 1);
      BOOST_CHECK_EQUAL(app1.chain_database()->head_block_num(), 1);

      BOOST_TEST_MESSAGE( "Checking GRAPHENE_NULL_ACCOUNT has balance" );
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
}
