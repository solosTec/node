//#include <smf/mbus/units.h>
#include <smf/obis/defs.h>
#include <smf/sml/select.h>
//#include <smf/sml/readout.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SML
#include <cyng/io/ostream.h>
#include <iostream>
#endif
#ifdef _DEBUG
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#endif
#include <boost/assert.hpp>

namespace smf {
    namespace sml {
        // void select_devices(cyng::tuple_t const &tpl) {
        //   -- path: 81811106ffff
        //   -- code: 81811106ffff - ROOT_ACTIVE_DEVICES
        //   -- attr: (0:null)
        //   -- list:
        //   {{8181110601ff,(0:null),
        //       {{818111060101,(0:null),
        //           {{8181c78204ff,(1:0a000001),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:10+0100),{}}}},
        //       {818111060102,(0:null),
        //           {{8181c78204ff,(1:0a000002),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:11+0100),{}}}},
        //       {818111060103,(0:null),
        //           {{8181c78204ff,(1:0a000003),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:12+0100),{}}}},
        //       {818111060104,(0:null),
        //           {{8181c78204ff,(1:0a000004),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:13+0100),{}}}},
        //       {818111060105,(0:null),
        //           {{8181c78204ff,(1:0aa00001),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:14+0100),{}}}},
        //       {818111060106,(0:null),
        //           {{8181c78204ff,(1:0ab00001),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:16+0100),{}}}},
        //       {818111060107,(0:null),
        //           {{8181c78204ff,(1:0ad00001),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:17+0100),{}}}},
        //       {818111060108,(0:null),
        //           {{8181c78204ff,(1:0ae00001),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:39:18+0100),{}}}},
        //       {818111060109,(0:null),
        //           {{8181c78204ff,(1:01a815743145040102),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:42:32+0100),{}}}},
        //       {81811106010a,(0:null),
        //           {{8181c78204ff,(1:01e61e571406213603),{}},
        //           {8181c78202ff,(1:2d2d2d),{}},
        //           {010000090b00,(4:2022-01-18T19:41:52+0100),{}}}}}}}

        //  ServerId: 05 00 15 3B 01 EC 46
        //  81 81 11 06 FF FF                Not set
        //     81 81 11 06 01 FF             Not set
        //        81 81 11 06 01 01          Not set
        //           81 81 C7 82 04 FF       ____ (0A 00 00 01 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534750 (timestamp)
        //        81 81 11 06 01 02          Not set
        //           81 81 C7 82 04 FF       ____ (0A 00 00 02 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534751 (timestamp)
        //        81 81 11 06 01 03          Not set
        //           81 81 C7 82 04 FF       ____ (0A 00 00 03 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534752 (timestamp)
        //        81 81 11 06 01 04          Not set
        //           81 81 C7 82 04 FF       ____ (0A 00 00 04 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534753 (timestamp)
        //        81 81 11 06 01 05          Not set
        //           81 81 C7 82 04 FF       ____ (0A A0 00 01 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534754 (timestamp)
        //        81 81 11 06 01 06          Not set
        //           81 81 C7 82 04 FF       ____ (0A B0 00 01 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534756 (timestamp)
        //        81 81 11 06 01 07          Not set
        //           81 81 C7 82 04 FF       ____ (0A D0 00 01 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534757 (timestamp)
        //        81 81 11 06 01 08          Not set
        //           81 81 C7 82 04 FF       ____ (0A E0 00 01 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534758 (timestamp)
        //        81 81 11 06 01 09          Not set
        //           81 81 C7 82 04 FF       ___t1E___ (01 A8 15 74 31 45 04 01 02 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534952 (timestamp)
        //        81 81 11 06 01 0A          Not set
        //           81 81 C7 82 04 FF       ___W__!6_ (01 E6 1E 57 14 06 21 36 03 )
        //           81 81 C7 82 02 FF       --- (2D 2D 2D )
        //           01 00 00 09 0B 00       1642534912 (timestamp)
        //    if (!tpl.empty()) {
        //        BOOST_ASSERT(tpl.size() == 1);
        //        std::cerr << std::endl << std::endl << "tpl: " << cyng::io::to_typed(tpl) << std::endl << std::endl <<
        //        std::endl;
        //        // std::cout << tpl.size() << std::endl;
        //        auto const [code, list] = get_list(tpl.front());
        //        BOOST_ASSERT(code == cyng::make_obis(0x81, 0x81, 0x11, 0x06, 0x01, 0xff));
        //        for (auto const &devs : list) {
        //            // std::cerr << "device: " << dev << std::endl;
        //            auto const [code, dev] = get_list(devs);
        //            // std::cerr << "dev: " << dev << std::endl;
        //            //
        //            {{8181c78204ff,(1:0a000001),{}},{8181c78202ff,(1:---),{}},{010000090b00,(4:2022-01-19T17:24:11+0100),{}}}
        //            cyng::prop_map_t om;
        //            for (auto const &v : dev) {
        //                auto const [c, attr] = get_attribute(v);
        //                om.emplace(c, attr.second);
        //                std::cerr << om << std::endl;
        //                //  8181c78204ff: 0a000001
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:11+0100
        //                //  8181c78204ff: 0a000002
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:12+0100
        //                //  8181c78204ff: 0a000003
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:13+0100
        //                //  8181c78204ff: 0a000004
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:14+0100
        //                //  8181c78204ff: 0aa00001
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:16+0100
        //                //  8181c78204ff: 0ab00001
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:17+0100
        //                //  8181c78204ff: 0ad00001
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:18+0100
        //                //  8181c78204ff: 0ae00001
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:34:19+0100
        //                //  8181c78204ff: 01a815743145040102
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:38:29+0100
        //                //  8181c78204ff: 01e61e571406213603
        //                //  8181c78202ff: ---
        //                //  010000090b00: 2022-01-19T17:38:57+0100
        //            }
        //        }
        //    }
        //}

        namespace {
            std::size_t select(
                cyng::tuple_t const &tpl,
                cyng::obis root,
                std::size_t idx,
                std::function<void(cyng::prop_map_t const &, std::size_t)> cb) {
                auto depth = cyng::depth(root);
                if (depth > 0) {
                    for (auto const &v : tpl) {
                        auto const [code, list] = get_list(v);
                        idx += select(list, code, idx, cb);
                    }
                } else {
                    //  data level
                    cyng::prop_map_t om;
                    for (auto const &v : tpl) {
                        auto const [c, attr] = get_attribute(v);
                        om.emplace(c, attr.second);
                    }
                    // std::cerr << '#' << idx << " - " << om << std::endl;
                    cb(om, idx);
                    ++idx;
                }
                return idx;
            }
        } // namespace

        void select(cyng::tuple_t const &tpl, cyng::obis root, std::function<void(cyng::prop_map_t const &, std::size_t)> cb) {
            select(tpl, root, 0, cb);
        }

        std::pair<cyng::obis, cyng::tuple_t> get_list(cyng::object const &obj) {
            auto const tpl = cyng::container_cast<cyng::tuple_t>(obj);
            return get_list(tpl);
        }

        std::pair<cyng::obis, cyng::tuple_t> get_list(cyng::tuple_t const &tpl) {
            BOOST_ASSERT(tpl.size() == 3);
            if (tpl.size() == 3 && tpl.front().rtti().tag() == cyng::TC_OBIS) {
                return {cyng::value_cast(tpl.front(), cyng::obis()), cyng::container_cast<cyng::tuple_t>(std::move(tpl.back()))};
            }
            return {{}, {}};
        }

        std::pair<cyng::obis, cyng::attr_t> get_attribute(cyng::object const &obj) {
            auto const list = cyng::container_cast<cyng::tuple_t>(obj);
            return get_attribute(list);
        }

        std::pair<cyng::obis, cyng::attr_t> get_attribute(cyng::tuple_t const &tpl) {
            BOOST_ASSERT(tpl.size() == 3);

            if (tpl.size() == 3 && tpl.front().rtti().tag() == cyng::TC_OBIS) {
                auto pos = std::next(tpl.begin()); //  second element
                return {cyng::value_cast(tpl.front(), cyng::obis()), cyng::value_cast(*pos, cyng::attr_t())};
            }
            return {{}, {}};
        }

    } // namespace sml
} // namespace smf
