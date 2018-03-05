//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_EXAMPLE_COMMON_SERVER_CERTIFICATE_HPP
#define BOOST_BEAST_EXAMPLE_COMMON_SERVER_CERTIFICATE_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>

/*  Load a signed certificate into the ssl context, and configure
the context for use with a server.

For this to work with the browser or operating system, it is
necessary to import the "Beast Test CA" certificate into
the local certificate store, browser, or operating system
depending on your environment Please see the documentation
accompanying the Beast certificate for more details.
*/
inline
void
load_server_certificate(boost::asio::ssl::context& ctx)
{
	/*
	The certificate was generated from CMD.EXE on Windows 10 using:

	winpty openssl dhparam -out dh.pem 2048
	winpty openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=US\ST=CA\L=Los Angeles\O=Beast\CN=www.example.com"
	*/

	std::string const cert =
		"-----BEGIN CERTIFICATE-----\n"
		//"MIIDaDCCAlCgAwIBAgIJAO8vBu8i8exWMA0GCSqGSIb3DQEBCwUAMEkxCzAJBgNV\n"
		//"BAYTAlVTMQswCQYDVQQIDAJDQTEtMCsGA1UEBwwkTG9zIEFuZ2VsZXNPPUJlYXN0\n"
		//"Q049d3d3LmV4YW1wbGUuY29tMB4XDTE3MDUwMzE4MzkxMloXDTQ0MDkxODE4Mzkx\n"
		//"MlowSTELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMS0wKwYDVQQHDCRMb3MgQW5n\n"
		//"ZWxlc089QmVhc3RDTj13d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\n"
		//"A4IBDwAwggEKAoIBAQDJ7BRKFO8fqmsEXw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcF\n"
		//"xqGitbnLIrOgiJpRAPLy5MNcAXE1strVGfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7b\n"
		//"Fu8TsCzO6XrxpnVtWk506YZ7ToTa5UjHfBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO\n"
		//"9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wWKIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBp\n"
		//"yY8anC8u4LPbmgW0/U31PH0rRVfGcBbZsAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrv\n"
		//"enu2tOK9Qx6GEzXh3sekZkxcgh+NlIxCNxu//Dk9AgMBAAGjUzBRMB0GA1UdDgQW\n"
		//"BBTZh0N9Ne1OD7GBGJYz4PNESHuXezAfBgNVHSMEGDAWgBTZh0N9Ne1OD7GBGJYz\n"
		//"4PNESHuXezAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCmTJVT\n"
		//"LH5Cru1vXtzb3N9dyolcVH82xFVwPewArchgq+CEkajOU9bnzCqvhM4CryBb4cUs\n"
		//"gqXWp85hAh55uBOqXb2yyESEleMCJEiVTwm/m26FdONvEGptsiCmF5Gxi0YRtn8N\n"
		//"V+KhrQaAyLrLdPYI7TrwAOisq2I1cD0mt+xgwuv/654Rl3IhOMx+fKWKJ9qLAiaE\n"
		//"fQyshjlPP9mYVxWOxqctUdQ8UnsUKKGEUcVrA08i1OAnVKlPFjKBvk+r7jpsTPcr\n"
		//"9pWXTO9JrYMML7d+XRSZA1n3856OqZDX4403+9FnXCvfcLZLLKTBvwwFgEFGpzjK\n"
		//"UEVbkhd5qstF6qWK\n"
		"MIIC1jCCAb6gAwIBAgIJAIA++QcqOgT5MA0GCSqGSIb3DQEBCwUAMAAwHhcNMTgw\n"
		"MjI4MDAwNjM2WhcNNDUwNzE2MDAwNjM2WjAAMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
		"AQ8AMIIBCgKCAQEArcSoBvNg6iNLfryyeanjeM8akBMLxJPM69Ar3K+gtacuvsvc\n"
		"JyPaZ0+HXYZyXZ4dI8/AuVZID6ESKq5HxNYtzuHuH7oFNm0Yd7nlM/nJdyA78vBb\n"
		"12htq8Me4rg01AaIYrxczwm9GHFl4qy/CHpHJ6TA7mIaa6qXtcoFOMQh1yKDcsQi\n"
		"Cn2E6r9N/lwsTorr+HRqsnaeLyLekhWlYxInWCGXe8o8un7vhNXWnDno/0RIUID5\n"
		"rOKtDBZrFbngurxpDdcMgvnig4SHoaa8sq5ul+xc9x4SVRecWPtNCDvA/K+4jgkm\n"
		"93RByScCQ7m2ROeE1LpNwDCHUH10emb7WdKcUQIDAQABo1MwUTAdBgNVHQ4EFgQU\n"
		"Bi2HmS31plRxUZTJcW2WvtircKowHwYDVR0jBBgwFoAUBi2HmS31plRxUZTJcW2W\n"
		"vtircKowDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAeQAA1ZDv\n"
		"S/Tsozk3FsITgU+/+KuxKBza3dYsS1psWqTnYb2HsxSoZUoho3RxA4BHDtJJ299I\n"
		"yG8xyb9NSrSOBoEjoWL8z8X7INf6ipLYtjG1Bcmalkc53TlYkR6nuE8dPvqb9N4R\n"
		"5Pq2IOWF2zFssIW06MncfuRo2ERFZcK4bmEz1/AaIlcPlilAHb7ciCf0tRokUafn\n"
		"gyPoHFxKOjslCTBT/WmY72VrS10Ypk0DqcE7IcgT/wYfY64Kn69jsiHmwlm4dqTw\n"
		"7bz9ob96UrCOMzlDj23vvuuyGKkUdzXYDob2VwqrqjNpuB/+cMElEE6w8i3OQ2Cj\n"
		"KvnMBplhJIP+IA==\n"
		"-----END CERTIFICATE-----\n";

	std::string const key =
		"-----BEGIN PRIVATE KEY-----\n"
		//"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDJ7BRKFO8fqmsE\n"
		//"Xw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcFxqGitbnLIrOgiJpRAPLy5MNcAXE1strV\n"
		//"GfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7bFu8TsCzO6XrxpnVtWk506YZ7ToTa5UjH\n"
		//"fBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wW\n"
		//"KIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBpyY8anC8u4LPbmgW0/U31PH0rRVfGcBbZ\n"
		//"sAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrvenu2tOK9Qx6GEzXh3sekZkxcgh+NlIxC\n"
		//"Nxu//Dk9AgMBAAECggEBAK1gV8uETg4SdfE67f9v/5uyK0DYQH1ro4C7hNiUycTB\n"
		//"oiYDd6YOA4m4MiQVJuuGtRR5+IR3eI1zFRMFSJs4UqYChNwqQGys7CVsKpplQOW+\n"
		//"1BCqkH2HN/Ix5662Dv3mHJemLCKUON77IJKoq0/xuZ04mc9csykox6grFWB3pjXY\n"
		//"OEn9U8pt5KNldWfpfAZ7xu9WfyvthGXlhfwKEetOuHfAQv7FF6s25UIEU6Hmnwp9\n"
		//"VmYp2twfMGdztz/gfFjKOGxf92RG+FMSkyAPq/vhyB7oQWxa+vdBn6BSdsfn27Qs\n"
		//"bTvXrGe4FYcbuw4WkAKTljZX7TUegkXiwFoSps0jegECgYEA7o5AcRTZVUmmSs8W\n"
		//"PUHn89UEuDAMFVk7grG1bg8exLQSpugCykcqXt1WNrqB7x6nB+dbVANWNhSmhgCg\n"
		//"VrV941vbx8ketqZ9YInSbGPWIU/tss3r8Yx2Ct3mQpvpGC6iGHzEc/NHJP8Efvh/\n"
		//"CcUWmLjLGJYYeP5oNu5cncC3fXUCgYEA2LANATm0A6sFVGe3sSLO9un1brA4zlZE\n"
		//"Hjd3KOZnMPt73B426qUOcw5B2wIS8GJsUES0P94pKg83oyzmoUV9vJpJLjHA4qmL\n"
		//"CDAd6CjAmE5ea4dFdZwDDS8F9FntJMdPQJA9vq+JaeS+k7ds3+7oiNe+RUIHR1Sz\n"
		//"VEAKh3Xw66kCgYB7KO/2Mchesu5qku2tZJhHF4QfP5cNcos511uO3bmJ3ln+16uR\n"
		//"GRqz7Vu0V6f7dvzPJM/O2QYqV5D9f9dHzN2YgvU9+QSlUeFK9PyxPv3vJt/WP1//\n"
		//"zf+nbpaRbwLxnCnNsKSQJFpnrE166/pSZfFbmZQpNlyeIuJU8czZGQTifQKBgHXe\n"
		//"/pQGEZhVNab+bHwdFTxXdDzr+1qyrodJYLaM7uFES9InVXQ6qSuJO+WosSi2QXlA\n"
		//"hlSfwwCwGnHXAPYFWSp5Owm34tbpp0mi8wHQ+UNgjhgsE2qwnTBUvgZ3zHpPORtD\n"
		//"23KZBkTmO40bIEyIJ1IZGdWO32q79nkEBTY+v/lRAoGBAI1rbouFYPBrTYQ9kcjt\n"
		//"1yfu4JF5MvO9JrHQ9tOwkqDmNCWx9xWXbgydsn/eFtuUMULWsG3lNjfst/Esb8ch\n"
		//"k5cZd6pdJZa4/vhEwrYYSuEjMCnRb0lUsm7TsHxQrUd6Fi/mUuFU/haC0o0chLq7\n"
		//"pVOUFq5mW8p0zbtfHbjkgxyF\n"
		"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCtxKgG82DqI0t+\n"
		"vLJ5qeN4zxqQEwvEk8zr0Cvcr6C1py6+y9wnI9pnT4ddhnJdnh0jz8C5VkgPoRIq\n"
		"rkfE1i3O4e4fugU2bRh3ueUz+cl3IDvy8FvXaG2rwx7iuDTUBohivFzPCb0YcWXi\n"
		"rL8IekcnpMDuYhprqpe1ygU4xCHXIoNyxCIKfYTqv03+XCxOiuv4dGqydp4vIt6S\n"
		"FaVjEidYIZd7yjy6fu+E1dacOej/REhQgPms4q0MFmsVueC6vGkN1wyC+eKDhIeh\n"
		"pryyrm6X7Fz3HhJVF5xY+00IO8D8r7iOCSb3dEHJJwJDubZE54TUuk3AMIdQfXR6\n"
		"ZvtZ0pxRAgMBAAECggEBAIsPiSRe2t0lN8KKAg5pTdgdbXWFOHKtkV3Z73AhwOv+\n"
		"ieM4w8sy3xK0S3EmKhoPceR52xK3IN4ZGa+8X0T/3hLlLaqINKm0rtMJmop4yKij\n"
		"zDYD8ou1T6cYdHwdzHEtdTIG6gLqGUEZZt77PbnsGUt5hsh/DAPDtrtNm9Ys56QA\n"
		"8jUb0wLPpgf5qRXoDo9VUf9Y0+OSlW6ojjwWm1+Bc6AADdM3KZ4hbQLq84gqOXs9\n"
		"eETM//k3p+Qfx+6rWjx4d7Y4UgDAx/b9JHy21KLiNi0I7klmu9PehXMBQBpsWT3w\n"
		"jLZB5x7uNsjTjTf8BFDrwLmhKRcRxo0QrrWeND7M400CgYEA1VP1iPqixOqvzGFN\n"
		"PDZKdjt3AoOo/eH6avT5IOfDdnBzu7yL9nsoKetKEVf3iODdgtaYgW8FnkxK9hu3\n"
		"iJXzItZR4lOwM4U5bypJAD+6n1LGN/l7otmOBFnRtixgmCfsv2z0x7dX+uO5as1f\n"
		"YsK3LrXazynUOA53d9hELdp16EMCgYEA0IbuThg47pPkQSGUe6Tc3Sz2rLx18lZQ\n"
		"pyy5c/Vij7nQFex8hOynEMlvT7vPDk8XQWgOeGWT/dtSK681orQhp+J7LosPDE6J\n"
		"1IvUxS7IjsoSMtx8mw7pT5ewaP4XDW1WLLu8HB8IzDvHP3BBUtrTH5i9jrQkFLqs\n"
		"eR6yl2PqOdsCgYEAjk4xrqyzQ/TiTM5jvVTiGzjTzOOTKblDWXINdnvkke+15HiE\n"
		"TWoegsgoYqVxxOdsHMmWdlFfSBfQsZgPuJd+17BsczQsiFHI3HUyuW3JylpnTBOq\n"
		"/BlweUqJcKLt1NJdRd0i9M9Da2PZ3nsdtD38ALbjPerDXJmZ7GJiKMxgdw0CgYAb\n"
		"oLT0Hdt1KJ0GUBenJhmpKCrqifGqkOsQqylLBsjvN/Qs429AAUbFP5sC2mQ9hhcT\n"
		"sGCybOrlqGhDp2wYyXroDma5rOzqeYFjar9e/KrP2E/+8x2DQb+BrxxNXNTbD5Bq\n"
		"TtlGdIoq3QSyEAJnotx0BD2hKZbaND1jssCAtFk1HwKBgCCv/6OYrWCEe0GE6QGY\n"
		"+vqzOF2jojlx681bj3qn37+j1WYDD38aTk2UjdDsYegJFqgHBlj/+vMLs+XiStTK\n"
		"gVJXyEMIJuZn4gxeiH5v8l3iQiaGvJCwkUIveYXrj0ca7AwT9JBPKkid+PDAgPXh\n"
		"btU9uQ/qzH2kDyaZg4FGTTY5\n"
		"-----END PRIVATE KEY-----\n";

	std::string const dh =
		"-----BEGIN DH PARAMETERS-----\n"
		//"MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
		//"/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
		//"4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
		//"tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
		//"oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
		//"QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
		"MIIBCAKCAQEA9VTzDc1fJLRTnNMyF04ka323hlMg5mENKUZwQJkmPTyFas9XlMNW\n"
		"ZUFlQR2lMK2qCLb5ijqI5GGiOu+RMu2NpDKtj7Puwhys3Kg6NhyNGOgx+p2mTNX8\n"
		"rBgbs1GAOuImrbV03H/JhsVV6kVO5yySPc0A4o7PxPYkOORqI1Uaujf0xeGAQS5W\n"
		"cnlhXrxj5bVhKjo6lc+YWBAWjw2P4K0BlVvisV2MIvh0p2XA9Y4GvlICmRiOtJbS\n"
		"5vwprVwf697etoEqQy2x++ggmJo5F+MIuEZ4TRX5e++xbeeFx5SUO/HIySWM7e9L\n"
		"UvTUfvTrEETqPkQda0tBd4uA4H2lg7pwkwIBAg==\n"
		"-----END DH PARAMETERS-----\n";

	ctx.set_password_callback(
		[](std::size_t,
			boost::asio::ssl::context_base::password_purpose)
	{
		return "test";
	});

	ctx.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2 |
		boost::asio::ssl::context::single_dh_use);

	ctx.use_certificate_chain(
		boost::asio::buffer(cert.data(), cert.size()));

	ctx.use_private_key(
		boost::asio::buffer(key.data(), key.size()),
		boost::asio::ssl::context::file_format::pem);

	ctx.use_tmp_dh(
		boost::asio::buffer(dh.data(), dh.size()));
}

#endif
