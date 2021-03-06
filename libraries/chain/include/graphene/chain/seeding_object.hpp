/* (c) 2016, 2021 FFF Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

namespace graphene { namespace chain {

class seeding_object : public graphene::db::abstract_object<local_ids, local_seeding_object_type, seeding_object>
{
public:
   std::string URI; //<Content address
   fc::ripemd160 _hash; //<Content hash
   fc::time_point_sec expiration; //<Content expiration
   fc::optional<decent::encrypt::CustodyData> cd; //<Content custody data

   account_id_type seeder; //<Seeder seeding this content managed by this plugin
   decent::encrypt::CiphertextString key; //<Decryption key part

   uint64_t size = 0;
   bool downloaded = false;
   bool deleted = false;
};

struct by_URI;

typedef boost::multi_index_container<
   seeding_object,
      db::mi::indexed_by<
      db::object_id_index,
         db::mi::ordered_unique<db::mi::tag<by_URI>,
            db::mi::member<seeding_object, std::string, &seeding_object::URI>
         >
      >
>seeding_object_multi_index_type;

typedef graphene::db::generic_index< seeding_object, seeding_object_multi_index_type > seeding_index;

}} // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::seeding_object, (graphene::db::object), (URI)(expiration)(cd)(seeder)(key)(size)(downloaded)(deleted)(_hash) );
