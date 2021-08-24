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

#include <boost/program_options.hpp>

#include <graphene/net/node.hpp>

namespace boost {
  namespace filesystem {
    class path;
  }
}

namespace graphene {
   namespace chain { class database; }
   namespace app { namespace detail { class application_impl; }

   class abstract_plugin;
   class api_access_info;

   class application
   {
      public:
         application();
         ~application();

         static void set_program_options(boost::program_options::options_description& command_line_options,
                                         boost::program_options::options_description& configuration_file_options);

         void initialize(const boost::filesystem::path& data_dir, const boost::program_options::variables_map&options);
         void initialize_plugins( const boost::program_options::variables_map& options );
         void startup();
         void shutdown();
         void startup_plugins();
         void shutdown_plugins();

         template<typename PluginType>
         std::shared_ptr<PluginType> create_plugin()
         {
            auto plug = std::make_shared<PluginType>(this);
            add_plugin( PluginType::plugin_name(), plug );
            return plug;
         }

         std::shared_ptr<abstract_plugin> get_plugin( const std::string& name ) const;

         template<typename PluginType>
         std::shared_ptr<PluginType> get_plugin( const std::string& name ) const
         {
            std::shared_ptr<abstract_plugin> abs_plugin = get_plugin( name );
            std::shared_ptr<PluginType> result = std::dynamic_pointer_cast<PluginType>( abs_plugin );
            FC_ASSERT( result != std::shared_ptr<PluginType>() );
            return result;
         }

         net::node_ptr                    p2p_node();
         std::shared_ptr<chain::database> chain_database()const;

         void set_block_production(bool producing_blocks);
         fc::optional< api_access_info > get_api_access_info( const std::string& username )const;
         void set_api_access_info(const std::string& username, api_access_info&& permissions);

         uint64_t get_processed_transactions();

      private:
         void add_plugin( const std::string& name, std::shared_ptr<abstract_plugin> p );
         std::shared_ptr<detail::application_impl> my;
   };

} } // graphene::app
