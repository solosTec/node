#include <smf/cluster/bus.h>

#include <smf.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/net/client_factory.hpp>
#include <cyng/net/net.h>
#include <cyng/obj/algorithm/add.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/sys/process.h>
#include <cyng/vm/generator.hpp>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <boost/bind.hpp>

namespace smf {

    bus::bus(
        cyng::controller &ctl,
        cyng::logger logger,
        toggle::server_vec_t &&tgl,
        std::string const &node_name,
        boost::uuids::uuid tag,
        std::uint32_t features,
        bus_client_interface *bip)
        : ctl_(ctl)
        , logger_(logger)
        , tgl_(std::move(tgl))
        , client_()
        , is_connected_(false)
        , node_name_(node_name)
        , tag_(tag)
        , features_(features)
        , bip_(bip)
        , vm_()
        , parser_([this](cyng::object &&obj) {
            //  un-comment this line to debug problems with transferring data over
            //  the cluster bus.
            // CYNG_LOG_DEBUG(logger_, "[cluster] parser: " << cyng::io::to_typed(obj));
            vm_.load(std::move(obj));
        }) {

        BOOST_ASSERT(bip != nullptr);

        //
        //  create and initialize VM with named functions
        //
        vm_ = init_vm(bip);

        cyng::net::client_factory cf(ctl);
        client_ = cf.create_proxy<boost::asio::ip::tcp::socket, 2048>(
            [this](std::size_t, std::size_t counter, std::string &host, std::string &service)
                -> std::pair<std::chrono::seconds, bool> {
                //
                //  connect failed, reconnect after 20 seconds
                //
                CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect failed for the " << counter << " time");
                if (counter < 3) {
                    //  retry
                    return {std::chrono::seconds(20), true};
                }

                //  try next redundancy in 30 seconds
                tgl_.changeover();
                host = tgl_.get().host_;
                service = tgl_.get().service_;
                return {std::chrono::seconds(0), true};
            },
            [=, this](boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep, cyng::channel_ptr sp) {
                // std::cout << "connected to " << ep << " #" << sp->get_id() << std::endl;
                CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " connected to " << rep);
                //
                //	send login sequence
                //
                auto cfg = tgl_.get();
                client_.send(cyng::serialize_invoke(
                    "cluster.req.login",
                    cfg.account_,
                    cfg.pwd_,
                    cyng::sys::get_process_id(),
                    node_name_,
                    tag_,
                    features_,
                    cyng::version(SMF_VERSION_MAJOR, SMF_VERSION_MINOR)));
            },
            [&](cyng::buffer_t data) {
                //  read from socket
                CYNG_LOG_DEBUG(logger_, "[cluster] " << tgl_.get() << " received " << data.size() << " bytes");

                //
                //	let parse it
                //
                parser_.read(data.begin(), data.end());
            },
            [&](boost::system::error_code ec) {
                //
                //	call disconnect function
                //
                vm_.load(cyng::generate_invoke("cluster.disconnect", ec.message()));
                vm_.run();

                //
                //  reconnect
                //
                tgl_.changeover();
                client_.connect(tgl_.get().host_, tgl_.get().service_);
            },
            [this](cyng::net::client_state state) -> void {
                //
                //  update client state
                //
                is_connected_ = state == cyng::net::client_state::CONNECTED;
                switch (state) {
                case cyng::net::client_state::INITIAL: CYNG_LOG_TRACE(logger_, "[cluster] client state INITIAL"); break;
                case cyng::net::client_state::WAIT: CYNG_LOG_TRACE(logger_, "[cluster] client state WAIT"); break;
                case cyng::net::client_state::CONNECTED: CYNG_LOG_TRACE(logger_, "[cluster] client state CONNECTED"); break;
                case cyng::net::client_state::STOPPED: CYNG_LOG_TRACE(logger_, "[cluster] client state STOPPED"); break;
                default: CYNG_LOG_ERROR(logger_, "[cluster] client state ERROR"); break;
                }
            });
    }

    cyng::vm_proxy bus::init_vm(bus_client_interface *bip) {

        return bip->get_fabric()->make_proxy(

            //	"cluster.res.login"
            cyng::make_description(
                "cluster.res.login", cyng::vm_adaptor<bus_client_interface, void, bool>(bip, &bus_client_interface::on_login)),

            //	"cluster.req.ping"
            cyng::make_description(
                "cluster.req.ping", cyng::vm_adaptor<bus, void, std::chrono::system_clock::time_point>(this, &bus::on_ping)),

            //	"cluster.test.msg"
            cyng::make_description("cluster.test.msg", cyng::vm_adaptor<bus, void, std::string>(this, &bus::on_test_msg)),

            //	"cluster.disconnect"
            cyng::make_description(
                "cluster.disconnect",
                cyng::vm_adaptor<bus_client_interface, void, std::string>(bip, &bus_client_interface::on_disconnect)),

            //	"db.res.insert"
            cyng::make_description(
                "db.res.insert",
                cyng::vm_adaptor<
                    bus_client_interface,
                    void,
                    std::string,
                    cyng::key_t,
                    cyng::data_t,
                    std::uint64_t,
                    boost::uuids::uuid>(bip, &bus_client_interface::db_res_insert)),

            //	"db.res.trx"
            cyng::make_description(
                "db.res.trx",
                cyng::vm_adaptor<bus_client_interface, void, std::string, bool>(bip, &bus_client_interface::db_res_trx)),

            //	"db.res.update"
            cyng::make_description(
                "db.res.update",
                cyng::vm_adaptor<
                    bus_client_interface,
                    void,
                    std::string,
                    cyng::key_t,
                    cyng::attr_t,
                    std::uint64_t,
                    boost::uuids::uuid>(bip, &bus_client_interface::db_res_update)),

            //	"db.res.remove"
            cyng::make_description(
                "db.res.remove",
                cyng::vm_adaptor<bus_client_interface, void, std::string, cyng::key_t, boost::uuids::uuid>(
                    bip, &bus_client_interface::db_res_remove)),

            //	"db.res.clear"
            cyng::make_description(
                "db.res.clear",
                cyng::vm_adaptor<bus_client_interface, void, std::string, boost::uuids::uuid>(
                    bip, &bus_client_interface::db_res_clear)),

            // "cfg.backup.merge"
            cyng::make_description(
                "cfg.backup.merge",
                cyng::vm_adaptor<bus, void, boost::uuids::uuid, cyng::buffer_t, cyng::buffer_t, cyng::obis_path_t, cyng::object>(
                    this, &bus::cfg_backup_merge)),

            //  "cfg.data.sml"
            cyng::make_description(
                "cfg.data.sml",
                cyng::vm_adaptor<bus, void, boost::uuids::uuid, cyng::vector_t, std::string, cyng::obis, cyng::param_map_t>(
                    this, &bus::cfg_data_sml)));
    }

    void bus::start() {

        auto const srv = tgl_.get();
        CYNG_LOG_INFO(logger_, "[cluster] connect(" << srv << ")");

        //
        //	connect to cluster
        //
        client_.connect(tgl_.get().host_, tgl_.get().service_);
    }

    void bus::stop() {
        CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " stop");
        vm_.stop();
        client_.stop();
    }

    boost::uuids::uuid bus::get_tag() const { return tag_; }
    bool bus::is_connected() const { return this->is_connected_; }

    void bus::req_subscribe(std::string table_name) { client_.send(cyng::serialize_invoke("db.req.subscribe", table_name, tag_)); }

    void bus::req_db_insert(std::string const &table_name, cyng::key_t key, cyng::data_t data, std::uint64_t generation) {

        client_.send(cyng::serialize_invoke("db.req.insert", table_name, key, data, generation, tag_));
    }

    void bus::req_db_insert_auto(std::string const &table_name, cyng::data_t data) {

        client_.send(cyng::serialize_invoke("db.req.insert.auto", table_name, data, tag_));
    }

    void bus::req_db_update(std::string const &table_name, cyng::key_t key, cyng::param_map_t data) {

        //
        //	triggers a merge() on the receiver side
        //
        client_.send(cyng::serialize_invoke("db.req.update", table_name, key, data, tag_));
    }

    void bus::req_db_remove(std::string const &table_name, cyng::key_t key) {

        client_.send(cyng::serialize_invoke("db.req.remove", table_name, key, tag_));
    }

    void bus::req_db_clear(std::string const &table_name) {
        client_.send(cyng::serialize_invoke("db.req.clear", table_name, tag_));
    }

    void bus::pty_login(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        std::string data_layer,
        boost::asio::ip::tcp::endpoint ep) {

        auto const srv = tgl_.get();
        client_.send(cyng::serialize_invoke("pty.req.login", tag, name, pwd, ep, data_layer));
    }

    void bus::pty_logout(boost::uuids::uuid dev, boost::uuids::uuid tag) {

        client_.send(cyng::serialize_invoke("pty.req.logout", tag, dev));
    }

    void bus::pty_open_connection(std::string msisdn, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.open.connection", tag, dev, msisdn, token));
    }

    void bus::pty_res_open_connection(
        bool success,
        boost::uuids::uuid dev,    //	callee dev-tag
        boost::uuids::uuid callee, //	callee vm-tag
        cyng::param_map_t &&token) {

        auto const reader = cyng::make_reader(token);
        auto const peer = cyng::value_cast(reader["caller-peer"].get(), boost::uuids::nil_uuid());
        BOOST_ASSERT(!peer.is_nil());

        client_.send(cyng::serialize_forward(
            "pty.return.open.connection",
            peer, //	caller_vm
            success,
            dev,
            callee,
            token));
    }
    void bus::pty_close_connection(boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.close.connection", tag, dev, token));
    }

    void bus::pty_transfer_data(boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::buffer_t &&data) {

        client_.send(cyng::serialize_invoke("pty.transfer.data", tag, dev, std::move(data)));
    }

    void bus::pty_reg_target(
        std::string name,
        std::uint16_t paket_size,
        std::uint8_t window_size,
        boost::uuids::uuid dev,
        boost::uuids::uuid tag,
        cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.register.target", tag, dev, name, paket_size, window_size, token));
    }

    void bus::pty_dereg_target(std::string name, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.deregister", tag, dev, name, token));
    }

    //	"pty.open.channel"
    void bus::pty_open_channel(
        std::string name,
        std::string account,
        std::string msisdn,
        std::string version,
        std::string id,
        std::chrono::seconds timeout,
        boost::uuids::uuid dev,
        boost::uuids::uuid tag,
        cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.open.channel", tag, dev, name, account, msisdn, version, id, timeout, token));
    }

    //	"pty.close.channel"
    void bus::pty_close_channel(std::uint32_t channel, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.close.channel", tag, dev, channel, token));
    }

    //	"pty.req.push.data"
    void bus::pty_push_data(
        std::uint32_t channel,
        std::uint32_t source,
        cyng::buffer_t data,
        boost::uuids::uuid dev,
        boost::uuids::uuid tag,
        cyng::param_map_t &&token) {

        client_.send(cyng::serialize_invoke("pty.req.push.data", tag, dev, channel, source, data, token));
    }

    void bus::pty_stop(std::string const &table_name, cyng::key_t key) {

        client_.send(cyng::serialize_invoke("pty.stop", table_name, key));
    }

    void bus::push_sys_msg(std::string msg, cyng::severity level) { client_.send(cyng::serialize_invoke("sys.msg", msg, level)); }

    void bus::update_pty_counter(std::uint64_t count) {

        req_db_update("cluster", cyng::key_generator(tag_), cyng::param_map_factory("clients", count));
    }

    void bus::cfg_backup_init(std::string const &table_name, cyng::key_t key, std::chrono::system_clock::time_point tp) {

        client_.send(cyng::serialize_invoke("cfg.backup.init", table_name, key, tp));
    }

    void bus::cfg_sml_channel_out(
        cyng::vector_t key,        //  gateway
        std::string channel,       //  SML message type
        cyng::obis section,        //  OBIS root
        cyng::param_map_t params,  //  optional parameters (OBIS path)
        boost::uuids::uuid source) //  HTTP session
    {
        //  address.out = key
        //  address.back = source, tag_
        //  message = channel, section, params
        client_.send(cyng::serialize_invoke("cfg.sml.channel", true, key, channel, section, params, source, tag_));
    }

    void bus::cfg_sml_channel_back(
        cyng::vector_t key,        //  gateway
        std::string channel,       //  SML message type
        cyng::obis section,        //  OBIS root
        cyng::param_map_t params,  //  optional parameters (OBIS path)
        boost::uuids::uuid source, //  HTTP session
        boost::uuids::uuid tag)    //  cluster node tag (mostly dash)
    {
        //  address.out = source, tag
        //  message = key, channel, section, params
        client_.send(cyng::serialize_invoke("cfg.sml.channel", false, key, channel, section, params, source, tag));
    }

    bool bus::has_cfg_sink_interface() const { return bip_->get_cfg_sink_interface() != nullptr; }
    bool bus::has_cfg_data_interface() const { return bip_->get_cfg_data_interface() != nullptr; }

    void bus::cfg_backup_merge(
        boost::uuids::uuid tag,
        cyng::buffer_t gw,
        cyng::buffer_t meter,
        cyng::obis_path_t path,
        cyng::object value) {

        if (has_cfg_sink_interface()) {
            bip_->get_cfg_sink_interface()->cfg_merge(tag, gw, meter, path, value);
        } else {
            CYNG_LOG_TRACE(logger_, "[cluster#cfg.backup.merge]: " << path << ": " << value);
            client_.send(cyng::serialize_invoke("cfg.backup.merge", tag, gw, meter, path, value));
        }
    }

    void bus::cfg_data_sml(
        boost::uuids::uuid tag,  // HTTP session
        cyng::vector_t key,      // table key (gateway)
        std::string channel,     // SML message type
        cyng::obis section,      // OBIS root
        cyng::param_map_t params // results
    ) {
        if (has_cfg_data_interface()) {
            CYNG_LOG_TRACE(logger_, "[cluster#cfg.data.sml]: " << channel << ", section: " << section);
            bip_->get_cfg_data_interface()->cfg_data(tag, key, channel, section, params);
        } else {
            CYNG_LOG_WARNING(logger_, "[cluster#cfg.data.sml]: not implemented");
        }
    }

    void bus::on_ping(std::chrono::system_clock::time_point tp) {
        CYNG_LOG_TRACE(logger_, "ping: " << tp);
        //
        //  echo
        //
        client_.send(cyng::serialize_invoke("cluster.res.ping", tp));
    }

    void bus::on_test_msg(std::string msg) { CYNG_LOG_INFO(logger_, "[cluster] test msg: \"" << msg << "\""); }

} // namespace smf
