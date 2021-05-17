/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>
#include <download.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/value_cast.hpp>

#include <fstream>
#include <iostream>

namespace smf {

    download::download(cyng::logger logger, bus &cluster_bus, db &cache) : logger_(logger), cluster_bus_(cluster_bus), db_(cache) {}

    std::filesystem::path download::generate(std::string table, std::string format) {

        char tmp_file_name[L_tmpnam];
        ::tmpnam_s(tmp_file_name, L_tmpnam);

        if (boost::algorithm::equals(format, "JSON")) {

            cyng::vector_t vec;
            db_.cache_.access(
                [this, &vec](cyng::table const *tbl) {
                    tbl->loop([this, &vec](cyng::record &&rec, std::size_t) -> bool {
                        vec.push_back(cyng::make_object(rec.to_tuple()));

                        return true;
                    });
                },
                cyng::access::read(table));

            auto const file_name = (std::filesystem::temp_directory_path() / std::string(tmp_file_name)).replace_extension(".json");
            std::ofstream of(file_name.string(), std::ios::binary | std::ios::trunc | std::ios::out);
            if (of.is_open()) {
                CYNG_LOG_INFO(logger_, "create download file [" << file_name << "]");
                auto const obj = cyng::make_object(vec);
                cyng::io::serialize_json_pretty(of, obj);
                of.close();
            }
            return file_name;
        } else if (boost::algorithm::equals(format, "XML")) {

            db_.cache_.access([](cyng::table const *tbl) {}, cyng::access::read(table));

            auto const file_name = (std::filesystem::temp_directory_path() / std::string(tmp_file_name)).replace_extension(".xml");
            std::ofstream of(file_name.string(), std::ios::binary | std::ios::trunc | std::ios::out);
            if (of.is_open()) {
                CYNG_LOG_INFO(logger_, "create download file [" << file_name << "]");
                of.close();
            }
            return file_name;
        } else if (boost::algorithm::equals(format, "CSV")) {

            cyng::vector_t vec;
            db_.cache_.access([&vec](cyng::table const *tbl) { vec = tbl->to_vector(true); }, cyng::access::read(table));

            auto const file_name = (std::filesystem::temp_directory_path() / std::string(tmp_file_name)).replace_extension(".csv");
            std::ofstream of(file_name.string(), std::ios::binary | std::ios::trunc | std::ios::out);
            if (of.is_open()) {
                CYNG_LOG_INFO(logger_, "create download file [" << file_name << "]");
                cyng::io::serialize_csv(of, make_object(vec));
                of.close();
            }
            return file_name;
        }
        return "";
    }

} // namespace smf
