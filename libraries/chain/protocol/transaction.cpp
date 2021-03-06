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
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/block.hpp>
#include <graphene/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>

namespace graphene { namespace chain {

digest_type processed_transaction::merkle_digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::sig_digest( const chain_id_type& chain_id )const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return enc.result();
}

void transaction::validate() const
{
   if(operations.size() == 0)
      FC_THROW_EXCEPTION(trx_must_have_at_least_one_op_exception, "Trx: ${trx}", ("trx", *this));

   int index = 0;
   try {
      for(const auto& op : operations) {
         operation_validate(op);
         index++;
      }
   } FC_REWRAP_EXCEPTIONS(operation_validate_exception, error, "Zero based index of operation ${index}", ("index", index))
}

graphene::chain::transaction_id_type graphene::chain::transaction::id() const
{
   auto h = digest();
   transaction_id_type result;
   memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
   return result;
}

const signature_type& graphene::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)
{
   signatures.push_back(signature(key, chain_id));
   return signatures.back();
}

signature_type graphene::chain::signed_transaction::signature(const private_key_type& key, const chain_id_type& chain_id) const
{
   return key.sign_compact(sig_digest(chain_id));
}

void transaction::set_expiration( fc::time_point_sec expiration_time )
{
    expiration = expiration_time;
}

void transaction::set_reference_block( const block_id_type& reference_block )
{
   ref_block_num = static_cast<uint16_t>(block_header::num_from_id(reference_block));
   ref_block_prefix = reference_block._hash[1];
}

void transaction::get_required_authorities( boost::container::flat_set<account_id_type>& active,
                                            boost::container::flat_set<account_id_type>& owner,
                                            std::vector<authority>& other )const
{
   for( const auto& op : operations )
      operation_get_required_authorities( op, active, owner, other );
}

struct sign_state
{
      /** returns true if we have a signature for this key or can
       * produce a signature for this key, else returns false.
       */
      bool signed_by( const public_key_type& k )
      {
         auto itr = provided_signatures.find(k);
         if( itr == provided_signatures.end() )
         {
            auto pk = available_keys.find(k);
            if( pk  != available_keys.end() )
               return provided_signatures[k] = true;
            return false;
         }
         return itr->second = true;
      }


      bool check_authority( account_id_type id )
      {
         if( approved_by.find(id) != approved_by.end() ) return true;
         return check_authority( get_active(id) );
      }

      /**
       *  Checks to see if we have signatures of the active authorites of
       *  the accounts specified in authority or the keys specified.
       */
      bool check_authority( const authority* au, uint32_t depth = 0 )
      {
         if( au == nullptr ) return false;
         const authority& auth = *au;

         uint32_t total_weight = 0;
         for( const auto& k : auth.key_auths )
            if( signed_by( k.first ) )
            {
               total_weight += k.second;
               if( total_weight >= auth.weight_threshold )
                  return true;
            }


         for( const auto& a : auth.account_auths )
         {
            if( approved_by.find(a.first) == approved_by.end() )
            {
               if( depth == max_recursion )
                  return false;
               if( check_authority( get_active( a.first ), depth+1 ) )
               {
                  approved_by.insert( a.first );
                  total_weight += a.second;
                  if( total_weight >= auth.weight_threshold )
                     return true;
               }
            }
            else
            {
               total_weight += a.second;
               if( total_weight >= auth.weight_threshold )
                  return true;
            }
         }
         return total_weight >= auth.weight_threshold;
      }

      bool remove_unused_signatures()
      {
         std::vector<public_key_type> remove_sigs;
         for( const auto& sig : provided_signatures )
            if( !sig.second ) remove_sigs.push_back( sig.first );

         for( auto& sig : remove_sigs )
            provided_signatures.erase(sig);

         return remove_sigs.size() != 0;
      }

      sign_state( const boost::container::flat_set<public_key_type>& sigs,
                  const std::function<const authority*(account_id_type)>& a )
      :get_active(a)
      {
         for( const auto& key : sigs )
            provided_signatures[ key ] = false;
         approved_by.insert( GRAPHENE_TEMP_ACCOUNT  );
      }

      const std::function<const authority*(account_id_type)>& get_active;

      boost::container::flat_set<public_key_type>        available_keys;
      boost::container::flat_map<public_key_type,bool>   provided_signatures;
      boost::container::flat_set<account_id_type>        approved_by;
      uint32_t                         max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH;
};

// optimized version with only one sign
struct sign_state1
{
   /** returns true if we have a signature for this key or can
   * produce a signature for this key, else returns false.
   */
   bool signed_by(const public_key_type& k)
   {
      if (k == provided_signature_key)
         return true;
      return false;
   }

   bool check_authority(account_id_type id)
   {
      if (approved_by.find(id) != approved_by.end()) return true;
         return check_authority(get_active(id));
   }

   /**
    *  Checks to see if we have signatures of the active authorites of
    *  the accounts specified in authority or the keys specified.
    */
   bool check_authority(const authority* au, uint32_t depth = 0)
   {
      if (au == nullptr) return false;
      const authority& auth = *au;
      uint32_t total_weight = 0;
      for (const auto& k : auth.key_auths)
         if (signed_by(k.first))
         {
            total_weight += k.second;
            if (total_weight >= auth.weight_threshold)
               return true;
         }

      for (const auto& a : auth.account_auths)
      {
         if (approved_by.find(a.first) == approved_by.end())
         {
            if (depth == max_recursion)
               return false;
            if (check_authority(get_active(a.first), depth + 1))
              {
               approved_by.insert(a.first);
               total_weight += a.second;
               if (total_weight >= auth.weight_threshold)
                  return true;
              }
           }
         else
         {
            total_weight += a.second;
            if (total_weight >= auth.weight_threshold)
               return true;
         }
      }
      return total_weight >= auth.weight_threshold;
   }

   bool remove_unused_signatures()
   {
      return false;
   }

   sign_state1(const public_key_type& sig,
      const std::function<const authority*(account_id_type)>& a)
      :get_active(a),provided_signature_key(sig)
   {
      provided_signature = false;
      approved_by.insert(GRAPHENE_TEMP_ACCOUNT);
   }

   const std::function<const authority*(account_id_type)>& get_active;

   const public_key_type&           provided_signature_key;
   bool                             provided_signature;
   boost::container::flat_set<account_id_type> approved_by;
   uint32_t                         max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH;
};

void verify_authority( const std::vector<operation>& ops,
                       const boost::container::flat_set<public_key_type>& sigs,
                       const std::function<const authority*(account_id_type)>& get_active,
                       const std::function<const authority*(account_id_type)>& get_owner,
                       uint32_t max_recursion_depth,
                       bool  allow_committee,
                       const boost::container::flat_set<account_id_type>& active_aprovals,
                       const boost::container::flat_set<account_id_type>& owner_approvals )
{ try {
   boost::container::flat_set<account_id_type> required_active;
   boost::container::flat_set<account_id_type> required_owner;
   std::vector<authority> other;

   for( const auto& op : ops )
      operation_get_required_authorities( op, required_active, required_owner, other );

   if( !allow_committee )
      FC_VERIFY_AND_THROW( required_active.find(GRAPHENE_MINER_ACCOUNT) == required_active.end(),
                       invalid_committee_approval_exception, "Committee account may only propose transactions" );

   sign_state s(sigs,get_active);
   s.max_recursion = max_recursion_depth;
   for( auto& id : active_aprovals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      FC_VERIFY_AND_THROW( s.check_authority(&auth), tx_missing_other_auth_exception, "Missing Authority ${auth}", ("auth",auth) );
   }

   // fetch all of the top level authorities
   for( auto id : required_active )
   {
      FC_VERIFY_AND_THROW( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                       tx_missing_active_auth_exception, "Missing Active Authority ${id}", ("id",id) );
   }

   for( auto id : required_owner )
   {
      FC_VERIFY_AND_THROW( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth_exception, "Missing Owner Authority ${id}", ("id",id) );
   }

   FC_VERIFY_AND_THROW(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig_exception,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }

void verify_authority1(const std::vector<operation>& ops, const public_key_type& sigs,
  const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion_depth)
{
   try {
      boost::container::flat_set<account_id_type> required_active;
      boost::container::flat_set<account_id_type> required_owner;
      std::vector<authority> other;

      for (const auto& op : ops)
          operation_get_required_authorities(op, required_active, required_owner, other);

      sign_state1 s(sigs, get_active);
      s.max_recursion = max_recursion_depth;

      for (const auto& auth : other)
      {
         FC_VERIFY_AND_THROW(s.check_authority(&auth), tx_missing_other_auth_exception, "Missing Authority", ("auth", auth));
      }

      // fetch all of the top level authorities
      for (auto id : required_active)
      {
         FC_VERIFY_AND_THROW(s.check_authority(id) ||
            s.check_authority(get_owner(id)),
            tx_missing_active_auth_exception, "Missing Active Authority ${id}", ("id", id));
      }

      for (auto id : required_owner)
      {
         FC_VERIFY_AND_THROW(
            s.check_authority(get_owner(id)),
            tx_missing_owner_auth_exception, "Missing Owner Authority ${id}", ("id", id));
      }
   } FC_CAPTURE_AND_RETHROW((ops)(sigs))
}

boost::container::flat_set<public_key_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
   auto d = sig_digest( chain_id );
   boost::container::flat_set<public_key_type> result;
   for( const auto&  sig : signatures )
   {
      FC_VERIFY_AND_THROW(
         result.insert( fc::ecc::public_key(sig,d) ).second,
         tx_duplicate_sig_exception,
         "Duplicate Signature detected" );
   }
   return result;
} FC_RETHROW() }

std::set<public_key_type> signed_transaction::get_required_signatures(
   const chain_id_type& chain_id,
   const boost::container::flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion_depth )const
{
   boost::container::flat_set<account_id_type> required_active;
   boost::container::flat_set<account_id_type> required_owner;
   std::vector<authority> other;
   get_required_authorities( required_active, required_owner, other );

   sign_state s(get_signature_keys( chain_id ),get_active);
   s.available_keys = available_keys;
   s.max_recursion = max_recursion_depth;

   for( const auto& auth : other )
      s.check_authority(&auth);
   for( auto& owner : required_owner )
      s.check_authority( get_owner( owner ) );
   for( auto& active : required_active )
      s.check_authority( active  );

   s.remove_unused_signatures();

   std::set<public_key_type> result;

   for( auto& provided_sig : s.provided_signatures )
      if( available_keys.find( provided_sig.first ) != available_keys.end() )
         result.insert( provided_sig.first );

   return result;
}

std::set<public_key_type> signed_transaction::minimize_required_signatures(
   const chain_id_type& chain_id,
   const boost::container::flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion
   ) const
{
   std::set<public_key_type> s = get_required_signatures( chain_id, available_keys, get_active, get_owner, max_recursion );
   boost::container::flat_set<public_key_type> result( s.begin(), s.end() );

   for( const public_key_type& k : s )
   {
      result.erase( k );
      try
      {
         graphene::chain::verify_authority( operations, result, get_active, get_owner, max_recursion );
         continue;  // element stays erased if verify_authority is ok
      }
      catch( const tx_missing_owner_auth_exception& ) {}
      catch( const tx_missing_active_auth_exception& ) {}
      catch( const tx_missing_other_auth_exception& ) {}
      result.insert( k );
   }
   return std::set<public_key_type>( result.begin(), result.end() );
}

void signed_transaction::verify_authority(
   const boost::container::flat_set<public_key_type>& sig_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion )const

{ try {
   if(sig_keys.size() == 1)
      graphene::chain::verify_authority1(operations, *sig_keys.begin(), get_active, get_owner, max_recursion);
   else
      graphene::chain::verify_authority(operations, sig_keys, get_active, get_owner, max_recursion);
} FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // graphene::chain
