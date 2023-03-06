#include <analyze_log_v7.h>

#include <cyng/obj/intrinsics.h>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/parse/string.h>
#include <cyng/parse/timestamp.h>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <map>

struct data {
    cyng::date date_;
    std::string id_; // meter ID
    std::string ep_; // tcp/ip endpoint
};

using history = std::map<boost::uuids::uuid, data>;

void read_log_v7(std::filesystem::path const &p, std::size_t counter, history &h) {
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

void analyse_log_v7(std::string s) {
    std::cout << "directory_iterator: " << s << std::endl;
    std::size_t file_counter = 0;
    history h;
    for (auto const &dir_entry : std::filesystem::directory_iterator{s}) {
        // std::cout << "entry: " << dir_entry.path().stem() << std::endl;
        if (boost::algorithm::starts_with(dir_entry.path().filename().string(), "smf-e350_backup_2023")) {
            ++file_counter;
            std::cout << file_counter << ">> " << dir_entry.path().stem() << std::endl;
            read_log_v7(dir_entry.path(), file_counter, h);
        }
    }

    //
    //  dump data
    //
    std::cout << "*** history: " << std::endl;
    std::cout << "login;id;ep" << std::endl;
    for (auto const &rec : h) {
        std::cout << cyng::as_string(rec.second.date_, "%Y-%m-%dT%H:%M:%S") << ";" << rec.second.id_ << ";" << rec.second.ep_
                  << std::endl;
    }
}
