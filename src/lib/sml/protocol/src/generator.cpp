#include <smf/sml/generator.h>

#include <algorithm>

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        trx::trx()
            : gen_(cyng::crypto::make_rnd_num())
            , value_()
            , num_(1) {
            regenerate(7);
        }

        trx::trx(trx const &other)
            : gen_(cyng::crypto::make_rnd_num())
            , value_(other.value_)
            , num_(other.num_) {}

        void trx::regenerate(std::size_t length) {
            value_.resize(length);
            std::generate(value_.begin(), value_.end(), [&]() { return gen_.next(); });
        }

        trx &trx::operator++() {
            ++num_;
            return *this;
        }

        trx trx::operator++(int) {
            trx tmp(*this);
            ++num_;
            return tmp;
        }

        std::string trx::operator*() const { return value_ + "-" + std::to_string(num_); }

        std::string generate_file_id() {

            static cyng::crypto::rnd gen("0123456789");

            std::string id;
            id.resize(12);
            std::generate(id.begin(), id.end(), [&]() { return gen.next(); });
            return id;
        }

    } // namespace sml
} // namespace smf
