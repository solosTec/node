/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/iec/parser.h>

namespace smf
{
	namespace iec
	{
		/**
		 *	IEC 62056-21 parser (readout mode).
		 */
		class parser
		{
		private:
			/**
			* This enum stores the global state
			* of the parser. For each state there
			* are different helper variables mostly
			* declared in the private section of this class.
			*/
			enum class state
			{
				_ERROR,
				START,		//!< initial state
				STX,			//!< after STX
				DATA_BLOCK,	//!< data block
				DATA_LINE,
				DATA_SET,
				OBIS,			//!< ID
				CHOICE_VALUE,
				CHOICE_STATUS,
				VALUE,
				STATUS,	//!< new;old status
				UNIT,
				ETX,
				BCC,	//!< XOR of all characters after STX and before ETX
				//STATE_EOF	//!> after !
			};

		};
	}
}


