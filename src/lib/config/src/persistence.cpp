#include <smf/config/persistence.h>

#include <cyng/sql/sql.hpp>

#include <boost/assert.hpp>

namespace smf {
    namespace config {
        bool
        persistent_insert(cyng::meta_sql const &meta, cyng::db::session db, cyng::key_t key, cyng::data_t data, std::uint64_t gen) {
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
    } // namespace config
} // namespace smf
