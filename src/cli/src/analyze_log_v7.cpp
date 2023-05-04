#include <analyze_log_v7.h>

#include <cyng/obj/intrinsics.h>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/parse/string.h>
#include <cyng/parse/timestamp.h>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <map>

struct data_imega {
    cyng::date date_;
    std::string id_; // meter ID
    std::string ep_; // tcp/ip endpoint
};

/**
 * imega history
 */
using history_imega = std::map<boost::uuids::uuid, data_imega>;

struct data_ipt {
    cyng::date accept_;
    cyng::date close_;
    std::string ec_;
    std::string id_;      // meter ID
    std::string ep_;      // tcp/ip endpoint
    std::uint32_t bytes_; // total bytes transferred
    std::vector<std::string> connections_;
};

/**
 * ip-t history
 */
using history_ipt = std::map<boost::uuids::uuid, data_ipt>;

void read_log_imega_v7(std::filesystem::path const &p, std::size_t counter, history_imega &h) {
    std::ifstream ifs(p.string());
    std::string line;
    while (std::getline(ifs, line)) {
        //
        //  get date
        //
        if (line.size() > 47 && line.at(0) == '[' && line.at(29) == ']') {
            auto str_dt = line.substr(1, 19);
            auto const tp = cyng::to_timestamp(str_dt);
            auto t_c = std::chrono::system_clock::to_time_t(tp);
            auto const d = cyng::date::make_date_from_utc_time(t_c);

            // std::cout << str_dt << " => " << std::put_time(std::localtime(&t_c), "%F %T") << " - " << line.substr(46, 6)  <<
            // std::endl;
            auto const msg = line.substr(46);
            if (boost::algorithm::equals(msg.substr(0, 6), "accept")) {
                auto const n = msg.find(" - ", 7);
                auto const id = cyng::to_uuid(msg.substr(n + 3));
                auto const ep = msg.substr(7, n - 7);

                std::cout << "ACCEPT: " << ep << " - " << id << std::endl;
                h.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(d, "", ep));

            } else if (boost::algorithm::equals(msg.substr(0, 22), "imega.req.login.public")) {
                std::cout << "LOGIN: " << msg << std::endl;
            } else if (boost::algorithm::equals(msg.substr(0, 22), "use TELNB as password:")) {
                if (msg.size() == 85) {
                    auto const id = cyng::to_uuid(msg.substr(24, 36));
                    std::cout << "TELNB: " << id << " - " << msg.substr(67, 8) << ", " << msg.substr(76, 8) << std::endl;
                    auto pos = h.find(id);
                    if (pos != h.end()) {
                        pos->second.id_ = msg.substr(67, 8);
                    }
                }
            } else if (boost::algorithm::equals(msg.substr(0, 10), "pool size:")) {
                std::cout << "RESTART: " << msg << std::endl;
            }
        }
    }
}

void read_log_ipt_v7(std::filesystem::path const &p, std::size_t counter, history_ipt &h) {
    std::ifstream ifs(p.string());
    std::string line;
    while (std::getline(ifs, line)) {
        //
        //  get date
        //
        if (line.size() > 47 && line.at(0) == '[' && line.at(29) == ']') {
            auto str_dt = line.substr(1, 19);
            auto const tp = cyng::to_timestamp(str_dt);
            auto t_c = std::chrono::system_clock::to_time_t(tp);
            auto const d = cyng::date::make_date_from_utc_time(t_c);

            auto const msg = line.substr(46);
            if (boost::algorithm::equals(msg.substr(0, 6), "accept")) {
                auto const n = msg.find(" - ", 7);
                auto const id = cyng::to_uuid(msg.substr(n + 3));
                auto const ep = msg.substr(7, n - 7);

#ifdef CLI_OUTPUT_CONSOLE
                std::cout << "ACCEPT: " << ep << " - " << id << std::endl;
#endif
                h.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(d, d, "", "", ep, 0, std::vector<std::string>{}));
            } else if (boost::algorithm::equals(msg.substr(0, 20), "ipt.req.login.public")) {
                // ipt.req.login.public 2077d582-c8b0-4d0f-b936-89008a08f1b5:SCSSGSWPull8:SCSSGSWPull2014
                auto const id = cyng::to_uuid(msg.substr(21, 36));
                auto const vec = cyng::split(msg.substr(21), ":");
                BOOST_ASSERT(vec.size() == 3);
                if (vec.size() == 3) {
#ifdef CLI_OUTPUT_CONSOLE
                    std::cout << "LOGIN : " << id << " (" << vec.at(1) << '/' << vec.at(2) << ")" << std::endl;
#endif
                    auto pos = h.find(id);
                    if (pos != h.end()) {
                        pos->second.id_ = vec.at(1);
                    }
                }
            } else if (msg.size() > 114 && boost::algorithm::equals(msg.substr(48, 23), "ipt.req.open.connection")) {
                // session.2077d582-c8b0-4d0f-b936-89008a08f1b5 -
                // [ipt.req.open.connection,[2077d582-c8b0-4d0f-b936-89008a08f1b5,1,95313439]]
                auto const id = cyng::to_uuid(msg.substr(73, 36));
                auto const remote = msg.substr(112, msg.size() - 114);
#ifdef CLI_OUTPUT_CONSOLE
                std::cout << "OPEN  : " << id << " -> " << remote << std::endl;
#endif
                auto pos = h.find(id);
                if (pos != h.end()) {
                    pos->second.connections_.push_back(remote);
                }
            } else if (msg.size() == 53 && boost::algorithm::equals(msg.substr(47, 6), "[HALT]")) {
                // session.35e9ce4a-33f3-433d-9481-854d5d3c61f0 - [HALT]
                auto const id = cyng::to_uuid(msg.substr(8, 36));
#ifdef CLI_OUTPUT_CONSOLE
                std::cout << "HALT  : " << id << std::endl;
#endif
            } else if (msg.size() > 55 && boost::algorithm::equals(msg.substr(45, 6), "closed")) {
                // session ce8cad7f-b8cc-4cd3-abd2-df7a8679acac closed: asio.misc:2:2 [End of file]
                auto const id = cyng::to_uuid(msg.substr(8, 36));
                auto pos = h.find(id);
                if (pos != h.end()) {
                    pos->second.close_ = d;
                    if (msg.at(51) == ':') {
#ifdef CLI_OUTPUT_CONSOLE
                        std::cout << "CLOSED: " << id << " - " << msg.substr(53) << std::endl;
#endif
                        pos->second.ec_ = msg.substr(53);
                    } else {
#ifdef CLI_OUTPUT_CONSOLE
                        std::cout << "CLOSED: " << id << " - " << msg.substr(67) << std::endl;
#endif
                        pos->second.ec_ = msg.substr(67);
                    }
                }
            } else if (msg.size() > 54 && boost::algorithm::equals(msg.substr(0, 2), ">>")) {
                //>> 4bea27d3-221a-41b8-9c1c-91199b6f05df enter state CONNECTED_LOCAL
                auto const id = cyng::to_uuid(msg.substr(3, 36));
#ifdef CLI_OUTPUT_CONSOLE
                std::cout << "STATE : " << id << " - " << msg.substr(52) << std::endl;
#endif
            } else if (msg.size() == 161 && boost::algorithm::equals(msg.substr(120, 4), "<==>")) {
                // server_stub 6a0f6fc3-f225-46d5-8efa-f0a73af72b75 establish a local connection [0]:
                // ccd53ca6-8ec2-426b-837e-3324961828fc <==> 4bea27d3-221a-41b8-9c1c-91199b6f05df
                auto const id1 = cyng::to_uuid(msg.substr(83, 36));
                auto const id2 = cyng::to_uuid(msg.substr(125, 36));
#ifdef CLI_OUTPUT_CONSOLE
                std::cout << "LOCAL : " << id1 << " <=> " << id2 << std::endl;
#endif
            } else if (msg.size() == 107 && boost::algorithm::equals(msg.substr(0, 20), "transmit data failed")) {
                // transmit data failed: 5c4e5043-d93e-44ac-9942-27f9295ee199 is not member of connection map with 0 entrie(s)
                auto const id = cyng::to_uuid(msg.substr(22, 36));
                // std::cout << "FAIL  : " << id << " - " << msg << std::endl;
            } else if (msg.size() > 79 && boost::algorithm::equals(msg.substr(48, 23), "ipt connection received")) {
                // session.ecd4fd16-efe3-4c18-9063-ab781a4ef1b9 - [ipt connection received,9,bytes]
                auto const id = cyng::to_uuid(msg.substr(8, 36));
                auto const vec = cyng::split(msg.substr(48, msg.size() - 49), ",");
                if (vec.size() == 3) {
#ifdef CLI_OUTPUT_CONSOLE
                    std::cout << "REC   : " << id << " - " << vec.at(1) << " bytes" << std::endl;
#endif
                }
            } else if (msg.size() > 150 && boost::algorithm::equals(msg.substr(48, 32), "client.req.transmit.data.forward")) {
                //
                // session.4f9a1277-4134-4471-9511-714fe1edf3e6 -
                // [client.req.transmit.data.forward,[936c7143-9dcf-4286-b3a4-3efa16c23231,22918,%(("tp-layer":imega)),283030392E3029283030302E3029283030302E3129283030302E3029283030302E3029283030372E30290D0A283030342E3429283030302E3029283030302E3029283030302E3029283030302E3029283031302E36290D0A0476]]
                auto const id = cyng::to_uuid(msg.substr(8, 36));
                auto const vec = cyng::split(msg.substr(82, msg.size() - 84), ",");
                if (vec.size() == 4) {
                    auto const bytes = (vec.at(3).size() / 2);
#ifdef CLI_OUTPUT_CONSOLE
                    std::cout << "TRANS : " << id << " - " << bytes << " bytes" << std::endl;
#endif
                    auto pos = h.find(id);
                    if (pos != h.end()) {
                        pos->second.bytes_ += bytes;
                    }
                }
            }
        }
    }
}

void write(history_ipt const &h, std::ostream &os) {
    os << "login;duration;id;ep;ec;connections;bytes;remote" << std::endl;
    for (auto const &rec : h) {
        os << cyng::as_string(rec.second.accept_, "%Y-%m-%dT%H:%M:%S")
           << ";"
           //                  << cyng::as_string(rec.second.close_, "%Y-%m-%dT%H:%M:%S") << ";"
           << rec.second.close_.sub<std::chrono::seconds>(rec.second.accept_) << ";" << rec.second.id_ << ";" << rec.second.ep_
           << ";" << rec.second.ec_ << ";" << rec.second.connections_.size() << ";" << rec.second.bytes_;
        for (auto const &remote : rec.second.connections_) {
            os << ";" << remote;
        }
        os << std::endl;
    }
}

void analyse_log_v7(std::string s) {
    std::cout << "directory_iterator: " << s << std::endl;
    std::size_t file_counter = 0;
    history_imega imega;
    history_ipt ipt;

#ifdef __DEBUG
    read_log_ipt_v7("F:/backup/customer/sgsw/logs/2023-04-05/ipt-master_backup_2021T322_2471.log", file_counter, ipt);
#else
    for (auto const &dir_entry : std::filesystem::directory_iterator{s}) {
        // std::cout << "entry: " << dir_entry.path().stem() << std::endl;
        if (boost::algorithm::starts_with(dir_entry.path().filename().string(), "smf-e350_backup_2023")) {
            ++file_counter;
            std::cout << std::setw(2) << file_counter << ">> " << dir_entry.path().stem() << std::endl;
            read_log_imega_v7(dir_entry.path(), file_counter, imega);
            //} else if (boost::algorithm::starts_with(dir_entry.path().filename().string(), "ipt-master_backup_2023")) {
        } else if (boost::algorithm::starts_with(dir_entry.path().filename().string(), "ipt-master")) {
            ++file_counter;
            std::cout << std::setw(2) << file_counter << ">> " << dir_entry.path().stem() << std::endl;
            read_log_ipt_v7(dir_entry.path(), file_counter, ipt);
        }
    }
#endif

    //
    //  dump data
    //
    std::cout << "*** history imega: " << std::endl;
    std::cout << "login;id;ep" << std::endl;
    for (auto const &rec : imega) {
        std::cout << cyng::as_string(rec.second.date_, "%Y-%m-%dT%H:%M:%S") << ";" << rec.second.id_ << ";" << rec.second.ep_
                  << std::endl;
    }
    std::cout << "*** history ipt: " << std::endl;
    // 2021-11-15T03:15:54;23s;SCSSGSWPull10;100.77.255.10:52281;asio.misc:2:2 [End of file];1;95253671
#ifdef CLI_OUTPUT_CONSOLE
    write(ipt, std::cout);
#else
    std::ofstream ifs("ipt-statistics.csv", std::ios::trunc);
    write(ipt, ifs);
#endif
}
