/* (c) 2016, 2021 FFF Services. For details refers to LICENSE.txt */

#pragma once

#include <mutex>
#include <fc/thread/thread.hpp>
#include <graphene/app/api.hpp>

namespace boost { namespace filesystem { class path; } }

namespace graphene { namespace wallet {
   struct server_data;
   class wallet_api;

   typedef typename fc::api<graphene::app::database_api>::vtable_type db_api;
   typedef typename fc::api<graphene::app::network_broadcast_api>::vtable_type net_api;

   class WalletAPI
   {
   public:
      WalletAPI();
      ~WalletAPI();

      void Connect(const boost::filesystem::path &wallet_file, const graphene::wallet::server_data &ws);
      std::string RunTask(std::string const& str_command);

      bool is_connected() { std::lock_guard<std::mutex> lock(m_mutex); return m_pimpl != nullptr; }

      template<typename Result, typename ...Args, typename ...Values>
      auto query(std::function<Result(Args...)> db_api::* func, Values... values)
      {
         std::lock_guard<std::mutex> lock(m_mutex);
         return m_pthread->async([=, api = get_db_api()]() -> Result { return ((*api).*func)(values...); });
      }

      template<typename Result, typename ...Args, typename ...Values>
      auto broadcast(std::function<Result(Args...)> net_api::* func, Values... values)
      {
         std::lock_guard<std::mutex> lock(m_mutex);
         return m_pthread->async([=, api = get_net_api()]() -> Result { return ((*api).*func)(values...); });
      }

      template<typename Result, typename ...Args, typename ...Values>
      auto exec(Result (wallet_api::* func)(Args...), Values... values)
      {
         std::lock_guard<std::mutex> lock(m_mutex);
         return m_pthread->async([=, api = get_api()]() -> Result { return (api.get()->*func)(values...); });
      }

      template<typename Result, typename ...Args, typename ...Values>
      auto exec(Result (wallet_api::* func)(Args...) const, Values... values)
      {
         std::lock_guard<std::mutex> lock(m_mutex);
         return m_pthread->async([=, api = get_api()]() -> Result { return (api.get()->*func)(values...); });
      }

   private:
      fc::api<app::database_api> get_db_api() const;
      fc::api<app::network_broadcast_api> get_net_api() const;
      std::shared_ptr<wallet_api> get_api() const;

      // wallet_api does not like to be accessed from several threads
      // so all the access is encapsulated inside m_pthread :(
      std::mutex m_mutex;
      std::unique_ptr<fc::thread> m_pthread;

      struct Impl;
      std::unique_ptr<Impl> m_pimpl;
   };

} }
