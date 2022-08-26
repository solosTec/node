#
#	get time stamp of current build
#

# get timestamp and build a patch level from the year and
# the day of the year. This is a unique number to distinguish
# different builds
string(TIMESTAMP _THIS_YEAR "%Y")

# Patch level as year + day of the year
string(TIMESTAMP _TWEAK_ID "%j")	# day of the year
math(EXPR _TWEAK_ID "((${_THIS_YEAR} - 2022) * 1000) + ${_TWEAK_ID}")

string(TIMESTAMP VERSION_TIMESTAMP "%s")	# seconds since 1970-01-01 00:00:00 UTC

#
#	get a UTC timestamp
#	use this information instead of __DATE__ and __TIME__ macros 
#	to guarantee identical builds from identical sources
#
string(TIMESTAMP _NOW_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)
