#include <smf/config/persistence.h>

#include <cyng/sql/sql.hpp>

#include <boost/assert.hpp>

namespace smf {
    namespace config {
        bool persistent_insert(
            cyng::meta_sql const &meta,
            cyng::db::session db,
            cyng::key_t key,
            cyng::data_t const &data,
            std::uint64_t gen) {
            auto const sql = cyng::sql::insert(db.get_dialect(), meta).bind_values(meta).to_str();
            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {
                BOOST_ASSERT(r.first == data.size() + key.size() + 1);

                //
                //	pk
                //
                std::size_t col_index{0};
                for (auto &kp : key) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(kp, width); //	pk
                    ++col_index;
                }

                //
                //	gen
                //
                stmt->push(cyng::make_object(gen), 0);
                ++col_index;

                //
                //	body
                //
                for (auto &val : data) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(val, width);
                    ++col_index;
                }

                if (stmt->execute()) {
                    stmt->clear();
                }
            }

            return r.second;
        }

        bool persistent_update(
            cyng::meta_sql const &meta,
            cyng::db::session db,
            cyng::key_t key,
            cyng::attr_t const &attr,
            std::uint64_t gen) {

            auto const sql = cyng::sql::update(db.get_dialect(), meta).set_placeholder(attr.first).where(cyng::sql::pk())();

            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {
                BOOST_ASSERT(r.first == 2 + key.size()); //	attribute to "gen"

                //
                //  attribute
                //
                auto const width = meta.get_body_column(attr.first).width_;
                stmt->push(attr.second, width);
                stmt->push(cyng::make_object(gen), 0); //	generation

                //
                //	pk
                //
                std::size_t col_index{0};
                for (auto &kp : key) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(kp, width); //	pk
                    ++col_index;
                }

                if (stmt->execute()) {
                    stmt->clear();
                    return true;
                }
            }
            return false;
        }

        bool persistent_remove(cyng::meta_sql const &meta, cyng::db::session db, cyng::key_t key) {
            auto const sql = cyng::sql::remove(db.get_dialect(), meta).where(cyng::sql::pk())();

            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {

                BOOST_ASSERT(r.first == key.size());

                //
                //	pk
                //
                std::size_t col_index{0};
                for (auto &kp : key) {
                    auto const width = meta.get_column(col_index).width_;
                    stmt->push(kp, width); //	pk
                    ++col_index;
                }

                if (stmt->execute()) {
                    stmt->clear();
                    return true;
                }
            }
            return false;
        }

        bool persistent_clear(cyng::meta_sql const &meta, cyng::db::session db) {
            auto const sql = cyng::sql::remove(db.get_dialect(), meta)();
            auto stmt = db.create_statement();
            std::pair<int, bool> const r = stmt->prepare(sql);
            if (r.second) {

                BOOST_ASSERT(r.first == 0);

                if (stmt->execute()) {
                    stmt->clear();
                    return true;
                }
            }
            return false;
        }

    } // namespace config
} // namespace smf
