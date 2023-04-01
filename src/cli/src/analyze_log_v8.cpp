#include <analyze_log_v8.h>

#include <cyng/io/io_buffer.h>
#include <cyng/obj/intrinsics.h>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>
#include <cyng/parse/timestamp.h>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <map>

// struct channel {
//     std::uint32_t channel_;
//     std::uint32_t source_;
//     cyng::buffer_t data_;
// };

struct data {
    data() = default;
    data(cyng::date d)
        : date_(d)
        , state_(state::OPEN) {}

    // boost::uuids::uuid tag_;
    cyng::date date_;
    std::map<std::uint64_t, std::vector<cyng::buffer_t>> value_;
    enum class state {
        OPEN,
        PUSH,
        CLOSED,
    } state_;
};

using history = std::map<boost::uuids::uuid, data>;

void read_log_v8(std::filesystem::path const &p, std::size_t counter, history &h) {
    std::ifstream ifs(p.string());
    std::string line;
    while (std::getline(ifs, line)) {
        //
        //  get date
        //
        //  [2018-11-29 14:32:14.12023320] INFO   2788 -- session.99b97eea-d830-437c-b221-004e2218fd35 -
        //  [ipt.req.open.push.channel,[99b97eea-d830-437c-b221-004e2218fd35,83,PUSHBTC,,,,,003c]]
        if (line.size() > 120 && line.at(0) == '[' && line.at(29) == ']' && line.at(93) == '[') {
            auto str_dt = line.substr(1, 19);
            auto const tp = cyng::to_timestamp(str_dt);
            auto t_c = std::chrono::system_clock::to_time_t(tp);
            auto const d = cyng::date::make_date_from_utc_time(t_c);

            auto const pos = line.find_first_of(',', 94);
            if (pos != std::string::npos) {
                // std::cout << str_dt << " - " << line.substr(94, pos - 94) << std::endl;

                auto const msg = line.substr(94, pos - 94);

                //
                //  open push channel
                //
                if (boost::algorithm::equals(msg, "ipt.req.open.push.channel")) {
                    auto const id = cyng::to_uuid(line.substr(121, 36));
                    // std::cout << id << std::endl;
                    auto pd = h.find(id);
                    if (pd != h.end()) {
                        //
                        //  update existing entry
                        //
                        pd->second.state_ = data::state::OPEN;
                    } else {
                        //
                        //  new entry: OPEN
                        //
                        h.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(d));
                    }
                } else if (boost::algorithm::equals(msg, "ipt.req.transfer.pushdata")) {
                    auto const id = cyng::to_uuid(line.substr(121, 36));
                    // std::cout << id << std::endl;
                    auto pd = h.find(id);
                    if (pd != h.end()) {
                        auto const msg = line.substr(121, line.size() - 123);
                        auto const vec = cyng::split(msg, ",");
                        if (vec.size() == 7) {
                            std::uint32_t const channel = std::stoul(vec.at(2), nullptr, 16);
                            std::uint32_t const source = std::stoul(vec.at(3), nullptr, 16);
                            //  static_cast<uint64_t>(channel) << 32 | source;
                            std::uint64_t const key = static_cast<uint64_t>(channel) << 32 | source;
                            if (pd->second.state_ == data::state::OPEN) {
                                //
                                //  read and update channel and source
                                //
                                std::cout << "open channel: " << channel << ", source: " << source << ", key: " << key << std::endl;

                                pd->second.value_.insert(std::make_pair(key, std::vector<cyng::buffer_t>{}));
                                //
                                //  update state
                                //
                                pd->second.state_ = data::state::PUSH;
                            }

                            //
                            //  insert data
                            //
                            auto pv = pd->second.value_.find(key);
                            if (pv != pd->second.value_.end()) {
                                auto const d = cyng::hex_to_buffer(vec.at(6));
                                //  information overload
                                // std::cout << "key: " << key << ", data: " << vec.at(6) << std::endl;
                                pv->second.push_back(d);
                            } else {
                                std::cerr << "internal error: " << line << std::endl;
                            }

                        } else {
                            std::cerr << "format is invalid: " << msg << std::endl;
                        }
                    } else {
                        std::cerr << "session " << id << " has no open channel" << std::endl;
                    }
                } else if (boost::algorithm::equals(msg, "ipt.res.transfer.pushdata")) {
                } else if (boost::algorithm::equals(msg, "ipt.req.close.push.channel")) {
                    auto const id = cyng::to_uuid(line.substr(122, 36));
                    // std::cout << id << std::endl;
                    auto pd = h.find(id);
                    if (pd != h.end()) {
                        if (!pd->second.value_.empty()) {
                            std::cout << "close channel: " << id << " with " << pd->second.value_.size() << " entrie(s)"
                                      << std::endl;
                        } else {
                            std::cerr << "session " << id << " has no entries" << std::endl;
                        }
                        pd->second.state_ = data::state::CLOSED;
                    } else {
                        std::cerr << "session " << id << " has no open channel" << std::endl;
                    }
                } else if (boost::algorithm::equals(msg, "ipt.req.open.connection")) {
                }
            }
        }
    }
}

void analyse_log_v8(std::string s) {
    std::cout << "directory_iterator: " << s << std::endl;
    std::size_t file_counter = 0;
    history h;
    for (auto const &dir_entry : std::filesystem::directory_iterator{s}) {
        // std::cout << "entry: " << dir_entry.path().stem() << std::endl;
        if (boost::algorithm::starts_with(dir_entry.path().filename().string(), "ipt-master")) {
            ++file_counter;
            std::cout << file_counter << ">> " << dir_entry.path().stem() << std::endl;
            read_log_v8(dir_entry.path(), file_counter, h);
        }
    }

    //
    //  dump data
    //
    std::cout << "*** history: " << std::endl;
    std::cout << "login;id;@" << std::endl;
    for (auto const &rec : h) {
        if (rec.second.state_ == data::state::CLOSED) {
            std::cout << rec.first << " @" << cyng::as_string(rec.second.date_, "%Y-%m-%dT%H:%M:%S") << " #"
                      << rec.second.value_.size() << std::endl;
            std::stringstream ss;
            ss << "push-data-" << rec.first << ".log";
            std::ofstream ofs(ss.str());
            if (ofs.is_open()) {
                for (auto const [key, vec] : rec.second.value_) {
                    std::cout << "\t" << key << " #" << vec.size() << std::endl;
                    for (auto const &data : vec) {
                        ofs << cyng::io::to_hex(data);
                    }
                    ofs << std::endl;
                }
                ofs.close();
            } else {
                std::cerr << "cannot open " << ss.str() << std::endl;
            }
        } else {
            std::cout << rec.first << " @" << cyng::as_string(rec.second.date_, "%Y-%m-%dT%H:%M:%S") << " is incomplete"
                      << std::endl;
        }
    }
}
