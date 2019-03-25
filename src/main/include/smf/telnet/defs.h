/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */


#ifndef NODE_TELNET_DEFS_H
#define NODE_TELNET_DEFS_H

#include <string>
#include <cstdint>

namespace node
{
	namespace telnet
	{
		/**
		 * Implementation of a statefull telnet parser according to
		 *
		 * <ul>
		 * <li>http://www.faqs.org/rfcs/rfc854.html</li>
		 * <li>http://www.faqs.org/rfcs/rfc855.html</li>
		 * <li>http://www.faqs.org/rfcs/rfc1091.html</li>
		 * <li>http://www.faqs.org/rfcs/rfc1143.html</li>
		 * <li>http://www.faqs.org/rfcs/rfc1408.html</li>
		 * <li>http://www.faqs.org/rfcs/rfc1572.html</li>
		 * </ul>
		 */
		
		enum commands : std::uint8_t
		{
			TELNET_IAC = 255,
			TELNET_DONT = 254,
			TELNET_DO = 253,
			TELNET_WONT = 252,
			TELNET_WILL = 251,
			TELNET_SB = 250,
			TELNET_GA = 249,
			TELNET_EL = 248,
			TELNET_EC = 247,
			TELNET_AYT = 246,
			TELNET_AO = 245,
			TELNET_IP = 244,
			TELNET_BREAK = 243,
			TELNET_DM = 242,
			TELNET_NOP = 241,
			TELNET_SE = 240,
			TELNET_EOR = 239,
			TELNET_ABORT = 238,
			TELNET_SUSP = 237,
			TELNET_EOF = 236
		};

		enum options : std::uint8_t
		{
			TELOPT_BINARY = 0,
			TELOPT_ECHO = 1,
			TELOPT_RCP = 2,
			TELOPT_SGA = 3,
			TELOPT_NAMS = 4,
			TELOPT_STATUS = 5,
			TELOPT_TM = 6,
			TELOPT_RCTE = 7,
			TELOPT_NAOL = 8,
			TELOPT_NAOP = 9,
			TELOPT_NAOCRD = 10,
			TELOPT_NAOHTS = 11,
			TELOPT_NAOHTD = 12,
			TELOPT_NAOFFD = 13,
			TELOPT_NAOVTS = 14,
			TELOPT_NAOVTD = 15,
			TELOPT_NAOLFD = 16,
			TELOPT_XASCII = 17,
			TELOPT_LOGOUT = 18,
			TELOPT_BM = 19,
			TELOPT_DET = 20,
			TELOPT_SUPDUP = 21,
			TELOPT_SUPDUPOUTPUT = 22,
			TELOPT_SNDLOC = 23,
			TELOPT_TTYPE = 24,
			TELOPT_EOR = 25,
			TELOPT_TUID = 26,
			TELOPT_OUTMRK = 27,
			TELOPT_TTYLOC = 28,
			TELOPT_3270REGIME = 29,
			TELOPT_X3PAD = 30,
			TELOPT_NAWS = 31,
			TELOPT_TSPEED = 32,
			TELOPT_LFLOW = 33,
			TELOPT_LINEMODE = 34,
			TELOPT_XDISPLOC = 35,
			TELOPT_ENVIRON = 36,
			TELOPT_AUTHENTICATION = 37,
			TELOPT_ENCRYPT = 38,
			TELOPT_NEW_ENVIRON = 39,
			TELOPT_MSSP = 70,
			TELOPT_COMPRESS = 85,
			TELOPT_COMPRESS2 = 86,
			TELOPT_ZMP = 93,
			TELOPT_EXOPL = 255

			//TELOPT_MCCP2 = 86
		};

		enum error_t : std::uint32_t
		{
			TEL_EOK = 0,   //!< no error 
			TEL_EBADVAL,   //!< invalid parameter, or API misuse 
			TEL_ENOMEM,    //!< memory allocation failure 
			TEL_EOVERFLOW, //!< data exceeds buffer size 
			TEL_EPROTOCOL, //!< invalid sequence of special bytes
			TEL_ECOMPRESS  //!< error handling compressed streams 
		};

		enum events : std::uint32_t
		{
			EV_DATA = 0,        //!< raw text data has been received
			EV_SEND,            //!< data needs to be sent to the peer
			EV_IAC,             //!< generic IAC code received
			EV_WILL,            //!< WILL option negotiation received
			EV_WONT,            //!< WONT option neogitation received
			EV_DO,              //!< DO option negotiation received
			EV_DONT,            //!< DONT option negotiation received
			EV_SUBNEGOTIATION,  //!< sub-negotiation data received
			EV_COMPRESS,        //!< compression has been enabled
			EV_ZMP,             //!< ZMP command has been received
			EV_TTYPE,           //!< TTYPE command has been received
			EV_ENVIRON,         //!< ENVIRON command has been received
			EV_MSSP,            //!< MSSP command has been received
			EV_WARNING,         //!< recoverable error has occured
			EV_ERROR            //!< non-recoverable error has occured
		};

		//
		//	SLC options RFC 1184
		//
		enum slc : std::uint8_t
		{
			SLC_SYNCH = 1,
			SLC_BRK = 2,
			SLC_IP = 3,
			SLC_AO = 4,
			SLC_AYT = 5,
			SLC_EOR = 6,
			SLC_ABORT = 7,
			SLC_EOF = 8,
			SLC_SUSP = 9,
			SLC_EC = 10,	//	0x0A - Erase Character
			SLC_EL = 11,	//	0x0B - Erase Line
			SLC_EW = 12,	//	0x0C - Erase Word
			SLC_RP = 13,	//	0x0D - Reprint Line
			SLC_LNEXT = 14,	//	0x0E - Literal Next - next character is to taken literal
			SLC_XON = 15,	//	0x0F - Start Output - resume output
			SLC_XOFF = 16,
			SLC_FORW1 = 17,
			SLC_FORW2 = 18,
			SLC_MCL = 19,
			SLC_MCR = 20,
			SLC_MCWL = 21,
			SLC_MCWR = 22,
			SLC_MCBOL = 23,
			SLC_MCEOL = 24,
			SLC_INSRT = 25,
			SLC_OVER = 26,
			SLC_ECR = 27,
			SLC_EWR = 28,
			SLC_EBOL = 29,
			SLC_EEOL = 30
		};

		constexpr char crlf_[] = { '\r', '\n' };
		constexpr char crnul_[] = { '\r', '\0' };
	}
}


#endif	//	NODE_TELNET_DEFS_H