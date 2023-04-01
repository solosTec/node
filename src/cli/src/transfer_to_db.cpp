#include <transfer_to_db.h>

#include <smf/config/schemes.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/sml/reader.h>
#include <smf/sml/tokenizer.h>
#include <smf/sml/unpack.h>

#include <cyng/db/session.h>
#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>
#include <cyng/sql/sql.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/assert.hpp>

namespace smf {

    cyng::prop_map_t convert_to_param_map(sml::sml_list_t const &inp) {
        cyng::prop_map_t r;
        std::transform(
            std::begin(inp),
            std::end(inp),
            std::inserter(r, r.end()),
            [](sml::sml_list_t::value_type value) -> cyng::prop_map_t::value_type {
                //  Add the original key as value. This saves a reconversion from string type
                value.second.emplace("key", cyng::make_object(value.first));
                value.second.emplace("code", cyng::make_object(obis::get_name(value.first)));
                value.second.emplace("descr", cyng::make_object(obis::get_description(value.first)));

                return {value.first, cyng::make_object(value.second)};
            });
        return r;
    }

    //
    //  from sml_db_writer.cpp
    //
    void store(cyng::db::session &db, boost::uuids::uuid tag, cyng::prop_map_t const &data) {

        auto const m = config::get_table_sml_readout_data();
        auto const sql = cyng::sql::insert(db.get_dialect(), m).bind_values(m)();

        auto stmt = db.create_statement();
        auto const r = stmt->prepare(sql);
        if (r.second) {

            for (auto const &value : data) {
                auto const reader = cyng::make_reader(value.second);
                auto const scaler = reader.get<std::int8_t>("scaler", 0); //  signed!
                auto const unit = reader.get<std::uint8_t>("unit", 0u);
                auto const reading = cyng::io::to_plain(reader.get("value")); //  string
                auto const obj = reader.get("raw");
                auto const type = obj.tag(); //  u16
                auto const descr = reader.get("descr", "");

                std::cout << "[sml.db] store " << value.first << ": " << value.second << " ("
                          << cyng::intrinsic_name_by_type_code(static_cast<cyng::type_code>(type)) << ")" << std::endl;
                std::cout << "[sml.db] store scaler : " << +scaler << std::endl;
                std::cout << "[sml.db] store unit   : " << +unit << std::endl;
                std::cout << "[sml.db] store reading: " << reading << " (" << cyng::io::to_typed(reader.get("value")) << ")"
                          << std::endl;
                std::cout << "[sml.db] store type   : " << type << " ("
                          << cyng::intrinsic_name_by_type_code(static_cast<cyng::type_code>(type)) << ")" << std::endl;

                stmt->push(cyng::make_object(tag), 0); //	tag

                auto const reg = value.first;
                stmt->push(cyng::make_object(reg), 0); //	register
                std::cout << "[sml.db] store reg    : " << reg << std::endl;
                stmt->push(cyng::make_object(0), 0); //	gen

                //  Scaled values must be stored as integers without scaling
                if (type == cyng::TC_BUFFER) {
                    //  Buffer values should contain val.size() % 2 == 0 elements
                    if (reading.size() % 2 == 0) {
                        stmt->push(cyng::make_object(reading), 256); //	reading
                    } else {
                        stmt->push(cyng::make_object(""), 256); //	reading
                    }
                } else {
                    stmt->push(cyng::make_object(reading), 256); //	reading
                }
                stmt->push(cyng::make_object(type), 0);   //	type
                stmt->push(cyng::make_object(scaler), 0); //	scaler
                stmt->push(cyng::make_object(unit), 0);   //	unit
                if (stmt->execute()) {
                    stmt->clear();
                    std::cout << "[sml.db] data record " << tag << " successful stored in table " << m.get_name() << std::endl;
                } else {
                    std::cerr << "[sml.db] insert into " << m.get_name() << " failed" << std::endl;
                }
            }
        } else {
            std::cerr << "[sml.db] internal error: " << sql << std::endl;
        }
    }

    //
    //  from sml_db_writer.cpp
    //
    void store(
        cyng::db::session &db,
        boost::uuids::random_generator_mt19937 &uidgen,
        std::string trx,
        cyng::buffer_t srv,
        cyng::obis profile,
        cyng::object act_time,
        std::uint32_t status,
        cyng::prop_map_t const &values) {

        auto const now = cyng::make_utc_date();

        //
        //  meter id
        //  srv_id_to_str() works for 8 and 9 byte formats
        //  size == 9 => 01-a815-54787404-01-02
        //  size == 8 => 3034393137373235 => 04917725
        //
        auto const id = srv_id_to_str(srv, false);

        //
        //  primary key
        //
        auto const tag = uidgen();

        //
        //  write to database
        //
        try {

            //
            //	start transaction
            //
            cyng::db::transaction transaction(db);

            //
            //	prepare statement
            //
            auto const m = config::get_table_sml_readout();
            auto const sql = cyng::sql::insert(db.get_dialect(), m).bind_values(m)();

            auto stmt = db.create_statement();
            auto const r = stmt->prepare(sql);
            if (r.second) {

                stmt->push(cyng::make_object(tag), 0); //	tag

                stmt->push(cyng::make_object(0), 0); //	gen

                stmt->push(cyng::make_object(srv), 9);     //	meterID
                stmt->push(cyng::make_object(profile), 0); //	profile
                stmt->push(cyng::make_object(trx), 21);    //	trx
                stmt->push(cyng::make_object(status), 0);  //	status
                //
                //  overwrite actTime (01 00 00 09 0b 00)
                //  OBIS_CURRENT_UTC
                //  The actTime is stored as TC_TIME but will be read as TC_DATE
                auto const pos = values.find(OBIS_CURRENT_UTC);
                if (pos != values.end()) {
                    auto const c = cyng::container_cast<cyng::param_map_t>(pos->second);
                    auto const idx = c.find("value");
                    if (idx != c.end()) {
                        auto const prev = act_time.clone();
                        act_time = idx->second.clone();
                        stmt->push(idx->second, 0); // actTime
                        stmt->push(prev, 0);        // original actTime

                        auto const unboxed = cyng::value_cast(act_time, now);
                        std::cout << "[sml.db] update actTime of data record " << tag << " from " << prev << " to " << act_time
                                  << std::endl;
                    } else {
                        std::cerr << "[sml.db] missing column \"value\" in data record " << tag << std::endl;
                    }
                } else {
                    stmt->push(act_time, 0);               //	received
                    stmt->push(cyng::make_object(now), 0); //	actTime
                }

                if (stmt->execute()) {
                    stmt->clear();
                    std::cout << "[sml.db] data record of " << id << " successful stored in table " << m.get_name()
                              << " by key: " << tag << std::endl;
                    ;
                } else {
                    std::cerr << "[sml.db] insert into " << m.get_name() << " failed" << std::endl;
                }

                //
                //  write data
                //
                store(db, tag, values);

            } else {
                std::cerr << "[sml.db] internal error: " << sql << std::endl;
            }

        } catch (std::exception const &ex) {
            std::cerr << "[sml.db]: " << ex.what() << std::endl;
        }
    }
} // namespace smf

void transfer_to_db(cyng::db::session &db, std::filesystem::path const &p, std::size_t counter) {
    std::ifstream ifs(p.string());
    BOOST_ASSERT(ifs.is_open());
    if (ifs.is_open()) {
        boost::uuids::random_generator_mt19937 uidgen;

        std::string line;
        while (std::getline(ifs, line)) {
            //
            //  convert to binary data
            //
            auto const data = cyng::hex_to_buffer(line);
            BOOST_ASSERT(!data.empty());
            BOOST_ASSERT(data.size() == (line.size() / 2));

            smf::sml::unpack p(
                [&](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {
                    // std::cout << "> " << smf::sml::get_name(type) << ": " << trx << std::endl;
                    //  std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;
                    if (type == smf::sml::msg_type::GET_PROFILE_LIST_RESPONSE) {
                        //  server id, tree path, act time, register period, val time, M-Bus state, period list
                        // cyng::buffer_t, cyng::obis_path_t, cyng::object, std::chrono::seconds, cyng::object, std::uint32_t,
                        // sml_list_t
                        auto const [srv, p, act, reg, val, stat, list] = smf::sml::read_get_profile_list_response(msg);
                        //  "TSMLReadout"
                        std::cout << "> " << smf::srv_id_to_str(srv, true) << ", @" << act << std::endl;
                        //  "TSMLReadoutData"
                        for (auto const &ro : list) {
                            std::cout << ">> " << ro.first << ": " << ro.second << std::endl;
                        }
                        auto const pmap = smf::convert_to_param_map(list);
                        BOOST_ASSERT(!pmap.empty());

                        //
                        //  insert into database
                        //
                        smf::store(db, uidgen, "trx", srv, p.front(), act, stat, pmap);
                    }
                });
            p.read(std::begin(data), std::end(data));
        }
    }
}

void transfer_to_db(std::string s, std::string p) {
    cyng::db::session db(cyng::db::connection_type::SQLITE);
    db.connect(cyng::make_tuple(
        cyng::make_param("type", "SQLite"), cyng::make_param("file.name", p), cyng::make_param("busy.timeout", 3800)));
    BOOST_ASSERT(db.is_alive());
    if (db.is_alive()) {
        std::cout << "directory_iterator: " << s << std::endl;
        std::size_t file_counter = 0;
        for (auto const &dir_entry : std::filesystem::directory_iterator{s}) {
            // std::cout << "entry: " << dir_entry.path().stem() << std::endl;
            if (boost::algorithm::starts_with(dir_entry.path().filename().string(), "push-data-")) {
                ++file_counter;
                std::cout << file_counter << ">> " << dir_entry.path().stem() << std::endl;
                transfer_to_db(db, dir_entry.path(), file_counter);
            }
        }
    } else {
        std::cerr << "cannot open database" << std::endl;
    }
}
