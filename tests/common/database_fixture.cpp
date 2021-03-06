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
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/account_history/account_history_plugin.hpp>
//#include <graphene/market_history/market_history_plugin.hpp>

#include <graphene/db/simple_index.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/miner_object.hpp>

#include "tempdir.hpp"

#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

using namespace graphene::chain::test;

uint32_t GRAPHENE_TESTING_GENESIS_TIMESTAMP = 1431700000;

namespace graphene { namespace chain {

using std::cout;
using std::cerr;

database_fixture::database_fixture()
     : app(), db( *app.chain_database() )
{
  try {
     int argc = boost::unit_test::framework::master_test_suite().argc;
     char** argv = boost::unit_test::framework::master_test_suite().argv;
     for( int i=1; i<argc; i++ )
     {
        const std::string arg = argv[i];
        if( arg == "--record-assert-trip" )
           fc::enable_record_assert_trip = true;
        if( arg == "--show-test-names" )
           std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
     }

     using test_plugins = graphene::app::plugin_set<
        account_history::account_history_plugin
     >;

     auto ahplugin = std::get<0>(test_plugins::create(app));
     init_account_pub_key = init_account_priv_key.get_public_key();

     boost::program_options::variables_map options;

     genesis_state.initial_timestamp = fc::time_point_sec( GRAPHENE_TESTING_GENESIS_TIMESTAMP );

     genesis_state.initial_active_miners = 10;
     for( int i = 0; i < (int)genesis_state.initial_active_miners; ++i )
     {
        auto name = "init"+fc::to_string(i);
        genesis_state.initial_accounts.emplace_back(name,
                                                    init_account_priv_key.get_public_key(),
                                                    init_account_priv_key.get_public_key());
        genesis_state.initial_miner_candidates.push_back({name, init_account_priv_key.get_public_key()});
     }
     genesis_state.initial_parameters.current_fees->zero_all_fees();
     open_database();
     // app.initialize();
     ahplugin->plugin_initialize(options);

     ahplugin->plugin_startup();

     generate_block();

     set_expiration( db, trx );
  } catch ( const fc::exception& e )
  {
     edump( (e.to_detail_string()) );
     throw;
  }

  return;
}

database_fixture::~database_fixture() noexcept(false)
{ try {
     // If we're unwinding due to an exception, don't do any more checks.
     // This way, boost test's last checkpoint tells us approximately where the error was.
     if( !std::uncaught_exception() )
     {
//        verify_asset_supplies(db);
//        verify_account_history_plugin_index();
        BOOST_CHECK( db.get_node_properties().skip_flags == database::skip_nothing );
     }

//     if( data_dir )
//        db.close();

     return;
  } FC_RETHROW() }



fc::ecc::private_key database_fixture::generate_private_key(const std::string& seed)
{
  static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("null_key")));
  if( seed == "null_key" )
     return committee;
  return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}

void database_fixture::verify_asset_supplies( const database& db )
{
  //wlog("*** Begin asset supply verification ***");
  const asset_dynamic_data_object& core_asset_data = db.get_core_asset().dynamic_asset_data_id(db);

  const simple_index<account_statistics_object>& statistics_index = db.get_index_type<simple_index<account_statistics_object>>();
  const auto& balance_index = db.get_index_type<account_balance_index>().indices();
  const auto& asset_idx = db.get_index_type<asset_index>().indices().get<by_id>();
  std::map<asset_id_type, share_type> total_balances;
  std::map<asset_id_type, share_type> total_debts;
  share_type core_in_orders;
  share_type reported_core_in_orders;

  for( const asset_object& a : asset_idx ) {
     const auto& ad = a.dynamic_asset_data_id(db);
     total_balances[asset_id_type()] += ad.core_pool;
     total_balances[a.get_id()] += ad.asset_pool;
  }

  for( const account_balance_object& b : balance_index )
     total_balances[b.asset_type] += b.balance;
  for( const account_statistics_object& a : statistics_index )
  {
     reported_core_in_orders += a.total_core_in_orders;
     total_balances[asset_id_type()] += a.pending_fees + a.pending_vested_fees;
  }
//   for( const limit_order_object& o : db.get_index_type<limit_order_index>().indices() )
//   {
//      asset for_sale = o.amount_for_sale();
//      if( for_sale.asset_id == asset_id_type() ) core_in_orders += for_sale.amount;
//      total_balances[for_sale.asset_id] += for_sale.amount;
//      total_balances[asset_id_type()] += o.deferred_fee;
//   }
//   for( const asset_object& asset_obj : db.get_index_type<asset_index>().indices() )
//   {
//      total_balances[asset_obj.id] += asset_obj.dynamic_asset_data_id(db).accumulated_fees;
//      if( asset_obj.id != asset_id_type() )
//         BOOST_CHECK_EQUAL(total_balances[asset_obj.id].value, asset_obj.dynamic_asset_data_id(db).current_supply.value);
//   }
  for( const vesting_balance_object& vbo : db.get_index_type< vesting_balance_index >().indices() )
     total_balances[ vbo.balance.asset_id ] += vbo.balance.amount;

//   total_balances[asset_id_type()] += db.get_dynamic_global_properties().miner_budget;

  for( const auto& item : total_debts )
  {
     BOOST_CHECK_EQUAL(item.first(db).dynamic_asset_data_id(db).current_supply.value, item.second.value);
  }

  BOOST_CHECK_EQUAL( core_in_orders.value , reported_core_in_orders.value );
  //int64_t v = total_balances[asset_id_type()].value; DEBUG
  BOOST_CHECK_EQUAL( total_balances[asset_id_type()].value , core_asset_data.current_supply.value);
//   wlog("***  End  asset supply verification ***");
}

const account_object& database_fixture::create_account(
     const std::string& name,
     const public_key_type& key /* = public_key_type() */
)
{
  trx.operations.push_back(make_account(name, key));
  trx.validate();
  processed_transaction ptx = db.push_transaction(trx, ~0);
  auto& result = db.get<account_object>(ptx.operation_results[0].get<object_id_type>());
  trx.operations.clear();
  return result;
}

const account_object& database_fixture::create_account(
     const std::string& name,
     const account_object& registrar,
     const account_object& referrer,
     uint8_t referrer_percent /* = 100 */,
     const public_key_type& key /*= public_key_type()*/
)
{
  try
  {
     trx.operations.resize(1);
     trx.operations.back() = (make_account(name, registrar, referrer, referrer_percent, key));
     trx.validate();
     auto r = db.push_transaction(trx, ~0);
     const auto& result = db.get<account_object>(r.operation_results[0].get<object_id_type>());
     trx.operations.clear();
     return result;
  }
  FC_CAPTURE_AND_RETHROW( (name)(registrar)(referrer) )
}

const account_object& database_fixture::create_account(
     const std::string& name,
     const private_key_type& key,
     const account_id_type& registrar_id /* = account_id_type() */,
     const account_id_type& referrer_id /* = account_id_type() */,
     uint8_t referrer_percent /* = 100 */
)
{
  try
  {
     trx.operations.clear();

     account_create_operation account_create_op;

     account_create_op.registrar = registrar_id;
     account_create_op.name = name;
     account_create_op.owner = authority(1234, public_key_type(key.get_public_key()), weight_type(1234));
     account_create_op.active = authority(5678, public_key_type(key.get_public_key()), weight_type(5678));
     account_create_op.options.memo_key = key.get_public_key();
     account_create_op.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
     trx.operations.push_back( account_create_op );

     trx.validate();

     processed_transaction ptx = db.push_transaction(trx, ~0);
     //wdump( (ptx) );
     const account_object& result = db.get<account_object>(ptx.operation_results[0].get<object_id_type>());
     trx.operations.clear();
     return result;
  }
  FC_CAPTURE_AND_RETHROW( (name)(registrar_id)(referrer_id) )
}



account_create_operation database_fixture::make_account(
     const std::string& name /* = "nathan" */,
     public_key_type key /* = key_id_type() */
)
{ try {
     account_create_operation create_account;
     create_account.registrar = account_id_type();

     create_account.name = name;
     create_account.owner = authority(123, key, weight_type(123));
     create_account.active = authority(321, key, weight_type(321));
     create_account.options.memo_key = key;
     create_account.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;

     auto& active_miners = db.get_global_properties().active_miners;
     if( active_miners.size() > 0 )
     {
        std::set<vote_id_type> votes;
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        create_account.options.votes = boost::container::flat_set<vote_id_type>(votes.begin(), votes.end());
     }
     create_account.options.num_miner = static_cast<uint16_t>(create_account.options.votes.size());

     create_account.fee = db.current_fee_schedule().calculate_fee( create_account );
     return create_account;
  } FC_RETHROW() }

account_create_operation database_fixture::make_account(
     const std::string& name,
     const account_object& registrar,
     const account_object& referrer,
     uint8_t referrer_percent /* = 100 */,
     public_key_type key /* = public_key_type() */
)
{
  try
  {
     account_create_operation          create_account;
     create_account.registrar          = registrar.id;
     //create_account.referrer           = referrer.id;
     //create_account.referrer_percent   = referrer_percent;

     create_account.name = name;
     create_account.owner = authority(123, key, weight_type(123));
     create_account.active = authority(321, key, weight_type(321));
     create_account.options.memo_key = key;
     create_account.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;

     const std::vector<miner_id_type>& active_miners = db.get_global_properties().active_miners;
     if( active_miners.size() > 0 )
     {
        std::set<vote_id_type> votes;
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        votes.insert(active_miners[rand() % active_miners.size()](db).vote_id);
        create_account.options.votes = boost::container::flat_set<vote_id_type>(votes.begin(), votes.end());
     }
     create_account.options.num_miner = static_cast<uint16_t>(create_account.options.votes.size());

     create_account.fee = db.current_fee_schedule().calculate_fee( create_account );
     return create_account;
  }
  FC_CAPTURE_AND_RETHROW((name)(referrer_percent))
}

digest_type database_fixture::digest( const transaction& tx )
{
  return tx.digest();
}

void database_fixture::open_database()
{
  if( !data_dir ) {
     data_dir = fc::temp_directory( graphene::utilities::temp_directory_path() );
     db.open(data_dir->path(), [this]{return genesis_state;});
  }


   //TODO: make it better
//   fc::path data_dir = fc::temp_directory_path();
//   data_dir /= fc::path(".decent_test");
//
//   db.open(data_dir, [this]{return genesis_state;});
//

//  if( !data_dir ) {
//     data_dir = fc::temp_directory( graphene::utilities::temp_directory_path() );
//     db.open(data_dir->path(), [this]{return genesis_state;});
//  }
}

signed_block database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
  skip |= database::skip_undo_history_check;
  // skip == ~0 will skip checks specified in database::validation_steps
  auto block = db.generate_block(db.get_slot_time(miss_blocks + 1),
                                 db.get_scheduled_miner(miss_blocks + 1),
                                 key, skip);
  db.clear_pending();
  return block;
}

void database_fixture::generate_blocks( uint32_t block_count )
{
  for( uint32_t i = 0; i < block_count; ++i )
     generate_block();
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks, uint32_t skip)
{
  if( miss_intermediate_blocks )
  {
     generate_block(skip);
     auto slots_to_miss = db.get_slot_at_time(timestamp);
     if( slots_to_miss <= 1 )
        return;
     --slots_to_miss;
     generate_block(skip, init_account_priv_key, slots_to_miss);
     return;
  }
  while( db.head_block_time() < timestamp )
     generate_block(skip);
}

const asset_object& database_fixture::create_monitored_asset(
     const std::string& name,
     account_id_type issuer /* = GRAPHENE_MINER_ACCOUNT */ )
{ try {
  asset_create_operation creator;
  creator.issuer = issuer;
  creator.fee = asset();
  creator.symbol = name;
  creator.options.max_supply = 0;  //MIA allways with zero
  creator.precision = 2;
  creator.options.core_exchange_rate = price({asset(1,asset_id_type(1)),asset(1)});
  creator.monitored_asset_opts = monitored_asset_options();
  trx.operations.push_back(std::move(creator));
  trx.validate();
  processed_transaction ptx = db.push_transaction(trx, ~0);
  trx.operations.clear();
  return db.get<asset_object>(ptx.operation_results[0].get<object_id_type>());
} FC_CAPTURE_AND_RETHROW( (name) ) }

const asset_object& database_fixture::create_user_issued_asset( const std::string& name )
{
  asset_create_operation creator;
  creator.issuer = account_id_type();
  creator.fee = asset();
  creator.symbol = name;
  creator.options.max_supply = 0;
  creator.precision = 2;
  creator.options.core_exchange_rate = price({asset(1,asset_id_type(1)),asset(1)});
  creator.monitored_asset_opts = monitored_asset_options();
  trx.operations.push_back(std::move(creator));
  trx.validate();
  processed_transaction ptx = db.push_transaction(trx, ~0);
  trx.operations.clear();
  return db.get<asset_object>(ptx.operation_results[0].get<object_id_type>());
}

const asset_object& database_fixture::create_user_issued_asset( const std::string& name, const account_object& issuer )
{
  asset_create_operation creator;
  creator.issuer = issuer.id;
  creator.fee = asset();
  creator.symbol = name;
  creator.precision = 2;
  creator.options.core_exchange_rate = price({asset(1,asset_id_type(1)),asset(1)});
  creator.options.max_supply = GRAPHENE_MAX_SHARE_SUPPLY;
  trx.operations.clear();
  trx.operations.push_back(std::move(creator));
  set_expiration( db, trx );
  trx.validate();
  processed_transaction ptx = db.push_transaction(trx, ~0);
  trx.operations.clear();
  return db.get<asset_object>(ptx.operation_results[0].get<object_id_type>());
}


const miner_object&database_fixture::create_miner(account_id_type owner, const fc::ecc::private_key& signing_private_key)
{
  return create_miner(owner(db), signing_private_key);
}

const miner_object& database_fixture::create_miner( const account_object& owner,
                                                   const fc::ecc::private_key& signing_private_key )
{ try {
     miner_create_operation op;
     op.miner_account = owner.id;
     op.block_signing_key = signing_private_key.get_public_key();
     trx.operations.push_back(op);
     trx.validate();
     processed_transaction ptx = db.push_transaction(trx, ~0);
     trx.clear();
     return db.get<miner_object>(ptx.operation_results[0].get<object_id_type>());
  } FC_RETHROW()
}

int64_t database_fixture::get_balance( account_id_type account, asset_id_type a )const
{
  return db.get_balance(account, a).amount.value;
}

int64_t database_fixture::get_balance( const account_object& account, const asset_object& a )const
{
  return db.get_balance(account.get_id(), a.get_id()).amount.value;
}

const asset_object& database_fixture::get_asset( const std::string& symbol )const
{
  const auto& idx = db.get_index_type<asset_index>().indices().get<by_symbol>();
  const auto itr = idx.find(symbol);
  assert( itr != idx.end() );
  return *itr;
}

const account_object& database_fixture::get_account( const std::string& name )const
{
  const auto& idx = db.get_index_type<account_index>().indices().get<by_name>();
  const auto itr = idx.find(name);
  assert( itr != idx.end() );
  return *itr;
}

const account_object& database_fixture::get_account_by_id(account_id_type id)const
{
   const auto& idx = db.get_index_type<account_index>().indices().get<by_id>();
   const auto itr = idx.find(id);
   assert(itr != idx.end());
   return *itr;
}

const miner_object& database_fixture::get_miner(account_id_type id)const
{
   //const auto& idx = db.get_index_type<miner_index>().indices().get<by_name>();
   //const auto itr = idx.find(name);
   //assert(itr != idx.end());
   //return *itr;
   const auto& all_miners = db.get_index_type<miner_index>().indices();
   for (const miner_object& wit : all_miners)
   {
      if (id == wit.miner_account)
         return wit;
   }
   FC_THROW("Miner not found: ${m}", ("m", id));
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
  trx.sign( key, db.get_chain_id() );
}

void database_fixture::transfer(
     account_id_type from,
     account_id_type to,
     const asset& amount,
     const asset& fee /* = asset() */
)
{
  transfer(from(db), to(db), amount, fee);
}

void database_fixture::transfer(
     const account_object& from,
     const account_object& to,
     const asset& amount,
     const asset& fee /* = asset() */ )
{
  try
  {
     set_expiration( db, trx );
     transfer_obsolete_operation trans;
     trans.fee = fee;
     trans.from = from.id;
     trans.to   = to.id;
     trans.amount = amount;
     trx.operations.push_back(trans);

     if( fee == asset() )
     {
        for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
     }
     trx.validate();
     db.push_transaction(trx, ~0);
     verify_asset_supplies(db);
     trx.operations.clear();
  } FC_CAPTURE_AND_RETHROW( (from.id)(to.id)(amount)(fee) )
}

void database_fixture::enable_fees()
{
  db.modify(global_property_id_type()(db), [](global_property_object& gpo)
  {
      gpo.parameters.current_fees = fee_schedule::get_default();
  });
}

uint64_t database_fixture::fund(
     const account_object& account,
     const asset& amount /* = asset(500000) */ )
{
  transfer(account_id_type()(db), account, amount);
  return get_balance(account, amount.asset_id(db));
}

std::string database_fixture::generate_anon_acct_name()
{
  // names of the form "anon-acct-x123" ; the "x" is necessary
  //    to workaround issue #46
  return "anon-acct-x" + std::to_string( anon_acct_count++ );
}

void database_fixture::issue_uia( const account_object& recipient, asset amount )
{
  BOOST_TEST_MESSAGE( "Issuing UIA" );
  asset_issue_operation op;
  op.issuer = amount.asset_id(db).issuer;
  op.asset_to_issue = amount;
  op.issue_to_account = recipient.id;
  trx.operations.push_back(op);
  db.push_transaction( trx, ~0 );
  trx.operations.clear();
}

void database_fixture::issue_uia( account_id_type recipient_id, asset amount )
{
  issue_uia( recipient_id(db), amount );
}

void database_fixture::fill_pools(asset_id_type uia, account_id_type by, asset to_core_pool, asset to_asset_pool)
{
   set_expiration( db, trx );
   trx.operations.clear();
   asset_fund_pools_operation filler;
   filler.dct_asset = to_core_pool;
   filler.from_account = by;
   filler.uia_asset = to_asset_pool;
   trx.operations.push_back(std::move(filler));
   db.push_transaction( trx, ~0 );
   trx.operations.clear();
}

void database_fixture::publish_feed( const asset_object& mia, const account_object& by, const price_feed& f )
{
  set_expiration( db, trx );
  trx.operations.clear();

  asset_publish_feed_operation op;
  op.publisher = by.id;
  op.asset_id = mia.id;
  op.feed = f;
  if( op.feed.core_exchange_rate.is_null() )
     op.feed.core_exchange_rate = price(asset(1, op.feed.core_exchange_rate.base.asset_id), asset(1, op.feed.core_exchange_rate.quote.asset_id));
  trx.operations.emplace_back( std::move(op) );

  for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
  trx.validate();
  db.push_transaction(trx, ~0);
  trx.operations.clear();
  verify_asset_supplies(db);
}

void database_fixture::create_content(account_id_type by, const std::string& url, asset price, std::map<account_id_type, uint32_t> co_authors)
{
   set_expiration( db, trx );
   trx.operations.clear();

   content_submit_operation op;
   op.size = 100;
   op.price.push_back({RegionCodes::OO_none, price});
   op.author = by;
   op.co_authors = co_authors;
   op.URI = url;
   op.hash = fc::ripemd160::hash(url);
   op.expiration = fc::time_point::now()+fc::microseconds(10000000000);
   op.publishing_fee = asset(0);
   op.quorum = 0;
   op.synopsis = "{\"title\":\"abcd\"}";
   trx.operations.emplace_back( std::move(op) );

   db.push_transaction(trx, ~0);
   trx.operations.clear();
}

void database_fixture::buy_content(account_id_type by, const std::string& url, asset price)
{
   set_expiration( db, trx );
   trx.operations.clear();

   request_to_buy_operation op;
   op.URI = url;
   op.price = price;
   op.consumer = by;

   trx.operations.emplace_back( std::move(op) );

   db.push_transaction(trx, ~0);
   trx.operations.clear();
}


#if 0  ///////////////////////////////////////////////////////////////////////////////////////




fc::ecc::private_key database_fixture::generate_private_key(string seed)
{
   static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));
   if( seed == "null_key" )
      return committee;
   return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}




void database_fixture::verify_account_history_plugin_index( )const
{
   return;
   if( skip_key_index_test )
      return;

   const std::shared_ptr<graphene::account_history::account_history_plugin> pin =
      app.get_plugin<graphene::account_history::account_history_plugin>("account_history");

   return;
}

const limit_order_object*database_fixture::create_sell_order(account_id_type user, const asset& amount, const asset& recv)
{
  auto r =  create_sell_order(user(db), amount, recv);
  verify_asset_supplies(db);
  return r;
}

const limit_order_object* database_fixture::create_sell_order( const account_object& user, const asset& amount, const asset& recv )
{
  //wdump((amount)(recv));
  limit_order_create_operation buy_order;
  buy_order.seller = user.id;
  buy_order.amount_to_sell = amount;
  buy_order.min_to_receive = recv;
  trx.operations.push_back(buy_order);
  for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
  trx.validate();
  auto processed = db.push_transaction(trx, ~0);
  trx.operations.clear();
  verify_asset_supplies(db);
  //wdump((processed));
  return db.find<limit_order_object>( processed.operation_results[0].get<object_id_type>() );
}







void database_fixture::change_fees(
   const flat_set< fee_parameters >& new_params,
   uint32_t new_scale /* = 0 */
   )
{
   const chain_parameters& current_chain_params = db.get_global_properties().parameters;
   const fee_schedule& current_fees = *(current_chain_params.current_fees);

   flat_map< int, fee_parameters > fee_map;
   fee_map.reserve( current_fees.parameters.size() );
   for( const fee_parameters& op_fee : current_fees.parameters )
      fee_map[ op_fee.which() ] = op_fee;
   for( const fee_parameters& new_fee : new_params )
      fee_map[ new_fee.which() ] = new_fee;

   fee_schedule_type new_fees;

   for( const std::pair< int, fee_parameters >& item : fee_map )
      new_fees.parameters.insert( item.second );
   if( new_scale != 0 )
      new_fees.scale = new_scale;

   chain_parameters new_chain_params = current_chain_params;
   new_chain_params.current_fees = new_fees;

   db.modify(db.get_global_properties(), [&](global_property_object& p) {
      p.parameters = new_chain_params;
   });
}



asset database_fixture::cancel_limit_order( const limit_order_object& order )
{
  limit_order_cancel_operation cancel_order;
  cancel_order.fee_paying_account = order.seller;
  cancel_order.order = order.id;
  trx.operations.push_back(cancel_order);
  for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
  trx.validate();
  auto processed = db.push_transaction(trx, ~0);
  trx.operations.clear();
   verify_asset_supplies(db);
  return processed.operation_results[0].get<asset>();
}




void database_fixture::print_market( const string& syma, const string& symb )const
{
   const auto& limit_idx = db.get_index_type<limit_order_index>();
   const auto& price_idx = limit_idx.indices().get<by_price>();

   cerr << std::fixed;
   cerr.precision(5);
   cerr << std::setw(10) << std::left  << "NAME"      << " ";
   cerr << std::setw(16) << std::right << "FOR SALE"  << " ";
   cerr << std::setw(16) << std::right << "FOR WHAT"  << " ";
   cerr << std::setw(10) << std::right << "PRICE (S/W)"   << " ";
   cerr << std::setw(10) << std::right << "1/PRICE (W/S)" << "\n";
   cerr << string(70, '=') << std::endl;
   auto cur = price_idx.begin();
   while( cur != price_idx.end() )
   {
      cerr << std::setw( 10 ) << std::left   << cur->seller(db).name << " ";
      cerr << std::setw( 10 ) << std::right  << cur->for_sale.value << " ";
      cerr << std::setw( 5 )  << std::left   << cur->amount_for_sale().asset_id(db).symbol << " ";
      cerr << std::setw( 10 ) << std::right  << cur->amount_to_receive().amount.value << " ";
      cerr << std::setw( 5 )  << std::left   << cur->amount_to_receive().asset_id(db).symbol << " ";
      cerr << std::setw( 10 ) << std::right  << cur->sell_price.to_real() << " ";
      cerr << std::setw( 10 ) << std::right  << (~cur->sell_price).to_real() << " ";
      cerr << "\n";
      ++cur;
   }
}

string database_fixture::pretty( const asset& a )const
{
  std::stringstream ss;
  ss << a.amount.value << " ";
  ss << a.asset_id(db).symbol;
  return ss.str();
}

void database_fixture::print_limit_order( const limit_order_object& cur )const
{
  std::cout << std::setw(10) << cur.seller(db).name << " ";
  std::cout << std::setw(10) << "LIMIT" << " ";
  std::cout << std::setw(16) << pretty( cur.amount_for_sale() ) << " ";
  std::cout << std::setw(16) << pretty( cur.amount_to_receive() ) << " ";
  std::cout << std::setw(16) << cur.sell_price.to_real() << " ";
}

void database_fixture::print_joint_market( const string& syma, const string& symb )const
{
  cout << std::fixed;
  cout.precision(5);

  cout << std::setw(10) << std::left  << "NAME"      << " ";
  cout << std::setw(10) << std::right << "TYPE"      << " ";
  cout << std::setw(16) << std::right << "FOR SALE"  << " ";
  cout << std::setw(16) << std::right << "FOR WHAT"  << " ";
  cout << std::setw(16) << std::right << "PRICE (S/W)" << "\n";
  cout << string(70, '=');

  const auto& limit_idx = db.get_index_type<limit_order_index>();
  const auto& limit_price_idx = limit_idx.indices().get<by_price>();

  auto limit_itr = limit_price_idx.begin();
  while( limit_itr != limit_price_idx.end() )
  {
     std::cout << std::endl;
     print_limit_order( *limit_itr );
     ++limit_itr;
  }
}

vector< operation_history_object > database_fixture::get_operation_history( account_id_type account_id )const
{
   vector< operation_history_object > result;
   const auto& stats = account_id(db).statistics(db);
   if(stats.most_recent_op == account_transaction_history_id_type())
      return result;

   const account_transaction_history_object* node = &stats.most_recent_op(db);
   while( true )
   {
      result.push_back( node->operation_id(db) );
      if(node->next == account_transaction_history_id_type())
         break;
      node = db.find(node->next);
   }
   return result;
}

#endif


namespace test {

void set_expiration( const database& db, transaction& tx )
{
   const chain_parameters& params = db.get_global_properties().parameters;
   tx.set_reference_block(db.head_block_id());
   tx.set_expiration( db.head_block_time() + fc::seconds( params.block_interval * (params.maintenance_skip_slots + 1) * 3 ) );
   return;
}

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags /* = 0 */ )
{
   return db.push_block( b, skip_flags);
}

processed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */ )
{ try {
   auto pt = db.push_transaction( tx, skip_flags );
   database_fixture::verify_asset_supplies(db);
   return pt;
} FC_CAPTURE_AND_RETHROW((tx)) }



} // graphene::chain::test

} } // graphene::chain
