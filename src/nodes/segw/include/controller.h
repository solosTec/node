/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_SEGW_CONTROLLER_H
#define SMF_SEGW_CONTROLLER_H

#include <smf/controller_base.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

namespace cyng {
    class controller;
}

namespace smf {

    class controller : public config::controller_base {
      public:
        controller(config::startup const &);
        virtual bool run_options(boost::program_options::variables_map &) override;

      protected:
        cyng::vector_t create_default_config(
            std::chrono::system_clock::time_point &&now,
            std::filesystem::path &&tmp,
            std::filesystem::path &&cwd) override;

        virtual void
        run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) override;

        virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) override;

      private:
        std::tuple<cyng::mac48, std::string> get_server_id() const;

        void init_storage(cyng::object &&);
        void read_config(cyng::object &&);
        void write_config(cyng::object &&, std::string);
        void clear_config(cyng::object &&);
        void list_config(cyng::object &&);
        void set_config_value(cyng::object &&, std::string const &path, std::string const &value, std::string const &type);
        void add_config_value(cyng::object &&, std::string const &path, std::string const &value, std::string const &type);
        void del_config_value(cyng::object &&, std::string const &path);
        void set_nms_mode(cyng::object &&cfg, std::string mode);
        void switch_gpio(cyng::object &&cfg, std::string const &number, std::string const &state);
        void alter_table(cyng::object &&cfg, std::string table);

      private:
        cyng::channel_ptr bridge_;
    };

    /**
     * When running on the segw hardware tries to figure out if this is a BPL device
     * and returns the link local address of "
     */
    // std::string get_nms_address(std::string nic);

    void print_nms_defaults(std::ostream &);
    void print_tty_options(std::ostream &, std::string tty);

} // namespace smf

#endif
