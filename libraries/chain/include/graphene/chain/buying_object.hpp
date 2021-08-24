/* (c) 2016, 2021 FFF Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/time.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/json.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

   class buying_object : public graphene::db::abstract_object<implementation_ids, impl_buying_object_type, buying_object>
   {
   public:
      account_id_type consumer;
      std::string URI;
      uint64_t size = 0; //< initialized by content.size
      uint32_t rating = 0;  //< this is the user rating
      std::string comment;
      asset price;  //< this is an escrow, initialized by request_to_buy_operation.price then reset to 0 for escrow system and inflation calculations
      asset paid_price_before_exchange; //< initialized by request_to_buy_operation.price
      asset paid_price_after_exchange;
      std::string synopsis;   //< initialized by content.synopsis
      std::vector<account_id_type> seeders_answered;
      std::vector<ciphertext_type> key_particles;
      bigint_type pubKey;
      fc::time_point_sec expiration_time;
      bool expired = false;
      bool delivered = false;
      fc::time_point_sec expiration_or_delivery_time;
      // User can't add rating and comment in two time-separated steps. For example, if content is already rated by user, he is not
      // allowed to add comment later. If user wants to add both rating and comment, he has to do it in one step.
      bool rated_or_commented = false;
      fc::time_point_sec created; //< initialized by content.created
      uint32_t region_code_from;

      bool is_open() const { return !( expired || delivered ); }
      bool is_rated() const { return rated_or_commented; }
      share_type get_price_before_exchange() const { return paid_price_before_exchange.amount; }
   };

   struct by_URI_consumer;
   struct by_consumer_URI;
   struct by_expiration_time;
   struct by_consumer_time;
   struct by_URI_open;
   struct by_URI_rated;
   struct by_open_expiration;
   struct by_consumer_open;
   struct by_size;
   struct by_price_before_exchange;
   struct by_created;
   struct by_purchased;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<by_size, buying_object>
   {
      static uint64_t get(buying_object const& ob)
      {
         return ob.size;
      }
   };

   template <>
   struct key_extractor<by_price_before_exchange, buying_object>
   {
      static share_type get(buying_object const& ob)
      {
         return ob.get_price_before_exchange();
      }
   };

   template <>
   struct key_extractor<by_created, buying_object>
   {
      static fc::time_point_sec get(buying_object const& ob)
      {
         return ob.created;
      }
   };

   template<>
   struct key_extractor<by_purchased, buying_object>
   {
      static fc::time_point_sec get(buying_object const& ob)
      {
         return ob.expiration_or_delivery_time;
      }
   };

   template <>
   struct key_extractor<by_consumer_open, buying_object>
   {
      static boost::tuple<account_id_type, bool> get(buying_object const& ob)
      {
         return boost::make_tuple(ob.consumer, ob.is_open());
      }
   };

   template <>
   struct key_extractor<by_URI_rated, buying_object>
   {
      static std::tuple<std::string, uint32_t> get(buying_object const& ob)
      {
         return std::make_tuple(ob.URI, ob.rating);
      }
   };

   typedef boost::multi_index_container<
      buying_object,
      db::mi::indexed_by<
         db::object_id_index,
         db::mi::ordered_unique<db::mi::tag<by_URI_consumer>,
            db::mi::composite_key<buying_object,
               db::mi::member<buying_object, std::string, &buying_object::URI>,
               db::mi::member<buying_object, account_id_type, &buying_object::consumer>
            >
         >,
         db::mi::ordered_unique<db::mi::tag<by_consumer_URI>,
            db::mi::composite_key<buying_object,
               db::mi::member<buying_object, account_id_type, &buying_object::consumer>,
               db::mi::member<buying_object, std::string, &buying_object::URI>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_expiration_time>,
            db::mi::member<buying_object, fc::time_point_sec, &buying_object::expiration_time>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_consumer_time>,
            db::mi::composite_key<buying_object,
               db::mi::member<buying_object, account_id_type, &buying_object::consumer>,
               db::mi::member<buying_object, fc::time_point_sec, &buying_object::expiration_or_delivery_time>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_URI_open>,
            db::mi::composite_key<buying_object,
               db::mi::member<buying_object, std::string, &buying_object::URI>,
               db::mi::const_mem_fun<buying_object, bool, &buying_object::is_open>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_URI_rated>,
            db::mi::composite_key<buying_object,
               db::mi::member<buying_object, std::string, &buying_object::URI>,
               db::mi::const_mem_fun<buying_object, bool, &buying_object::is_rated>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_open_expiration>,
            db::mi::composite_key<buying_object,
               db::mi::const_mem_fun<buying_object, bool, &buying_object::is_open>,
               db::mi::member<buying_object, fc::time_point_sec, &buying_object::expiration_time>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_consumer_open>,
            db::mi::composite_key<buying_object,
               db::mi::member<buying_object, account_id_type, &buying_object::consumer>,
               db::mi::const_mem_fun<buying_object, bool, &buying_object::is_open>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_size>,
               db::mi::member<buying_object, uint64_t, &buying_object::size>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_price_before_exchange>,
               db::mi::const_mem_fun<buying_object, share_type, &buying_object::get_price_before_exchange>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_created>,
               db::mi::member<buying_object, fc::time_point_sec, &buying_object::created>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_purchased>,
               db::mi::member<buying_object, fc::time_point_sec, &buying_object::expiration_or_delivery_time>
         >
      >
   >buying_object_multi_index_type;

   typedef graphene::db::generic_index< buying_object, buying_object_multi_index_type > buying_index;

}}

FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(synopsis)(price)(paid_price_before_exchange)(paid_price_after_exchange)(seeders_answered)(size)(rating)(comment)(expiration_time)(pubKey)(key_particles)
                   (expired)(delivered)(expiration_or_delivery_time)(rated_or_commented)(created)(region_code_from) )
