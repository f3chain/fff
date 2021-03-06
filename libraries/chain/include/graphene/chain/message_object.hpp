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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene {
   namespace chain {

   class message_object_receivers_data
   {
   public:
      /**
       * @brief Decrypt message
       * @param priv the private key of sender/receiver
       * @param pub the public key of receiver/sender
       * @return decrypted message
       */
      std::string get_message(const private_key_type& priv, const public_key_type& pub) const;

      account_id_type receiver;
      public_key_type receiver_pubkey;
      uint64_t nonce = 0;
      std::vector<char> data;
   };

   class message_object : public graphene::db::abstract_object<implementation_ids, impl_messaging_object_type, message_object>
   {
   public:
      fc::time_point_sec created;
      account_id_type sender;
      public_key_type sender_pubkey;

      std::vector<message_object_receivers_data> receivers_data;
   };

   class message_receiver_index : public graphene::db::secondary_index
   {
   public:
      virtual void object_inserted(const graphene::db::object& obj) override;
      virtual void object_removed(const graphene::db::object& obj) override;
      virtual void about_to_modify(const graphene::db::object& before) override;
      virtual void object_modified(const graphene::db::object& after) override;

      std::map< account_id_type, std::set<graphene::db::object_id_type> > message_to_receiver_memberships;
   };

   struct by_sender;
   typedef boost::multi_index_container<
      message_object,
      db::mi::indexed_by<
         db::object_id_index,
         db::mi::ordered_non_unique<db::mi::tag<by_sender>,
            db::mi::member<message_object, account_id_type, &message_object::sender>
         >
      >
   > message_multi_index_type;

   typedef graphene::db::generic_index<message_object, message_multi_index_type> message_index;
   }
} // namespaces

FC_REFLECT(
   graphene::chain::message_object_receivers_data,
   (receiver)
   (receiver_pubkey)
   (nonce)
   (data)
)

FC_REFLECT_DERIVED(
   graphene::chain::message_object,
   (graphene::db::object),
   (created)
   (sender)
   (sender_pubkey)
   (receivers_data)
)
