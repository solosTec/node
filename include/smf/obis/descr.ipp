case 0xff: return "device address - 0-0:0.0.0.255";	// device address - 0-0:0.0.0.255
case 0x1ff: return "0-0:0.0.1.255";	// 0-0:0.0.1.255
case 0x2ff: return "0-0:0.0.2.255";	// 0-0:0.0.2.255
case 0x3ff: return "0-0:0.0.3.255";	// 0-0:0.0.3.255
case 0x100ff: return "0.1.0";	// 0.1.0
case 0x200ff: return "COSEM class id 1";	// COSEM class id 1
case 0x10000ff: return "current time";	// current time
case 0x600100ff: return "(C.1.0) Serial number I (assigned by the manufacturer)";	// (C.1.0) Serial number I (assigned by the manufacturer)
case 0x600101ff: return "Serial number II (customer ID, assigned by the manufacturer).";	// Serial number II (customer ID, assigned by the manufacturer).
case 0x600102ff: return "Parameters file code (C.1.2)";	// Parameters file code (C.1.2)
case 0x600103ff: return "date of manufacture (C.1.3)";	// date of manufacture (C.1.3)
case 0x600104ff: return "Parameters check sum (C.1.4)";	// Parameters check sum (C.1.4)
case 0x600105ff: return "Firmware built date (C.1.5)";	// Firmware built date (C.1.5)
case 0x600106ff: return "Firmware check sum (C.1.6)";	// Firmware check sum (C.1.6)
case 0x6001ffff: return "fabrication number";	// fabrication number
case 0x600201ff: return "Date of last parameterisation (00-03-26)";	// Date of last parameterisation (00-03-26)
case 0x600300ff: return "Active pulse constant (C.3.0)";	// Active pulse constant (C.3.0)
case 0x600301ff: return "Reactive pulse constant (C.3.1)";	// Reactive pulse constant (C.3.1)
case 0x600600ff: return "Power down time counter (C.6.0)";	// Power down time counter (C.6.0)
case 0x600601ff: return "Battery remaining capacity (C.6.1)";	// Battery remaining capacity (C.6.1)
case 0x600602ff: return "Date of last battery change (C.6.2)";	// Date of last battery change (C.6.2)
case 0x600603ff: return "Battery Voltage (C.6.3)";	// Battery Voltage (C.6.3)
case 0x600700ff: return "Number of power failures ";	// Number of power failures 
case 0x600800ff: return "[SML_Time] seconds index";	// [SML_Time] seconds index
case 0x601000ff: return "";
case 0x604800ff: return "Number of under-voltages";	// Number of under-voltages
case 0x604900ff: return "Number of over-voltages";	// Number of over-voltages
case 0x604a00ff: return "Number of over-loads (over-current) ";	// Number of over-loads (over-current) 
case 0x60f00dff: return "name";	// name
case 0x616100ff: return "Status according to EN13757-3 (error register)";	// Status according to EN13757-3 (error register)
case 0x616101ff: return "not relevant under calibration law";	// not relevant under calibration law
case 0x616102ff: return "not relevant under calibration law";	// not relevant under calibration law
case 0x616103ff: return "not relevant under calibration law";	// not relevant under calibration law
case 0x1600205ff: return "Date and time of last calibration (L&G)";	// Date and time of last calibration (L&G)
case 0x260f008ff: return "Hardware ID of extension board (L&G)";	// Hardware ID of extension board (L&G)
case 0x260f009ff: return "Reference hardware ID of extension board (L&G)";	// Reference hardware ID of extension board (L&G)
case 0x80800000ff: return "root push operations";	// root push operations
case 0x8080000101: return "WAN on customer interface";	// WAN on customer interface
case 0x8080000301: return "SSL/TSL configuration available";	// SSL/TSL configuration available
case 0x80800004ff: return "list of SSL certificates";	// list of SSL certificates
case 0x80800010ff: return "request memory usage";	// request memory usage
case 0x80800011ff: return "";
case 0x80800012ff: return "";
case 0x80800100ff: return "list some basic security information";	// list some basic security information
case 0x80800101ff: return "";
case 0x80800102ff: return "";
case 0x80800105ff: return "3";	// 3
case 0x8080010610: return "3";	// 3
case 0x80800106ff: return "3";	// 3
case 0x80800107ff: return "1";	// 1
case 0x8080100001: return "LSM status";	// LSM status
case 0x80801100ff: return "list of actuators";	// list of actuators
case 0x80801110ff: return "ServerID des Aktors (uint16)";	// ServerID des Aktors (uint16)
case 0x8080111101: return "Schaltanforderung";	// Schaltanforderung
case 0x80801111ff: return "Connection state";	// Connection state
case 0x80801113ff: return "feedback configuration";	// feedback configuration
case 0x80801114ff: return "number of loads";	// number of loads
case 0x80801115ff: return "total power";	// total power
case 0x808011a0ff: return "see: 2.2.1.3 Status der Aktoren (Kontakte)";	// see: 2.2.1.3 Status der Aktoren (Kontakte)
case 0x808011a1ff: return "LSM version";	// LSM version
case 0x808011a2ff: return "LSM type";	// LSM type
case 0x808011a901: return "active ruleset";	// active ruleset
case 0x808011a902: return "";
case 0x80801403ff: return "Schaltauftrags ID ((octet string)	";	// Schaltauftrags ID ((octet string)	
case 0x80801420ff: return "current position";	// current position
case 0xb000020000: return "";
case 0xb000020001: return "readout interval in seconds % 3600";	// readout interval in seconds % 3600
case 0xb000020002: return "search interval in seconds";	// search interval in seconds
case 0xb000020003: return "search device now and by restart";	// search device now and by restart
case 0xb000020004: return "automatic activation of meters";	// automatic activation of meters
case 0xb000020005: return "used baud rates(bitmap)";	// used baud rates(bitmap)
case 0x100000000ff: return "Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)";	// Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
case 0x100000001ff: return "Identifikationsnummer 1.2";	// Identifikationsnummer 1.2
case 0x100000002ff: return "Identifikationsnummer 1.3";	// Identifikationsnummer 1.3
case 0x100000003ff: return "Identifikationsnummer 1.4";	// Identifikationsnummer 1.4
case 0x100000009ff: return "encode profiles";	// encode profiles
case 0x100000100ff: return "MD reset counter";	// MD reset counter
case 0x100000102ff: return "MD reset timestamp";	// MD reset timestamp
case 0x100000200ff: return "";
case 0x100000201ff: return "Parameters scheme ID";	// Parameters scheme ID
case 0x100000202ff: return "Tariff program ID";	// Tariff program ID
case 0x100000300ff: return "Active energy meter constant";	// Active energy meter constant
case 0x100000302ff: return "S0- Impulswertigkeit (0.3.2)";	// S0- Impulswertigkeit (0.3.2)
case 0x100000303ff: return "Pulse length (0.3.3)";	// Pulse length (0.3.3)
case 0x100000402ff: return "Converter factor (0.4.2)";	// Converter factor (0.4.2)
case 0x100000403ff: return "Voltage transformer (0.4.3)";	// Voltage transformer (0.4.3)
case 0x100000502ff: return "Measuring period (0.2.2)";	// Measuring period (0.2.2)
case 0x100000800ff: return "Duration of measurement interval for current power value (1-0.0:8.0*255)";	// Duration of measurement interval for current power value (1-0.0:8.0*255)
case 0x100000804ff: return "Load profile period";	// Load profile period
case 0x100000901ff: return "Current time at time of transmission (hh:mm:ss)";	// Current time at time of transmission (hh:mm:ss)
case 0x100000902ff: return "Date(YY.MM.DD or DD.MM.YY)";	// Date(YY.MM.DD or DD.MM.YY)
case 0x100000904ff: return "Date and Time(YYMMDDhhmmss)";	// Date and Time(YYMMDDhhmmss)
case 0x10000090b00: return "readout time in UTC";	// readout time in UTC
case 0x100010102ff: return "Battery voltage";	// Battery voltage
case 0x100010106ff: return "Successful readouts since restart";	// Successful readouts since restart
case 0x100010109ff: return "Temperature";	// Temperature
case 0x100010200ff: return "Positive active cumulative maximum demand(A+) total";	// Positive active cumulative maximum demand(A+) total
case 0x100010400ff: return "Positive active demand in a current demand period(A+)";	// Positive active demand in a current demand period(A+)
case 0x100010500ff: return "Positive active demand in the last completed demand period(A+)";	// Positive active demand in the last completed demand period(A+)
case 0x100010600ff: return "Positive active maximum demand(A+) total";	// Positive active maximum demand(A+) total
case 0x100010601ff: return "Positive active maximum demand(A+) in tariff 1";	// Positive active maximum demand(A+) in tariff 1
case 0x100010602ff: return "Positive active maximum demand(A+) in tariff 2";	// Positive active maximum demand(A+) in tariff 2
case 0x100010603ff: return "Positive active maximum demand(A+) in tariff 3";	// Positive active maximum demand(A+) in tariff 3
case 0x100010604ff: return "Positive active maximum demand(A+) in tariff 4";	// Positive active maximum demand(A+) in tariff 4
case 0x100010700ff: return "Positive active instantaneous power(A+)";	// Positive active instantaneous power(A+)
case 0x10001080060: return "Consumption over the last 24 hours";	// Consumption over the last 24 hours
case 0x10001080061: return "Consumption over the last 7 days";	// Consumption over the last 7 days
case 0x10001080062: return "Consumption over the last 30 days";	// Consumption over the last 30 days
case 0x10001080063: return "Consumption over the last 365 days";	// Consumption over the last 365 days
case 0x10001080064: return "Consumption since last reset";	// Consumption since last reset
case 0x100010800ff: return "Positive active energy(A+), current value";	// Positive active energy(A+), current value
case 0x100010801ff: return "Positive active energy(A+) in tariff T1";	// Positive active energy(A+) in tariff T1
case 0x100010802ff: return "Positive active energy(A+) in tariff T2";	// Positive active energy(A+) in tariff T2
case 0x100010803ff: return "Positive active energy(A+) in tariff T3";	// Positive active energy(A+) in tariff T3
case 0x100010804ff: return "Positive active energy(A+) in tariff T4";	// Positive active energy(A+) in tariff T4
case 0x100020200ff: return "Negative active cumulative maximum demand(A-) total";	// Negative active cumulative maximum demand(A-) total
case 0x100020400ff: return "Negative active demand in a current demand period(A-)";	// Negative active demand in a current demand period(A-)
case 0x100020500ff: return "Negative active demand in the last completed demand period(A-)";	// Negative active demand in the last completed demand period(A-)
case 0x100020600ff: return "Negative active maximum demand(A-) total";	// Negative active maximum demand(A-) total
case 0x100020601ff: return "Negative active maximum demand(A-) in tariff 1";	// Negative active maximum demand(A-) in tariff 1
case 0x100020602ff: return "Negative active maximum demand(A-) in tariff 2";	// Negative active maximum demand(A-) in tariff 2
case 0x100020603ff: return "Negative active maximum demand(A-) in tariff 3";	// Negative active maximum demand(A-) in tariff 3
case 0x100020604ff: return "Negative active maximum demand(A-) in tariff 4";	// Negative active maximum demand(A-) in tariff 4
case 0x100020700ff: return "Negative active instantaneous power(A-)";	// Negative active instantaneous power(A-)
case 0x100020800ff: return "Negative active energy(A+) total";	// Negative active energy(A+) total
case 0x100020801ff: return "Negative active energy(A+) in tariff T1";	// Negative active energy(A+) in tariff T1
case 0x100020802ff: return "Negative active energy(A+) in tariff T2";	// Negative active energy(A+) in tariff T2
case 0x100020803ff: return "Negative active energy(A+) in tariff T3";	// Negative active energy(A+) in tariff T3
case 0x100020804ff: return "Negative active energy(A+) in tariff T4";	// Negative active energy(A+) in tariff T4
case 0x100030200ff: return "Positive reactive cumulative maximum demand(Q+) total";	// Positive reactive cumulative maximum demand(Q+) total
case 0x100030400ff: return "Positive reactive demand in a current demand period(Q+)";	// Positive reactive demand in a current demand period(Q+)
case 0x100030500ff: return "Positive reactive demand in the last completed demand period(Q+)";	// Positive reactive demand in the last completed demand period(Q+)
case 0x100030600ff: return "Positive reactive maximum demand(Q+) total";	// Positive reactive maximum demand(Q+) total
case 0x100030700ff: return "Positive reactive instantaneous power(Q+)";	// Positive reactive instantaneous power(Q+)
case 0x100030800ff: return "Positive reactive energy(Q+) total";	// Positive reactive energy(Q+) total
case 0x100040200ff: return "Negative reactive cumulative maximum demand(Q-) total";	// Negative reactive cumulative maximum demand(Q-) total
case 0x100040400ff: return "Negative reactive demand in a current demand period(Q-)";	// Negative reactive demand in a current demand period(Q-)
case 0x100040500ff: return "Negative reactive demand in the last completed demand period(Q-)";	// Negative reactive demand in the last completed demand period(Q-)
case 0x100040600ff: return "Negative reactive maximum demand(Q-) total";	// Negative reactive maximum demand(Q-) total
case 0x100040700ff: return "Negative reactive instantaneous power(Q-)";	// Negative reactive instantaneous power(Q-)
case 0x100040800ff: return "Negative reactive energy(Q-) total";	// Negative reactive energy(Q-) total
case 0x100050200ff: return "Reactive cumulative maximum demand in Q1(Q1) total";	// Reactive cumulative maximum demand in Q1(Q1) total
case 0x100050400ff: return "Reactive demand in a current demand period in Q1(Q1)";	// Reactive demand in a current demand period in Q1(Q1)
case 0x100050500ff: return "Reactive demand in the last completed demand period in Q1(Q1)";	// Reactive demand in the last completed demand period in Q1(Q1)
case 0x100050600ff: return "Reactive maximum demand in Q1(Q1) total";	// Reactive maximum demand in Q1(Q1) total
case 0x100050800ff: return "Imported inductive reactive energy in 1 - st quadrant(Q1) total";	// Imported inductive reactive energy in 1 - st quadrant(Q1) total
case 0x100060200ff: return "Reactive cumulative maximum demand in Q2(Q2) total";	// Reactive cumulative maximum demand in Q2(Q2) total
case 0x100060400ff: return "Reactive demand in a current demand period in Q2(Q2)";	// Reactive demand in a current demand period in Q2(Q2)
case 0x100060500ff: return "Reactive demand in the last completed demand period in Q2(Q2)";	// Reactive demand in the last completed demand period in Q2(Q2)
case 0x100060600ff: return "Reactive maximum demand in Q2(Q2) total";	// Reactive maximum demand in Q2(Q2) total
case 0x100060800ff: return "Imported capacitive reactive energy in 2 - nd quadrant(Q2) total";	// Imported capacitive reactive energy in 2 - nd quadrant(Q2) total
case 0x100070200ff: return "Reactive cumulative maximum demand in Q3(Q3) total";	// Reactive cumulative maximum demand in Q3(Q3) total
case 0x100070400ff: return "Reactive demand in a current demand period in Q3(Q3)";	// Reactive demand in a current demand period in Q3(Q3)
case 0x100070500ff: return "Reactive demand in the last completed demand period in Q3(Q3)";	// Reactive demand in the last completed demand period in Q3(Q3)
case 0x100070600ff: return "Reactive maximum demand in Q3(Q3) total";	// Reactive maximum demand in Q3(Q3) total
case 0x100070800ff: return "Exported inductive reactive energy in 3 - rd quadrant(Q3) total";	// Exported inductive reactive energy in 3 - rd quadrant(Q3) total
case 0x100080200ff: return "Reactive cumulative maximum demand in Q4(Q4) total";	// Reactive cumulative maximum demand in Q4(Q4) total
case 0x100080400ff: return "Reactive demand in a current demand period in Q4(Q4)";	// Reactive demand in a current demand period in Q4(Q4)
case 0x100080500ff: return "Reactive demand in the last completed demand period in Q4(Q4)";	// Reactive demand in the last completed demand period in Q4(Q4)
case 0x100080600ff: return "Reactive maximum demand in Q4(Q4) total";	// Reactive maximum demand in Q4(Q4) total
case 0x100080800ff: return "Exported capacitive reactive energy in 4 - th quadrant(Q4) total";	// Exported capacitive reactive energy in 4 - th quadrant(Q4) total
case 0x100090200ff: return "Apparent cumulative maximum demand(S+) total";	// Apparent cumulative maximum demand(S+) total
case 0x100090400ff: return "Apparent demand in a current demand period(S+)";	// Apparent demand in a current demand period(S+)
case 0x100090500ff: return "Apparent demand in the last completed demand period(S+)";	// Apparent demand in the last completed demand period(S+)
case 0x100090600ff: return "Apparent maximum demand(S+) total";	// Apparent maximum demand(S+) total
case 0x100090601ff: return "Apparent maximum demand(S+) in tariff 1";	// Apparent maximum demand(S+) in tariff 1
case 0x100090602ff: return "Apparent maximum demand(S+) in tariff 2";	// Apparent maximum demand(S+) in tariff 2
case 0x100090603ff: return "Apparent maximum demand(S+) in tariff 3";	// Apparent maximum demand(S+) in tariff 3
case 0x100090604ff: return "Apparent maximum demand(S+) in tariff 4";	// Apparent maximum demand(S+) in tariff 4
case 0x100090700ff: return "Apparent instantaneous power(S+)";	// Apparent instantaneous power(S+)
case 0x100090800ff: return "Apparent energy(S+) total";	// Apparent energy(S+) total
case 0x1000a0601ff: return "Instantaneous apparent power average (S+) in tariff 1";	// Instantaneous apparent power average (S+) in tariff 1
case 0x1000a0602ff: return "Instantaneous apparent power average (S+) in tariff 2";	// Instantaneous apparent power average (S+) in tariff 2
case 0x1000a0603ff: return "Instantaneous apparent power average (S+) in tariff 3";	// Instantaneous apparent power average (S+) in tariff 3
case 0x1000a0604ff: return "Instantaneous apparent power average (S+) in tariff 4";	// Instantaneous apparent power average (S+) in tariff 4
case 0x1000a0800ff: return "Apparent energy total - Delivery (10.8.0)";	// Apparent energy total - Delivery (10.8.0)
case 0x1000a0801ff: return "Apparent energy tariff 1 - Delivery (10.8.1)";	// Apparent energy tariff 1 - Delivery (10.8.1)
case 0x1000a0802ff: return "Apparent energy tariff 2 - Delivery (10.8.2)";	// Apparent energy tariff 2 - Delivery (10.8.2)
case 0x1000a0803ff: return "Apparent energy tariff 3 - Delivery (10.8.3)";	// Apparent energy tariff 3 - Delivery (10.8.3)
case 0x1000a0804ff: return "Apparent energy tariff 4 - Delivery (10.8.4)";	// Apparent energy tariff 4 - Delivery (10.8.4)
case 0x1000b0600ff: return "Maximum current(I max)";	// Maximum current(I max)
case 0x1000b0700ff: return "Instantaneous current(I)";	// Instantaneous current(I)
case 0x1000c0700ff: return "Instantaneous voltage(U)";	// Instantaneous voltage(U)
case 0x1000d0700ff: return "Instantaneous power factor";	// Instantaneous power factor
case 0x1000e0700ff: return "Frequency";	// Frequency
case 0x1000f0200ff: return "Absolute active cumulative maximum demand(|A| ) total";	// Absolute active cumulative maximum demand(|A| ) total
case 0x1000f0400ff: return "Absolute active demand in a current demand period(|A| )";	// Absolute active demand in a current demand period(|A| )
case 0x1000f0500ff: return "Absolute active demand in the last completed demand period(|A| )";	// Absolute active demand in the last completed demand period(|A| )
case 0x1000f0600ff: return "Absolute active maximum demand(|A| ) total";	// Absolute active maximum demand(|A| ) total
case 0x1000f0700ff: return "Absolute active instantaneous power(|A|)";	// Absolute active instantaneous power(|A|)
case 0x1000f0800ff: return "Absolute active energy(A+) total";	// Absolute active energy(A+) total
case 0x100100700ff: return "Sum active instantaneous power(A+ - A-)";	// Sum active instantaneous power(A+ - A-)
case 0x100100800ff: return "Sum active energy without reverse blockade(A + -A - ) total";	// Sum active energy without reverse blockade(A + -A - ) total
case 0x100150700ff: return "Positive active instantaneous power(A+) in phase L1";	// Positive active instantaneous power(A+) in phase L1
case 0x100150800ff: return "Positive active energy(A+) in phase L1 total";	// Positive active energy(A+) in phase L1 total
case 0x100160700ff: return "Negative active instantaneous power(A-) in phase L1";	// Negative active instantaneous power(A-) in phase L1
case 0x100160800ff: return "Negative active energy(A-) in phase L1 total";	// Negative active energy(A-) in phase L1 total
case 0x100170700ff: return "Positive reactive instantaneous power(Q+) in phase L1";	// Positive reactive instantaneous power(Q+) in phase L1
case 0x100180700ff: return "Negative reactive instantaneous power(Q-) in phase L1";	// Negative reactive instantaneous power(Q-) in phase L1
case 0x1001d0700ff: return "Apparent instantaneous power(S+) in phase L1";	// Apparent instantaneous power(S+) in phase L1
case 0x1001f0600ff: return "Maximum current(I max) in phase L1";	// Maximum current(I max) in phase L1
case 0x1001f0700ff: return "Instantaneous current(I) in phase L1";	// Instantaneous current(I) in phase L1
case 0x100200700ff: return "Instantaneous voltage(U) in phase L1";	// Instantaneous voltage(U) in phase L1
case 0x100210700ff: return "Instantaneous power factor in phase L1";	// Instantaneous power factor in phase L1
case 0x100230700ff: return "Absolute active instantaneous power(|A|) in phase L1";	// Absolute active instantaneous power(|A|) in phase L1
case 0x100230800ff: return "Absolute active energy(|A|) in phase L1 total";	// Absolute active energy(|A|) in phase L1 total
case 0x100240700ff: return "Sum active instantaneous power(A + - A-) in phase L1";	// Sum active instantaneous power(A + - A-) in phase L1
case 0x100290700ff: return "Positive active instantaneous power(A+) in phase L2";	// Positive active instantaneous power(A+) in phase L2
case 0x100290800ff: return "Positive active energy(A+) in phase L2 total";	// Positive active energy(A+) in phase L2 total
case 0x1002a0700ff: return "Negative active instantaneous power(A-) in phase L2";	// Negative active instantaneous power(A-) in phase L2
case 0x1002a0800ff: return "Negative active energy(A-) in phase L2 total";	// Negative active energy(A-) in phase L2 total
case 0x1002b0700ff: return "Positive reactive instantaneous power(Q+) in phase L2";	// Positive reactive instantaneous power(Q+) in phase L2
case 0x1002c0700ff: return "Negative reactive instantaneous power(Q-) in phase L2";	// Negative reactive instantaneous power(Q-) in phase L2
case 0x100310700ff: return "Apparent instantaneous power(S+) in phase L2";	// Apparent instantaneous power(S+) in phase L2
case 0x100330600ff: return "Maximum current(I max) in phase L2";	// Maximum current(I max) in phase L2
case 0x100330700ff: return "Instantaneous current(I) in phase L2";	// Instantaneous current(I) in phase L2
case 0x100340700ff: return "Instantaneous voltage(U) in phase L2";	// Instantaneous voltage(U) in phase L2
case 0x100350700ff: return "Instantaneous power factor in phase L2";	// Instantaneous power factor in phase L2
case 0x100370700ff: return "Absolute active instantaneous power(|A|) in phase L2";	// Absolute active instantaneous power(|A|) in phase L2
case 0x100370800ff: return "Absolute active energy(|A|) in phase L2 total";	// Absolute active energy(|A|) in phase L2 total
case 0x100380700ff: return "Sum active instantaneous power(A + - A-) in phase L2";	// Sum active instantaneous power(A + - A-) in phase L2
case 0x1003d0700ff: return "Positive active instantaneous power(A+) in phase L3";	// Positive active instantaneous power(A+) in phase L3
case 0x1003d0800ff: return "Positive active energy(A+) in phase L3 total";	// Positive active energy(A+) in phase L3 total
case 0x1003d6100ff: return "Fatal error meter status";	// Fatal error meter status
case 0x1003e0700ff: return "Negative active instantaneous power(A-) in phase L3";	// Negative active instantaneous power(A-) in phase L3
case 0x1003e0800ff: return "Negative active energy(A-) in phase L3 total";	// Negative active energy(A-) in phase L3 total
case 0x1003f0700ff: return "Positive reactive instantaneous power(Q+) in phase L3";	// Positive reactive instantaneous power(Q+) in phase L3
case 0x100400700ff: return "Negative reactive instantaneous power(Q-) in phase L3";	// Negative reactive instantaneous power(Q-) in phase L3
case 0x100450700ff: return "Apparent instantaneous power(S+) in phase L3";	// Apparent instantaneous power(S+) in phase L3
case 0x100470600ff: return "Maximum current(I max) in phase L3";	// Maximum current(I max) in phase L3
case 0x100470700ff: return "Instantaneous current(I) in phase L3";	// Instantaneous current(I) in phase L3
case 0x100480700ff: return "Instantaneous voltage(U) in phase L3";	// Instantaneous voltage(U) in phase L3
case 0x100490700ff: return "Instantaneous power factor in phase L3";	// Instantaneous power factor in phase L3
case 0x1004b0700ff: return "Absolute active instantaneous power(|A|) in phase L3";	// Absolute active instantaneous power(|A|) in phase L3
case 0x1004b0800ff: return "Absolute active energy(|A|) in phase L3 total";	// Absolute active energy(|A|) in phase L3 total
case 0x1004c0700ff: return "Sum active instantaneous power(A + - A-) in phase L3";	// Sum active instantaneous power(A + - A-) in phase L3
case 0x1005b0600ff: return "Maximum current(I max) in neutral";	// Maximum current(I max) in neutral
case 0x1005b0700ff: return "Instantaneous current(I) in neutral";	// Instantaneous current(I) in neutral
case 0x100600200ff: return "Event parameters change";	// Event parameters change
case 0x100600201ff: return "Event parameters change";	// Event parameters change
case 0x100600700ff: return "Event power down";	// Event power down
case 0x10060070aff: return "Event power down";	// Event power down
case 0x100603301ff: return "Event terminal cover opened";	// Event terminal cover opened
case 0x100603302ff: return "Event terminal cover opened";	// Event terminal cover opened
case 0x100603303ff: return "Event main cover opened - counter";	// Event main cover opened - counter
case 0x100603304ff: return "Event main cover opened ";	// Event main cover opened 
case 0x100603305ff: return "Event magnetic field detection start - counter";	// Event magnetic field detection start - counter
case 0x100603306ff: return "Event magnetic field detection start";	// Event magnetic field detection start
case 0x100603307ff: return "Event reverse power flow";	// Event reverse power flow
case 0x100603308ff: return "Event reverse power flow";	// Event reverse power flow
case 0x10060330dff: return "Event power up";	// Event power up
case 0x10060330eff: return "Event power up";	// Event power up
case 0x10060330fff: return "Event RTC(Real Time Clock) set";	// Event RTC(Real Time Clock) set
case 0x100603310ff: return "Event RTC(Real Time Clock) set";	// Event RTC(Real Time Clock) set
case 0x100603315ff: return "Event terminal cover closed";	// Event terminal cover closed
case 0x100603316ff: return "Event terminal cover closed";	// Event terminal cover closed
case 0x100603317ff: return "Event main cover closed";	// Event main cover closed
case 0x100603318ff: return "Event main cover closed";	// Event main cover closed
case 0x100603319ff: return "Event log - book 1 erased";	// Event log - book 1 erased
case 0x10060331aff: return "Event log - book 1 erased";	// Event log - book 1 erased
case 0x10060331bff: return "Event fraud start";	// Event fraud start
case 0x10060331cff: return "Event fraud start";	// Event fraud start
case 0x10060331dff: return "Event fraud stop";	// Event fraud stop
case 0x10060331eff: return "Event fraud stop";	// Event fraud stop
case 0x100603501ff: return "Tamper 1 energy";	// Tamper 1 energy
case 0x100603502ff: return "Tamper 2 energy";	// Tamper 2 energy
case 0x100603503ff: return "Tamper 3 energy";	// Tamper 3 energy
case 0x100603504ff: return "Tamper 4 energy";	// Tamper 4 energy
case 0x100603505ff: return "Tamper 1 time counter register";	// Tamper 1 time counter register
case 0x100603506ff: return "Tamper 2 time counter register";	// Tamper 2 time counter register
case 0x100603507ff: return "Tamper 3 time counter register";	// Tamper 3 time counter register
case 0x100603509ff: return "Tamper 4 time counter register";	// Tamper 4 time counter register
case 0x10060350aff: return "Tamper 5 time counter register";	// Tamper 5 time counter register
case 0x10060350bff: return "Tamper 5 energy";	// Tamper 5 energy
case 0x100605700ff: return "Active tariff";	// Active tariff
case 0x100606009ff: return "Fraud flag";	// Fraud flag
case 0x101000402ff: return "Current transformer ratio (L&G)";	// Current transformer ratio (L&G)
case 0x101000403ff: return "Voltage transformer ratio (L&G)";	// Voltage transformer ratio (L&G)
case 0x101011d00ff: return "Load profile (+A) Active energy import";	// Load profile (+A) Active energy import
case 0x101021d00ff: return "Load profile (-A) Active energy export";	// Load profile (-A) Active energy export
case 0x101051d00ff: return "Load profile Reactive Q1";	// Load profile Reactive Q1
case 0x101061d00ff: return "Load profile Reactive Q2";	// Load profile Reactive Q2
case 0x101071d00ff: return "Load profile Reactive Q3";	// Load profile Reactive Q3
case 0x101081d00ff: return "Load profile Reactive Q4";	// Load profile Reactive Q4
case 0x101621700ff: return "Power threshold (D.23.0)";	// Power threshold (D.23.0)
case 0x40000010aff: return "Local date at set date";	// Local date at set date
case 0x400000901ff: return "Current time at time of transmission";	// Current time at time of transmission
case 0x400000902ff: return "Current date at time of transmission";	// Current date at time of transmission
case 0x400010001ff: return "Unrated integral, current value";	// Unrated integral, current value
case 0x400010200ff: return "Unrated integral, set date value";	// Unrated integral, set date value
case 0x501011d00ff: return "Load profile Cold";	// Load profile Cold
case 0x600000800ff: return "Power (energy flow) (P ), average, current value";	// Power (energy flow) (P ), average, current value
case 0x600010000ff: return "Energy (A), total, current value";	// Energy (A), total, current value
case 0x600010200ff: return "Energy (A), total, set date value";	// Energy (A), total, set date value
case 0x601011d00ff: return "Load profile Heat";	// Load profile Heat
case 0x6011f1d00ff: return "Load profile Heat (+E) Energy output";	// Load profile Heat (+E) Energy output
case 0x70000081cff: return "Averaging duration for actual flow rate value";	// Averaging duration for actual flow rate value
case 0x700002a02ff: return "defined Pressure, absolute, at base conditions";	// defined Pressure, absolute, at base conditions
case 0x700030000ff: return "Volume (meter), ";	// Volume (meter), 
case 0x700030100ff: return "Volume (meter), temperature converted (Vtc), forward, absolute, current value";	// Volume (meter), temperature converted (Vtc), forward, absolute, current value
case 0x700030200ff: return "Volume (meter), base conditions (Vb), forward, absolute, current value";	// Volume (meter), base conditions (Vb), forward, absolute, current value
case 0x7002b0f00ff: return "Flow rate at measuring conditions, averaging period 1 (default period = 5 min), current interval ";	// Flow rate at measuring conditions, averaging period 1 (default period = 5 min), current interval 
case 0x7002b1000ff: return "Flow rate, temperature converted, averaging period 1(default period = 5 min), current interval";	// Flow rate, temperature converted, averaging period 1(default period = 5 min), current interval
case 0x7002b1100ff: return "Flow rate at base conditions, averaging period 1 (default period = 5 min), current interval";	// Flow rate at base conditions, averaging period 1 (default period = 5 min), current interval
case 0x7010b1d00ff: return "Load profile (+Vb) Operating volume Uninterrupted delivery";	// Load profile (+Vb) Operating volume Uninterrupted delivery
case 0x800010000ff: return "Volume (V), accumulated, total, current value";	// Volume (V), accumulated, total, current value
case 0x800010200ff: return "Volume (V), accumulated, total, due date value";	// Volume (V), accumulated, total, due date value
case 0x800020000ff: return "Flow rate, average (Va/t), current value ";	// Flow rate, average (Va/t), current value 
case 0x801011d00ff: return "Load profile (+Vol) Volume output";	// Load profile (+Vol) Volume output
case 0x901011d00ff: return "Load profile Hot Water";	// Load profile Hot Water
case 0x810000090b00: return "actSensorTime - current UTC time";	// actSensorTime - current UTC time
case 0x810000090b01: return "offset to actual time zone in minutes (-720 .. +720)";	// offset to actual time zone in minutes (-720 .. +720)
case 0x810060050000: return "";
case 0x8101000000ff: return "";
case 0x810100000100: return "interface name";	// interface name
case 0x8102000000ff: return "";
case 0x810200000100: return "interface name";	// interface name
case 0x8102000700ff: return "see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle";	// see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle
case 0x8102000701ff: return "0 == manual, 1 == DHCP";	// 0 == manual, 1 == DHCP
case 0x810200070201: return "";
case 0x810200070202: return "";
case 0x810200070203: return "";
case 0x810200070204: return "";
case 0x810200070205: return "";
case 0x8102000702ff: return "if true use a DHCP server";	// if true use a DHCP server
case 0x8102000710ff: return "see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle ";	// see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle 
case 0x810217070000: return "current IP address (customer interface)";	// current IP address (customer interface)
case 0x810217070001: return "first manual set IP address";	// first manual set IP address
case 0x810217070002: return "second manual set IP address";	// second manual set IP address
case 0x810217070101: return "";
case 0x810217070102: return "";
case 0x8103000000ff: return "";
case 0x810300000100: return "interface name";	// interface name
case 0x8104000000ff: return "WAN interface";	// WAN interface
case 0x810400000100: return "interface name";	// interface name
case 0x8104000600ff: return "see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status ";	// see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status 
case 0x8104000700ff: return "see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter ";	// see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter 
case 0x8104020700ff: return "see: Datenstruktur zum Lesen/Setzen der GSM Parameter ";	// see: Datenstruktur zum Lesen/Setzen der GSM Parameter 
case 0x81040d060000: return "	aktueller Provider-Identifier(uint32)";	// 	aktueller Provider-Identifier(uint32)
case 0x81040d0600ff: return "";
case 0x81040d0700ff: return "see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter ";	// see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter 
case 0x81040d0800ff: return "";
case 0x81040e0600ff: return "";
case 0x810417070000: return "aktueller Location - oder Areacode";	// aktueller Location - oder Areacode
case 0x8104180700ff: return "";
case 0x81041a070000: return "aktuelle Zelleninformation";	// aktuelle Zelleninformation
case 0x81042b070000: return "";
case 0x8104623c0101: return "PPPoE username";	// PPPoE username
case 0x8104623c0201: return "PPPoE passphrase";	// PPPoE passphrase
case 0x8104623c0301: return "PPPoE mode";	// PPPoE mode
case 0x8105000000ff: return "";
case 0x81050d070001: return "always 1";	// always 1
case 0x81050d070002: return "0 = auto, 6 = 9600, 10 = 115200 baud";	// 0 = auto, 6 = 9600, 10 = 115200 baud
case 0x81050d0700ff: return "M-Bus EDL (RJ10)";	// M-Bus EDL (RJ10)
case 0x8106000000ff: return "";
case 0x810600000100: return "";
case 0x810600000300: return "";
case 0x810600020000: return "";
case 0x8106000203ff: return "";
case 0x8106000204ff: return "dbm";	// dbm
case 0x8106000374ff: return "Time since last radio telegram reception";	// Time since last radio telegram reception
case 0x81060f0600ff: return "see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status ";	// see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
case 0x8106190700ff: return "Wireless M-BUS";	// Wireless M-BUS
case 0x8106190701ff: return "";
case 0x8106190702ff: return "";
case 0x8106190703ff: return "";
case 0x8106190704ff: return "0 = default, 1 = low, 2 = medium, 3 = high";	// 0 = default, 1 = low, 2 = medium, 3 = high
case 0x8106190711ff: return "";
case 0x810627320301: return "";
case 0x81062b070000: return "Receive field strength";	// Receive field strength
case 0x8106643c0101: return "";
case 0x8106643c0104: return "max timeout between SML close request and SML open response from device to gateway";	// max timeout between SML close request and SML open response from device to gateway
case 0x8106643c0105: return "max timeout between SML close request and SML open response from gateway to device";	// max timeout between SML close request and SML open response from gateway to device
case 0x8141000000ff: return "";
case 0x814200000001: return "";
case 0x814200000002: return "";
case 0x814200000003: return "";
case 0x814200000004: return "";
case 0x814200000005: return "";
case 0x814200000006: return "";
case 0x8145000000ff: return "";
case 0x8146000000ff: return "";
case 0x8146000002ff: return "";
case 0x8147000000ff: return "";
case 0x8147170700ff: return "push target name";	// push target name
case 0x814800000000: return "";
case 0x814800000001: return "";
case 0x814800000002: return "";
case 0x814800000003: return "";
case 0x814800320101: return "";
case 0x814800320201: return "";
case 0x814800320301: return "true = DLS, false = LAN";	// true = DLS, false = LAN
case 0x81480d0600ff: return "see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter";	// see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter
case 0x814817070000: return "IPv4 or IPv6 address";	// IPv4 or IPv6 address
case 0x814817070001: return "IPv4 or IPv6 address";	// IPv4 or IPv6 address
case 0x8148170700ff: return "see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter";	// see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter
case 0x814817070100: return "";
case 0x814817070200: return "";
case 0x814817070400: return "";
case 0x814817070401: return "set/get OBIS_IF_LAN_DSL";	// set/get OBIS_IF_LAN_DSL
case 0x814817070500: return "";
case 0x814817070501: return "set/get OBIS_IF_LAN_DSL";	// set/get OBIS_IF_LAN_DSL
case 0x814817070600: return "";
case 0x814817070601: return "set/get OBIS_IF_LAN_DSL";	// set/get OBIS_IF_LAN_DSL
case 0x814827320601: return "";
case 0x814831320201: return "";
case 0x814831320701: return "reply to received ICMP packages";	// reply to received ICMP packages
case 0x814900000001: return "";
case 0x814900000002: return "";
case 0x8149000010ff: return "options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX";	// options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX
case 0x81490d0600ff: return "see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status ";	// see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status 
case 0x81490d0700ff: return "see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter ";	// see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter 
case 0x814917070000: return "ip address of IP-T master ";	// ip address of IP-T master 
case 0x814919070000: return "target port of IP-T master ";	// target port of IP-T master 
case 0x81491a070000: return "source port of IP-T master ";	// source port of IP-T master 
case 0x8149633c0100: return "";
case 0x8149633c0200: return "";
case 0x8149633c0300: return "";
case 0x814a000000ff: return "";
case 0x818100000001: return "peer address: OBISLOG";	// peer address: OBISLOG
case 0x818100000002: return "peer address: SCM (power)";	// peer address: SCM (power)
case 0x818100000010: return "peer address: EHZIF";	// peer address: EHZIF
case 0x818100000011: return "peer address: USERIF";	// peer address: USERIF
case 0x818100000012: return "internal MUC software function";	// internal MUC software function
case 0x818100000013: return "peer address: WAN/GSM";	// peer address: WAN/GSM
case 0x818100000014: return "internal MUC software function";	// internal MUC software function
case 0x8181000000ff: return "unit is 255";	// unit is 255
case 0x818100020000: return "(0.2.0) firmware revision";	// (0.2.0) firmware revision
case 0x818100020001: return "";
case 0x818100020002: return "";
case 0x818100020003: return "Anzahl aller Nachrichten zur Übertragung des Binary";	// Anzahl aller Nachrichten zur Übertragung des Binary
case 0x818100020004: return "Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary";	// Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary
case 0x818100020005: return "";
case 0x8181000202ff: return "";
case 0x8181000203ff: return "";
case 0x818100030001: return "";
case 0x818110000001: return "";
case 0x818110000002: return "internal software function";	// internal software function
case 0x818110000003: return "";
case 0x818110000004: return "";
case 0x81811000000c: return "";
case 0x818110000014: return "";
case 0x8181100601ff: return "1. Liste der sichtbaren Sensoren";	// 1. Liste der sichtbaren Sensoren
case 0x8181100602ff: return "2. Liste der sichtbaren Sensoren";	// 2. Liste der sichtbaren Sensoren
case 0x81811006ffff: return "visible devices (Liste der sichtbaren Sensoren)";	// visible devices (Liste der sichtbaren Sensoren)
case 0x81811016ffff: return "new active devices";	// new active devices
case 0x81811026ffff: return "not longer visible devices";	// not longer visible devices
case 0x8181110601ff: return "1. Liste der aktiven Sensoren)";	// 1. Liste der aktiven Sensoren)
case 0x8181110602ff: return "2. Liste der aktiven Sensoren)";	// 2. Liste der aktiven Sensoren)
case 0x81811106fbff: return "activate meter/sensor";	// activate meter/sensor
case 0x81811106fcff: return "deactivate meter/sensor";	// deactivate meter/sensor
case 0x81811106fdff: return "delete meter/sensor";	// delete meter/sensor
case 0x81811106ffff: return "active devices (Liste der aktiven Sensoren)";	// active devices (Liste der aktiven Sensoren)
case 0x81811206ffff: return "extended device information";	// extended device information
case 0x818127320701: return " Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook";	//  Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook
case 0x8181613c01ff: return "";
case 0x8181613c02ff: return "";
case 0x81818160ffff: return "see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte ";	// see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte 
case 0x81818161ffff: return "user name for access";	// user name for access
case 0x81818162ffff: return "SHA256 encrypted";	// SHA256 encrypted
case 0x81818163ffff: return "";
case 0x8181c78101ff: return "7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) ";	// 7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) 
case 0x8181c7810cff: return "";
case 0x8181c7810dff: return "";
case 0x8181c7810eff: return "";
case 0x8181c7810fff: return "";
case 0x8181c78110ff: return "";
case 0x8181c78123ff: return "";
case 0x8181c78201ff: return "see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) ";	// see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) 
case 0x8181c78202ff: return "Geräteklasse (OBIS code or '2D 2D 2D')";	// Geräteklasse (OBIS code or '2D 2D 2D')
case 0x8181c78203ff: return "FLAG";	// FLAG
case 0x8181c78204ff: return "Server ID ";	// Server ID 
case 0x8181c78205ff: return "";
case 0x8181c78206ff: return "Firmware";	// Firmware
case 0x8181c78208ff: return "";
case 0x8181c78209ff: return "hardware equipment (charge, type, ...) 81 81 C7 82 0A NN";	// hardware equipment (charge, type, ...) 81 81 C7 82 0A NN
case 0x8181c7820a01: return "model code (VMET-1KW-221-1F0)";	// model code (VMET-1KW-221-1F0)
case 0x8181c7820a02: return "serial number (3894517)";	// serial number (3894517)
case 0x8181c7820eff: return "";
case 0x8181c78241ff: return "3 x 230 /400 V and 5 (100) A ";	// 3 x 230 /400 V and 5 (100) A 
case 0x8181c78242ff: return "3 x 230 /400 V and 1 (6) A";	// 3 x 230 /400 V and 1 (6) A
case 0x8181c78243ff: return "3 x  58 / 100 V and 1 (6) A ";	// 3 x  58 / 100 V and 1 (6) A 
case 0x8181c78245ff: return "IW module";	// IW module
case 0x8181c78246ff: return "PSTN/GPRS";	// PSTN/GPRS
case 0x8181c78247ff: return "GPRS/PLC";	// GPRS/PLC
case 0x8181c78248ff: return "KM module (LAN/DSL)";	// KM module (LAN/DSL)
case 0x8181c78249ff: return "NK/HS";	// NK/HS
case 0x8181c7824aff: return "external load profile collector ";	// external load profile collector 
case 0x8181c7824bff: return "gateway";	// gateway
case 0x8181c7824fff: return "see DEV_CLASS_MUC_LAN";	// see DEV_CLASS_MUC_LAN
case 0x8181c78250ff: return "";
case 0x8181c78252ff: return "";
case 0x8181c78253ff: return "(MUC-LAN/DSL)";	// (MUC-LAN/DSL)
case 0x8181c78281ff: return "";
case 0x8181c7838201: return "";
case 0x8181c7838205: return "";
case 0x8181c7838207: return "";
case 0x8181c783820e: return "";
case 0x8181c78600ff: return "data mirror root element (Eigenschaften eines Datenspiegels)";	// data mirror root element (Eigenschaften eines Datenspiegels)
case 0x8181c78601ff: return " Bitmask to define bits that will be transferred into log";	//  Bitmask to define bits that will be transferred into log
case 0x8181c78602ff: return "average time between two received data records (milliseconds)";	// average time between two received data records (milliseconds)
case 0x8181c78603ff: return "";
case 0x8181c78604ff: return "0 == UTC, 1 == UTC + time zone, 2 == local time";	// 0 == UTC, 1 == UTC + time zone, 2 == local time
case 0x8181c78610ff: return "1 minute";	// 1 minute
case 0x8181c78611ff: return "";
case 0x8181c78612ff: return "";
case 0x8181c78613ff: return "";
case 0x8181c78614ff: return "past two hours";	// past two hours
case 0x8181c78615ff: return "weekly (on day change from Sunday to Monday)";	// weekly (on day change from Sunday to Monday)
case 0x8181c78616ff: return "monthly recorded meter readings";	// monthly recorded meter readings
case 0x8181c78617ff: return "annually recorded meter readings";	// annually recorded meter readings
case 0x8181c78618ff: return "81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods";	// 81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods
case 0x8181c78620ff: return "data collector root element (Eigenschaften eines Datensammlers)";	// data collector root element (Eigenschaften eines Datensammlers)
case 0x8181c78621ff: return "";
case 0x8181c78622ff: return "max. table size";	// max. table size
case 0x8181c7862fff: return "Version identifier";	// Version identifier
case 0x8181c78630ff: return "Version identifier filter";	// Version identifier filter
case 0x8181c78631ff: return "Device class, on the basis of which associated sensors / actuators are identified";	// Device class, on the basis of which associated sensors / actuators are identified
case 0x8181c78632ff: return "Filter for the device class";	// Filter for the device class
case 0x8181c78781ff: return "register period in seconds (0 == event driven)";	// register period in seconds (0 == event driven)
case 0x8181c78801ff: return "";
case 0x8181c78802ff: return "List of NTP servers";	// List of NTP servers
case 0x8181c78803ff: return "NTP port (123)";	// NTP port (123)
case 0x8181c78804ff: return "timezone";	// timezone
case 0x8181c78805ff: return "Offset to transmission of the signal for synchronization";	// Offset to transmission of the signal for synchronization
case 0x8181c78806ff: return "NTP enabled/disables";	// NTP enabled/disables
case 0x8181c78810ff: return "device time";	// device time
case 0x8181c789e1ff: return "";
case 0x8181c789e2ff: return "event";	// event
case 0x8181c78a01ff: return "push root element";	// push root element
case 0x8181c78a02ff: return "in seconds";	// in seconds
case 0x8181c78a03ff: return "in seconds";	// in seconds
case 0x8181c78a04ff: return "options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST";	// options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST
case 0x8181c78a21ff: return "SML data response without request - typical IP - T push";	// SML data response without request - typical IP - T push
case 0x8181c78a22ff: return "SML data response without request - target is SML client address";	// SML data response without request - target is SML client address
case 0x8181c78a23ff: return "OBIS list (data mirror)";	// OBIS list (data mirror)
case 0x8181c78a42ff: return "new meter/sensor data";	// new meter/sensor data
case 0x8181c78a43ff: return "configuration changed";	// configuration changed
case 0x8181c78a44ff: return "list of visible meters changed";	// list of visible meters changed
case 0x8181c78a45ff: return "list of active meters changed";	// list of active meters changed
case 0x8181c78a81ff: return "";
case 0x8181c78a82ff: return "list of registers to be delivered by the push source";	// list of registers to be delivered by the push source
case 0x8181c78a83ff: return "encode profiles";	// encode profiles
case 0x8181c79000ff: return "";
case 0x8181c79300ff: return "";
case 0x8181c79301ff: return " if true 1107 interface active otherwise SML interface active";	//  if true 1107 interface active otherwise SML interface active
case 0x8181c79302ff: return "Loop timeout in seconds";	// Loop timeout in seconds
case 0x8181c79303ff: return "Retry count";	// Retry count
case 0x8181c79304ff: return "Minimal answer timeout(300)";	// Minimal answer timeout(300)
case 0x8181c79305ff: return " Maximal answer timeout(5000)";	//  Maximal answer timeout(5000)
case 0x8181c79306ff: return "Maximum data bytes(10240)";	// Maximum data bytes(10240)
case 0x8181c79307ff: return "if true RS 485, otherwise RS 323";	// if true RS 485, otherwise RS 323
case 0x8181c79308ff: return "Protocol mode(A ... D)";	// Protocol mode(A ... D)
case 0x8181c79309ff: return "Liste der abzufragenden 1107 Zähler";	// Liste der abzufragenden 1107 Zähler
case 0x8181c7930aff: return "";
case 0x8181c7930bff: return "";
case 0x8181c7930cff: return "";
case 0x8181c7930dff: return "";
case 0x8181c7930eff: return "";
case 0x8181c79310ff: return "";
case 0x8181c79311ff: return "time grid of load profile readout in seconds";	// time grid of load profile readout in seconds
case 0x8181c79313ff: return "time sync in seconds";	// time sync in seconds
case 0x8181c79314ff: return "seconds";	// seconds
case 0x8181c7c7fd00: return "no error";	// no error
case 0x8181c7c7fd01: return "attention: job is running";	// attention: job is running
case 0x8181c7c7fe00: return "attention: unknown error";	// attention: unknown error
case 0x8181c7c7fe01: return "attention: unknown SML ID";	// attention: unknown SML ID
case 0x8181c7c7fe02: return "attention: not authorized";	// attention: not authorized
case 0x8181c7c7fe03: return "attention: unable to find recipient for request";	// attention: unable to find recipient for request
case 0x8181c7c7fe04: return "attention: no request field";	// attention: no request field
case 0x8181c7c7fe05: return "attention: cannot write";	// attention: cannot write
case 0x8181c7c7fe06: return "attention: cannot read";	// attention: cannot read
case 0x8181c7c7fe07: return "attention: communication error";	// attention: communication error
case 0x8181c7c7fe08: return "attention: parser error";	// attention: parser error
case 0x8181c7c7fe09: return "attention: out of range";	// attention: out of range
case 0x8181c7c7fe0a: return "attention: not executed";	// attention: not executed
case 0x8181c7c7fe0b: return "attention: invalid CRC";	// attention: invalid CRC
case 0x8181c7c7fe0c: return "attention: no broadcast";	// attention: no broadcast
case 0x8181c7c7fe0d: return "attention: unexpected message";	// attention: unexpected message
case 0x8181c7c7fe0e: return "attention: unknown OBIS code";	// attention: unknown OBIS code
case 0x8181c7c7fe0f: return "attention: data type not supported";	// attention: data type not supported
case 0x8181c7c7fe10: return "attention: element is not optional";	// attention: element is not optional
case 0x8181c7c7fe11: return "attention: no entries";	// attention: no entries
case 0x8181c7c7fe12: return "attention: end limit before start";	// attention: end limit before start
case 0x8181c7c7fe13: return "attention: range is empty - not the profile";	// attention: range is empty - not the profile
case 0x8181c7c7fe14: return "attention: missing close message";	// attention: missing close message
case 0x9000000000ff: return "90 00 00 00 00 NN - broker list";	// 90 00 00 00 00 NN - broker list
case 0x9000000001ff: return "";
case 0x9000000002ff: return "ip address";	// ip address
case 0x9000000003ff: return "port";	// port
case 0x9000000004ff: return "";
case 0x9000000005ff: return "";
case 0x9000000006ff: return "";
case 0x90000000a0ff: return "";
case 0x9100000000ff: return "";
case 0x9100000001ff: return "example: /dev/ttyAPP0";	// example: /dev/ttyAPP0
case 0x9100000002ff: return "";
case 0x9100000003ff: return "";
case 0x9100000004ff: return "";
case 0x9100000005ff: return "";
case 0x9100000006ff: return "";
case 0x9100000007ff: return "LMN port task";	// LMN port task
case 0x9200000000ff: return "";
case 0x9200000001ff: return "";
case 0x9200000002ff: return "";
case 0x9200000003ff: return "";
case 0x9200000004ff: return "";
case 0x9200000005ff: return "";
case 0x9300000000ff: return "";
case 0x9300000001ff: return "";
case 0x9300000002ff: return "ip address";	// ip address
case 0x9300000003ff: return "port";	// port
case 0x9300000004ff: return "";
case 0x9300000005ff: return "";
case 0x990000000003: return "current data set";	// current data set
case 0x990000000004: return "";
case 0x990000000005: return "";
