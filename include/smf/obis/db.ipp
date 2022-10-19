case 0xff: return "METER_ADDRESS";	// device address - 0-0:0.0.0.255
case 0x1ff: return "IDENTITY_NR_1";	// 0-0:0.0.1.255
case 0x2ff: return "IDENTITY_NR_2";	// 0-0:0.0.2.255
case 0x3ff: return "IDENTITY_NR_3";	// 0-0:0.0.3.255
case 0x100ff: return "RESET_COUNTER";	// 0.1.0
case 0x200ff: return "FIRMWARE_VERSION";	// COSEM class id 1
case 0x10000ff: return "REAL_TIME_CLOCK";	// current time
case 0x600100ff: return "SERIAL_NR";	// (C.1.0) Serial number I (assigned by the manufacturer)
case 0x600101ff: return "SERIAL_NR_SECOND";	// Serial number II (costumer ID, assigned by the manufacturer).
case 0x600102ff: return "PARAMETERS_FILE_CODE";	// Parameters file code (C.1.2)
case 0x600103ff: return "PRODUCTION_DATE";	// date of manufacture (C.1.3)
case 0x600104ff: return "PARAMETERS_CHECK_SUM";	// Parameters check sum (C.1.4)
case 0x600105ff: return "FIRMWARE_BUILD_DATE";	// Firmware built date (C.1.5)
case 0x600106ff: return "FIRMWARE_CHECK_SUM";	// Firmware check sum (C.1.6)
case 0x6001ffff: return "FABRICATION_NR";	// fabrication number
case 0x600201ff: return "DATE_TIME_PARAMETERISATION";	// Date of last parameterisation (00-03-26)
case 0x600300ff: return "PULSE_CONST_ACTIVE";	// Active pulse constant (C.3.0)
case 0x600301ff: return "PULSE_CONST_REACTIVE";	// Reactive pulse constant (C.3.1)
case 0x600600ff: return "COUNTER_POWER_DOWN_TIME";	// Power down time counter (C.6.0)
case 0x600601ff: return "BATTERY_REMAINING_CAPACITY";	// Battery remaining capacity (C.6.1)
case 0x600602ff: return "BATTERY_LAST_CHANGE";	// Date of last battery change (C.6.2)
case 0x600603ff: return "BATTERY_VOLTAGE";	// Battery Voltage (C.6.3)
case 0x600700ff: return "POWER_OUTAGES";	// Number of power failures 
case 0x600800ff: return "SECONDS_INDEX";	// [SML_Time] seconds index
case 0x601000ff: return "LOGICAL_NAME";
case 0x604800ff: return "COUNTER_UNDER_VOLTAGE";	// Number of under-voltages
case 0x604900ff: return "COUNTER_OVER_VOLTAGE";	// Number of over-voltages
case 0x604a00ff: return "COUNTER_OVER_LOAD";	// Number of over-loads (over-current) 
case 0x60f00dff: return "HARDWARE_TYPE";	// name
case 0x616100ff: return "MBUS_STATE";	// Status according to EN13757-3 (error register)
case 0x616101ff: return "MBUS_STATE_1";	// not relevant under calibration law
case 0x616102ff: return "MBUS_STATE_2";	// not relevant under calibration law
case 0x616103ff: return "MBUS_STATE_3";	// not relevant under calibration law
case 0x1600205ff: return "LAST_CALIBRATION";	// Date and time of last calibration (L&G)
case 0x260f008ff: return "HARDWARE_ID_EXT";	// Hardware ID of extension board (L&G)
case 0x260f009ff: return "HARDWARE_REF_EXT";	// Reference hardware ID of extension board (L&G)
case 0x80800000ff: return "STORAGE_TIME_SHIFT";	// root push operations
case 0x8080000101: return "HAS_WAN";	// WAN on customer interface
case 0x8080000301: return "HAS_SSL_CONFIG";	// SSL/TSL configuration available
case 0x80800004ff: return "SSL_CERTIFICATES";	// list of SSL certificates
case 0x80800010ff: return "ROOT_MEMORY_USAGE";	// request memory usage
case 0x80800011ff: return "ROOT_MEMORY_MIRROR";
case 0x80800012ff: return "ROOT_MEMORY_TMP";
case 0x80800100ff: return "ROOT_SECURITY";	// list some basic security information
case 0x80800101ff: return "SECURITY_SERVER_ID";
case 0x80800102ff: return "SECURITY_OWNER";
case 0x80800105ff: return "SECURITY_05";	// 3
case 0x8080010610: return "SECURITY_06_10";	// 3
case 0x80800106ff: return "SECURITY_06";	// 3
case 0x80800107ff: return "SECURITY_07";	// 1
case 0x8080100001: return "CLASS_OP_LSM_STATUS";	// LSM status
case 0x80801100ff: return "ACTUATORS";	// list of actuators
case 0x80801110ff: return "CLASS_OP_LSM_ACTOR_ID";	// ServerID des Aktors (uint16)
case 0x8080111101: return "CLASS_OP_LSM_SWITCH";	// Schaltanforderung
case 0x80801111ff: return "CLASS_OP_LSM_CONNECT";	// Connection state
case 0x80801113ff: return "CLASS_OP_LSM_FEEDBACK";	// feedback configuration
case 0x80801114ff: return "CLASS_OP_LSM_LOAD";	// number of loads
case 0x80801115ff: return "CLASS_OP_LSM_POWER";	// total power
case 0x808011a0ff: return "CLASS_STATUS";	// see: 2.2.1.3 Status der Aktoren (Kontakte)
case 0x808011a1ff: return "CLASS_OP_LSM_VERSION";	// LSM version
case 0x808011a2ff: return "CLASS_OP_LSM_TYPE";	// LSM type
case 0x808011a901: return "CLASS_OP_LSM_ACTIVE_RULESET";	// active ruleset
case 0x808011a902: return "CLASS_OP_LSM_PASSIVE_RULESET";
case 0x80801403ff: return "CLASS_OP_LSM_JOB";	// Schaltauftrags ID ((octet string)	
case 0x80801420ff: return "CLASS_OP_LSM_POSITION";	// current position
case 0xb000020000: return "CLASS_MBUS";
case 0xb000020001: return "CLASS_MBUS_RO_INTERVAL";	// readout interval in seconds % 3600
case 0xb000020002: return "CLASS_MBUS_SEARCH_INTERVAL";	// search interval in seconds
case 0xb000020003: return "CLASS_MBUS_SEARCH_DEVICE";	// search device now and by restart
case 0xb000020004: return "CLASS_MBUS_AUTO_ACTIVATE";	// automatic activation of meters
case 0xb000020005: return "CLASS_MBUS_BITRATE";	// used baud rates(bitmap)
case 0x100000000ff: return "SERVER_ID_1_1";	// Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
case 0x100000001ff: return "SERVER_ID_1_2";	// Identifikationsnummer 1.2
case 0x100000002ff: return "SERVER_ID_1_3";	// Identifikationsnummer 1.3
case 0x100000003ff: return "SERVER_ID_1_4";	// Identifikationsnummer 1.4
case 0x100000009ff: return "DEVICE_ID";	// encode profiles
case 0x100000100ff: return "REG_MD_RESET_COUNTER";	// MD reset counter
case 0x100000102ff: return "REG_MD_RESET_TIMESTAMP";	// MD reset timestamp
case 0x100000200ff: return "SOFTWARE_ID";
case 0x100000201ff: return "REG_PARAMETERS_SCHEME_ID";	// Parameters scheme ID
case 0x100000202ff: return "REG_TARIFF";	// Tariff program ID
case 0x100000300ff: return "REG_ACTIVE_ENERGY_METER_CONSTANT";	// Active energy meter constant
case 0x100000302ff: return "REG_SM_PULSE_VALUE";	// S0- Impulswertigkeit (0.3.2)
case 0x100000303ff: return "REG_SM_PULSE_DURATION";	// Pulse length (0.3.3)
case 0x100000402ff: return "REG_SM_RATIO_CONVERTER";	// Converter factor (0.4.2)
case 0x100000403ff: return "REG_SM_RATIO_VOLTAGE";	// Voltage transformer (0.4.3)
case 0x100000502ff: return "REG_SM_MEASURING_PERIOD";	// Measuring period (0.2.2)
case 0x100000800ff: return "REG_DP2_DEMAND_INTERVAL";	// Duration of measurement interval for current power value (1-0.0:8.0*255)
case 0x100000804ff: return "REG_LOAD_PROFILE_PERIOD";	// Load profile period
case 0x100000901ff: return "REG_DT_CURRENT_TIME";	// Current time at time of transmission (hh:mm:ss)
case 0x100000902ff: return "REG_DATE";	// Date(YY.MM.DD or DD.MM.YY)
case 0x100000904ff: return "REG_DATE_TIME";	// Date and Time(YYMMDDhhmmss)
case 0x10000090b00: return "CURRENT_UTC";	// readout time in UTC
case 0x100010102ff: return "REG_BATTERY_VOLTAGE";	// Battery voltage
case 0x100010106ff: return "REG_READOUTS";	// Successful readouts since restart
case 0x100010109ff: return "REG_TEMPERATURE";	// Temperature
case 0x100010200ff: return "REG_POS_ACT_CMD";	// Positive active cumulative maximum demand(A+) total
case 0x100010400ff: return "REG_POS_ACT_DCDP";	// Positive active demand in a current demand period(A+)
case 0x100010500ff: return "REG_POS_ACT_DLCDP";	// Positive active demand in the last completed demand period(A+)
case 0x100010600ff: return "REG_POS_ACT_MD";	// Positive active maximum demand(A+) total
case 0x100010601ff: return "REG_POS_ACT_MD_T1";	// Positive active maximum demand(A+) in tariff 1
case 0x100010602ff: return "REG_POS_ACT_MD_T2";	// Positive active maximum demand(A+) in tariff 2
case 0x100010603ff: return "REG_POS_ACT_MD_T3";	// Positive active maximum demand(A+) in tariff 3
case 0x100010604ff: return "REG_POS_ACT_MD_T4";	// Positive active maximum demand(A+) in tariff 4
case 0x100010700ff: return "REG_POS_ACT_IP";	// Positive active instantaneous power(A+)
case 0x10001080060: return "EDL21_LAST_24h";	// Consumption over the last 24 hours
case 0x10001080061: return "EDL21_LAST_7d";	// Consumption over the last 7 days
case 0x10001080062: return "EDL21_LAST_30d";	// Consumption over the last 30 days
case 0x10001080063: return "EDL21_LAST_365d";	// Consumption over the last 365 days
case 0x10001080064: return "EDL21_LAST_RESET";	// Consumption since last reset
case 0x100010800ff: return "REG_POS_ACT_E";	// Positive active energy(A+), current value
case 0x100010801ff: return "REG_POS_ACT_E_T1";	// Positive active energy(A+) in tariff T1
case 0x100010802ff: return "REG_POS_ACT_E_T2";	// Positive active energy(A+) in tariff T2
case 0x100010803ff: return "REG_POS_ACT_E_T3";	// Positive active energy(A+) in tariff T3
case 0x100010804ff: return "REG_POS_ACT_E_T4";	// Positive active energy(A+) in tariff T4
case 0x100020200ff: return "REG_NEG_ACT_CMD";	// Negative active cumulative maximum demand(A-) total
case 0x100020400ff: return "REG_NEG_ACT_DCDP";	// Negative active demand in a current demand period(A-)
case 0x100020500ff: return "REG_NEG_ACT_DLCDP";	// Negative active demand in the last completed demand period(A-)
case 0x100020600ff: return "REG_NEG_ACT_MD";	// Negative active maximum demand(A-) total
case 0x100020601ff: return "REG_NEG_ACT_MD_T1";	// Negative active maximum demand(A-) in tariff 1
case 0x100020602ff: return "REG_NEG_ACT_MD_T2";	// Negative active maximum demand(A-) in tariff 2
case 0x100020603ff: return "REG_NEG_ACT_MD_T3";	// Negative active maximum demand(A-) in tariff 3
case 0x100020604ff: return "REG_NEG_ACT_MD_T4";	// Negative active maximum demand(A-) in tariff 4
case 0x100020700ff: return "REG_NEG_ACT_IP";	// Negative active instantaneous power(A-)
case 0x100020800ff: return "REG_NEG_ACT_E";	// Negative active energy(A+) total
case 0x100020801ff: return "REG_NEG_ACT_E_T1";	// Negative active energy(A+) in tariff T1
case 0x100020802ff: return "REG_NEG_ACT_E_T2";	// Negative active energy(A+) in tariff T2
case 0x100020803ff: return "REG_NEG_ACT_E_T3";	// Negative active energy(A+) in tariff T3
case 0x100020804ff: return "REG_NEG_ACT_E_T4";	// Negative active energy(A+) in tariff T4
case 0x100030200ff: return "REG_PRCMD";	// Positive reactive cumulative maximum demand(Q+) total
case 0x100030400ff: return "REG_PRDCDP";	// Positive reactive demand in a current demand period(Q+)
case 0x100030500ff: return "REG_RDLCDP";	// Positive reactive demand in the last completed demand period(Q+)
case 0x100030600ff: return "REG_PRMD";	// Positive reactive maximum demand(Q+) total
case 0x100030700ff: return "REG_PRIP";	// Positive reactive instantaneous power(Q+)
case 0x100030800ff: return "REG_PRE";	// Positive reactive energy(Q+) total
case 0x100040200ff: return "REG_NRCMD";	// Negative reactive cumulative maximum demand(Q-) total
case 0x100040400ff: return "REG_NRDCDP";	// Negative reactive demand in a current demand period(Q-)
case 0x100040500ff: return "REG_NRDLCDP";	// Negative reactive demand in the last completed demand period(Q-)
case 0x100040600ff: return "REG_NRMD";	// Negative reactive maximum demand(Q-) total
case 0x100040700ff: return "REG_NRIP";	// Negative reactive instantaneous power(Q-)
case 0x100040800ff: return "REG_NRE";	// Negative reactive energy(Q-) total
case 0x100050200ff: return "REG_RCMDQ1";	// Reactive cumulative maximum demand in Q1(Q1) total
case 0x100050400ff: return "REG_RDCDPQ1";	// Reactive demand in a current demand period in Q1(Q1)
case 0x100050500ff: return "REG_RDLCDPQ1";	// Reactive demand in the last completed demand period in Q1(Q1)
case 0x100050600ff: return "REG_RMDQ1";	// Reactive maximum demand in Q1(Q1) total
case 0x100050800ff: return "REG_IIRE";	// Imported inductive reactive energy in 1 - st quadrant(Q1) total
case 0x100060200ff: return "REG_RCMDQ2";	// Reactive cumulative maximum demand in Q2(Q2) total
case 0x100060400ff: return "REG_RDCDPQ2";	// Reactive demand in a current demand period in Q2(Q2)
case 0x100060500ff: return "REG_RDLCDPQ2";	// Reactive demand in the last completed demand period in Q2(Q2)
case 0x100060600ff: return "REG_RMDQ2";	// Reactive maximum demand in Q2(Q2) total
case 0x100060800ff: return "REG_ICRE";	// Imported capacitive reactive energy in 2 - nd quadrant(Q2) total
case 0x100070200ff: return "REG_RCMDQ3";	// Reactive cumulative maximum demand in Q3(Q3) total
case 0x100070400ff: return "REG_RDCDPQ3";	// Reactive demand in a current demand period in Q3(Q3)
case 0x100070500ff: return "REG_RDLCDPQ3";	// Reactive demand in the last completed demand period in Q3(Q3)
case 0x100070600ff: return "REG_RMDQ3";	// Reactive maximum demand in Q3(Q3) total
case 0x100070800ff: return "REG_EIRE";	// Exported inductive reactive energy in 3 - rd quadrant(Q3) total
case 0x100080200ff: return "REG_RCMDQ4";	// Reactive cumulative maximum demand in Q4(Q4) total
case 0x100080400ff: return "REG_RDCDPQ4";	// Reactive demand in a current demand period in Q4(Q4)
case 0x100080500ff: return "REG_RDLCDPQ4";	// Reactive demand in the last completed demand period in Q4(Q4)
case 0x100080600ff: return "REG_RMDQ4";	// Reactive maximum demand in Q4(Q4) total
case 0x100080800ff: return "REG_ECRE";	// Exported capacitive reactive energy in 4 - th quadrant(Q4) total
case 0x100090200ff: return "REG_RCMDT";	// Apparent cumulative maximum demand(S+) total
case 0x100090400ff: return "REG_RDCDPT";	// Apparent demand in a current demand period(S+)
case 0x100090500ff: return "REG_ADLCDP";	// Apparent demand in the last completed demand period(S+)
case 0x100090600ff: return "REG_AMDT";	// Apparent maximum demand(S+) total
case 0x100090601ff: return "REG_AMDT_T1";	// Apparent maximum demand(S+) in tariff 1
case 0x100090602ff: return "REG_AMDT_T2";	// Apparent maximum demand(S+) in tariff 2
case 0x100090603ff: return "REG_AMDT_T3";	// Apparent maximum demand(S+) in tariff 3
case 0x100090604ff: return "REG_AMDT_T4";	// Apparent maximum demand(S+) in tariff 4
case 0x100090700ff: return "REG_AIP";	// Apparent instantaneous power(S+)
case 0x100090800ff: return "REG_AE";	// Apparent energy(S+) total
case 0x1000a0601ff: return "REG_TODO_T1";	// Instantaneous apparent power average (S+) in tariff 1
case 0x1000a0602ff: return "REG_TODO_T2";	// Instantaneous apparent power average (S+) in tariff 2
case 0x1000a0603ff: return "REG_TODO_T3";	// Instantaneous apparent power average (S+) in tariff 3
case 0x1000a0604ff: return "REG_TODO_T4";	// Instantaneous apparent power average (S+) in tariff 4
case 0x1000a0800ff: return "REG_AED";	// Apparent energy total - Delivery (10.8.0)
case 0x1000a0801ff: return "REG_AED_T1";	// Apparent energy tariff 1 - Delivery (10.8.1)
case 0x1000a0802ff: return "REG_AED_T2";	// Apparent energy tariff 2 - Delivery (10.8.2)
case 0x1000a0803ff: return "REG_AED_T3";	// Apparent energy tariff 3 - Delivery (10.8.3)
case 0x1000a0804ff: return "REG_AED_T4";	// Apparent energy tariff 4 - Delivery (10.8.4)
case 0x1000b0600ff: return "REG_MC";	// Maximum current(I max)
case 0x1000b0700ff: return "REG_IC";	// Instantaneous current(I)
case 0x1000c0700ff: return "REG_IV";	// Instantaneous voltage(U)
case 0x1000d0700ff: return "REG_IPF";	// Instantaneous power factor
case 0x1000e0700ff: return "REG_FREQ";	// Frequency
case 0x1000f0200ff: return "REG_AACMD";	// Absolute active cumulative maximum demand(|A| ) total
case 0x1000f0400ff: return "REG_AADCDP";	// Absolute active demand in a current demand period(|A| )
case 0x1000f0500ff: return "REG_AADLCDP";	// Absolute active demand in the last completed demand period(|A| )
case 0x1000f0600ff: return "REG_AAMD";	// Absolute active maximum demand(|A| ) total
case 0x1000f0700ff: return "REG_AAIP";	// Absolute active instantaneous power(|A|)
case 0x1000f0800ff: return "REG_AAE";	// Absolute active energy(A+) total
case 0x100100700ff: return "REG_SIP";	// Sum active instantaneous power(A+ - A-)
case 0x100100800ff: return "REG_SAE";	// Sum active energy without reverse blockade(A + -A - ) total
case 0x100150700ff: return "REG_POS_ACT_INST_PPOW_L1";	// Positive active instantaneous power(A+) in phase L1
case 0x100150800ff: return "REG_POS_ACT_EL1";	// Positive active energy(A+) in phase L1 total
case 0x100160700ff: return "REG_NEG_ACT_IPL1";	// Negative active instantaneous power(A-) in phase L1
case 0x100160800ff: return "REG_NEG_ACT_EL1T";	// Negative active energy(A-) in phase L1 total
case 0x100170700ff: return "REG_PRIPL1";	// Positive reactive instantaneous power(Q+) in phase L1
case 0x100180700ff: return "REG_NRIPL1";	// Negative reactive instantaneous power(Q-) in phase L1
case 0x1001d0700ff: return "REG_AIPL1";	// Apparent instantaneous power(S+) in phase L1
case 0x1001f0600ff: return "REG_MCL1";	// Maximum current(I max) in phase L1
case 0x1001f0700ff: return "REG_ICL1";	// Instantaneous current(I) in phase L1
case 0x100200700ff: return "REG_IVL1";	// Instantaneous voltage(U) in phase L1
case 0x100210700ff: return "REG_IPFL1";	// Instantaneous power factor in phase L1
case 0x100230700ff: return "REG_AAIPL1";	// Absolute active instantaneous power(|A|) in phase L1
case 0x100230800ff: return "REG_AAEL1";	// Absolute active energy(|A|) in phase L1 total
case 0x100240700ff: return "REG_SIPL1";	// Sum active instantaneous power(A + - A-) in phase L1
case 0x100290700ff: return "REG_POS_ACT_INST_PPOW_L2";	// Positive active instantaneous power(A+) in phase L2
case 0x100290800ff: return "REG_POS_ACT_EL2";	// Positive active energy(A+) in phase L2 total
case 0x1002a0700ff: return "REG_NEG_ACT_IPL2";	// Negative active instantaneous power(A-) in phase L2
case 0x1002a0800ff: return "REG_NEG_ACT_EL2T";	// Negative active energy(A-) in phase L2 total
case 0x1002b0700ff: return "REG_PRIPL2";	// Positive reactive instantaneous power(Q+) in phase L2
case 0x1002c0700ff: return "REG_NRIPL2";	// Negative reactive instantaneous power(Q-) in phase L2
case 0x100310700ff: return "REG_AIPL2";	// Apparent instantaneous power(S+) in phase L2
case 0x100330600ff: return "REG_MCL2";	// Maximum current(I max) in phase L2
case 0x100330700ff: return "REG_ICL2";	// Instantaneous current(I) in phase L2
case 0x100340700ff: return "REG_IVL2";	// Instantaneous voltage(U) in phase L2
case 0x100350700ff: return "REG_IPFL2";	// Instantaneous power factor in phase L2
case 0x100370700ff: return "REG_AAIPL2";	// Absolute active instantaneous power(|A|) in phase L2
case 0x100370800ff: return "REG_AAEL2";	// Absolute active energy(|A|) in phase L2 total
case 0x100380700ff: return "REG_SIPL2";	// Sum active instantaneous power(A + - A-) in phase L2
case 0x1003d0700ff: return "REG_PPOS_ACT_INST_PPOW_L3";	// Positive active instantaneous power(A+) in phase L3
case 0x1003d0800ff: return "REG_POS_ACT_EL1T";	// Positive active energy(A+) in phase L3 total
case 0x1003d6100ff: return "REG_FATAL_ERROR_METER_STATUS";	// Fatal error meter status
case 0x1003e0700ff: return "REG_NEG_ACT_IPL3";	// Negative active instantaneous power(A-) in phase L3
case 0x1003e0800ff: return "REG_NEG_ACT_EL3T";	// Negative active energy(A-) in phase L3 total
case 0x1003f0700ff: return "REG_PRIPL3";	// Positive reactive instantaneous power(Q+) in phase L3
case 0x100400700ff: return "REG_NRIPL3";	// Negative reactive instantaneous power(Q-) in phase L3
case 0x100450700ff: return "REG_AIPL3";	// Apparent instantaneous power(S+) in phase L3
case 0x100470600ff: return "REG_MCL3";	// Maximum current(I max) in phase L3
case 0x100470700ff: return "REG_ICL3";	// Instantaneous current(I) in phase L3
case 0x100480700ff: return "REG_IVL3";	// Instantaneous voltage(U) in phase L3
case 0x100490700ff: return "REG_IPFL3";	// Instantaneous power factor in phase L3
case 0x1004b0700ff: return "REG_AAIPL3";	// Absolute active instantaneous power(|A|) in phase L3
case 0x1004b0800ff: return "REG_AAEL3";	// Absolute active energy(|A|) in phase L3 total
case 0x1004c0700ff: return "REG_SIPL3";	// Sum active instantaneous power(A + - A-) in phase L3
case 0x1005b0600ff: return "REG_MCN";	// Maximum current(I max) in neutral
case 0x1005b0700ff: return "REG_ICN";	// Instantaneous current(I) in neutral
case 0x100600200ff: return "REG_EVT_PARAM_CHANGED_COUNTER";	// Event parameters change
case 0x100600201ff: return "REG_EVT_PARAM_CHANGED_TIMESTAMP";	// Event parameters change
case 0x100600700ff: return "REG_EVT_POWER_DOWN_COUNTER";	// Event power down
case 0x10060070aff: return "REG_EVT_POWER_DOWN_TIMESTAMP";	// Event power down
case 0x100603301ff: return "REG_EVT_1";	// Event terminal cover opened
case 0x100603302ff: return "REG_EVT_2";	// Event terminal cover opened
case 0x100603303ff: return "REG_EVT_3";	// Event main cover opened - counter
case 0x100603304ff: return "REG_EVT_4";	// Event main cover opened 
case 0x100603305ff: return "REG_EVT_5";	// Event magnetic field detection start - counter
case 0x100603306ff: return "REG_EVT_6";	// Event magnetic field detection start
case 0x100603307ff: return "REG_EVT_7";	// Event reverse power flow
case 0x100603308ff: return "REG_EVT_8";	// Event reverse power flow
case 0x10060330dff: return "REG_EVT_13";	// Event power up
case 0x10060330eff: return "REG_EVT_14";	// Event power up
case 0x10060330fff: return "REG_EVT_15";	// Event RTC(Real Time Clock) set
case 0x100603310ff: return "REG_EVT_16";	// Event RTC(Real Time Clock) set
case 0x100603315ff: return "REG_EVT_21";	// Event terminal cover closed
case 0x100603316ff: return "REG_EVT_22";	// Event terminal cover closed
case 0x100603317ff: return "REG_EVT_23";	// Event main cover closed
case 0x100603318ff: return "REG_EVT_24";	// Event main cover closed
case 0x100603319ff: return "REG_EVT_25";	// Event log - book 1 erased
case 0x10060331aff: return "REG_EVT_26";	// Event log - book 1 erased
case 0x10060331bff: return "REG_EVT_27";	// Event fraud start
case 0x10060331cff: return "REG_EVT_28";	// Event fraud start
case 0x10060331dff: return "REG_EVT_29";	// Event fraud stop
case 0x10060331eff: return "REG_EVT_30";	// Event fraud stop
case 0x100603501ff: return "REG_TAMPER_ENERGY_1";	// Tamper 1 energy
case 0x100603502ff: return "REG_TAMPER_ENERGY_2";	// Tamper 2 energy
case 0x100603503ff: return "REG_TAMPER_ENERGY_3";	// Tamper 3 energy
case 0x100603504ff: return "REG_TAMPER_ENERGY_4";	// Tamper 4 energy
case 0x100603505ff: return "REG_TAMPER_TIME_1";	// Tamper 1 time counter register
case 0x100603506ff: return "REG_TAMPER_TIME_2";	// Tamper 2 time counter register
case 0x100603507ff: return "REG_TAMPER_TIME_3";	// Tamper 3 time counter register
case 0x100603509ff: return "REG_TAMPER_TIME_4";	// Tamper 4 time counter register
case 0x10060350aff: return "REG_TAMPER_TIME_5";	// Tamper 5 time counter register
case 0x10060350bff: return "REG_TAMPER_ENERGY_5";	// Tamper 5 energy
case 0x100605700ff: return "REG_ACTIVE_TARFIFF";	// Active tariff
case 0x100606009ff: return "REG_FRAUD_FLAG";	// Fraud flag
case 0x101000402ff: return "CURRENT_TRANSFORMER_RATIO";	// Current transformer ratio (L&G)
case 0x101000403ff: return "VOLTAGE_TRANSFORMER_RATIO";	// Voltage transformer ratio (L&G)
case 0x101621700ff: return "REG_SM_POWER_THRESHOLD";	// Power threshold (D.23.0)
case 0x40000010aff: return "REG_HCA_10";	// Local date at set date
case 0x400000901ff: return "REG_HCA_F";	// Current time at time of transmission
case 0x400000902ff: return "REG_HCA_G";	// Current date at time of transmission
case 0x400010001ff: return "REG_HCA_0";	// Unrated integral, current value
case 0x400010200ff: return "REG_HCA_2";	// Unrated integral, set date value
case 0x600000800ff: return "REG_HEAT_AED";	// Power (energy flow) (P ), average, current value
case 0x600010000ff: return "REG_HEAT_CURRENT";	// Energy (A), total, current value
case 0x600010200ff: return "REG_HEAT_SET_DATE";	// Energy (A), total, set date value
case 0x70000081cff: return "REG_GAS_INT";	// Averaging duration for actual flow rate value
case 0x700002a02ff: return "REG_GAS_BP";	// defined Pressure, absolute, at base conditions
case 0x700030000ff: return "REG_GAS_MC_0_0";	// Volume (meter), 
case 0x700030100ff: return "REG_GAS_MC_1_0";	// Volume (meter), temperature converted (Vtc), forward, absolute, current value
case 0x700030200ff: return "REG_GAS_MC_2_0";	// Volume (meter), base conditions (Vb), forward, absolute, current value
case 0x7002b0f00ff: return "REG_GAS_FR_15";	// Flow rate at measuring conditions, averaging period 1 (default period = 5 min), current interval 
case 0x7002b1000ff: return "REG_GAS_FR_16";	// Flow rate, temperature converted, averaging period 1(default period = 5 min), current interval
case 0x7002b1100ff: return "REG_GAS_FR_17";	// Flow rate at base conditions, averaging period 1 (default period = 5 min), current interval
case 0x800010000ff: return "WATER_CURRENT";	// Volume (V), accumulated, total, current value
case 0x800010200ff: return "WATER_DATE";	// Volume (V), accumulated, total, due date value
case 0x800020000ff: return "WATER_FLOW_RATE";	// Flow rate, average (Va/t), current value 
case 0x810000090b00: return "ACT_SENSOR_TIME";	// actSensorTime - current UTC time
case 0x810000090b01: return "TZ_OFFSET";	// offset to actual time zone in minutes (-720 .. +720)
case 0x810060050000: return "CLASS_OP_LOG_STATUS_WORD";
case 0x8101000000ff: return "LOG_SOURCE_ETH_AUX";
case 0x810100000100: return "INTERFACE_01_NAME";	// interface name
case 0x8102000000ff: return "LOG_SOURCE_ETH_CUSTOM";
case 0x810200000100: return "INTERFACE_02_NAME";	// interface name
case 0x8102000700ff: return "ROOT_CUSTOM_INTERFACE";	// see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle
case 0x8102000701ff: return "CUSTOM_IF_IP_REF";	// 0 == manual, 1 == DHCP
case 0x810200070201: return "CUSTOM_IF_DHCP_LOCAL_IP_MASK";
case 0x810200070202: return "CUSTOM_IF_DHCP_DEFAULT_GW";
case 0x810200070203: return "CUSTOM_IF_DHCP_DNS";
case 0x810200070204: return "CUSTOM_IF_DHCP_START_ADDRESS";
case 0x810200070205: return "CUSTOM_IF_DHCP_END_ADDRESS";
case 0x8102000702ff: return "CUSTOM_IF_DHCP";	// if true use a DHCP server
case 0x8102000710ff: return "ROOT_CUSTOM_PARAM";	// see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle 
case 0x810217070000: return "CUSTOM_IF_IP_CURRENT_1";	// current IP address (customer interface)
case 0x810217070001: return "CUSTOM_IF_IP_ADDRESS_1";	// first manual set IP address
case 0x810217070002: return "CUSTOM_IF_IP_ADDRESS_2";	// second manual set IP address
case 0x810217070101: return "CUSTOM_IF_IP_MASK_1";
case 0x810217070102: return "CUSTOM_IF_IP_MASK_2";
case 0x8103000000ff: return "LOG_SOURCE_RS232";
case 0x810300000100: return "INTERFACE_03_NAME";	// interface name
case 0x8104000000ff: return "LOG_SOURCE_ETH";	// WAN interface
case 0x810400000100: return "INTERFACE_04_NAME";	// interface name
case 0x8104000600ff: return "ROOT_WAN";	// see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status 
case 0x8104000700ff: return "ROOT_WAN_PARAM";	// see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter 
case 0x8104020700ff: return "ROOT_GSM";	// see: Datenstruktur zum Lesen/Setzen der GSM Parameter 
case 0x81040d060000: return "CLASS_OP_LOG_PROVIDER";	// 	aktueller Provider-Identifier(uint32)
case 0x81040d0600ff: return "GSM_ADMISSIBLE_OPERATOR";
case 0x81040d0700ff: return "ROOT_GPRS_PARAM";	// see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter 
case 0x81040d0800ff: return "ROOT_GSM_STATUS";
case 0x81040e0600ff: return "PLC_STATUS";
case 0x810417070000: return "CLASS_OP_LOG_AREA_CODE";	// aktueller Location - oder Areacode
case 0x8104180700ff: return "IF_PLC";
case 0x81041a070000: return "CLASS_OP_LOG_CELL";	// aktuelle Zelleninformation
case 0x81042b070000: return "CLASS_OP_LOG_FIELD_STRENGTH";
case 0x8104623c0101: return "PPPoE_USERNAME";	// PPPoE username
case 0x8104623c0201: return "PPPoE_PASSWORD";	// PPPoE passphrase
case 0x8104623c0301: return "PPPoE_MODE";	// PPPoE mode
case 0x8105000000ff: return "LOG_SOURCE_eHZ";
case 0x81050d070001: return "IF_EDL_PROTOCOL";	// always 1
case 0x81050d070002: return "IF_EDL_BAUDRATE";	// 0 = auto, 6 = 9600, 10 = 115200 baud
case 0x81050d0700ff: return "IF_EDL";	// M-Bus EDL (RJ10)
case 0x8106000000ff: return "LOG_SOURCE_wMBUS";
case 0x810600000100: return "W_MBUS_ADAPTER_MANUFACTURER";
case 0x810600000300: return "W_MBUS_ADAPTER_ID";
case 0x810600020000: return "W_MBUS_FIRMWARE";
case 0x8106000203ff: return "W_MBUS_HARDWARE";
case 0x8106000204ff: return "W_MBUS_FIELD_STRENGTH";	// dbm
case 0x8106000374ff: return "W_MBUS_LAST_RECEPTION";	// Time since last radio telegram reception
case 0x81060f0600ff: return "ROOT_W_MBUS_STATUS";	// see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
case 0x8106190700ff: return "ROOT_W_MBUS_PARAM";	// Wireless M-BUS
case 0x8106190701ff: return "W_MBUS_PROTOCOL";
case 0x8106190702ff: return "W_MBUS_MODE_S";
case 0x8106190703ff: return "W_MBUS_MODE_T";
case 0x8106190704ff: return "W_MBUS_POWER";	// 0 = default, 1 = low, 2 = medium, 3 = high
case 0x8106190711ff: return "W_MBUS_INSTALL_MODE";
case 0x810627320301: return "W_MBUS_REBOOT";
case 0x81062b070000: return "W_MBUS_FIELD_STRENGTH_2";	// Receive field strength
case 0x8106643c0101: return "W_MBUS_MAX_MSG_TIMEOUT";
case 0x8106643c0104: return "W_MBUS_MAX_SML_TIMEOUT_IN";	// max timeout between SML close request and SML open response from device to gateway
case 0x8106643c0105: return "W_MBUS_MAX_SML_TIMEOUT_OUT";	// max timeout between SML close request and SML open response from gateway to device
case 0x8141000000ff: return "LOG_SOURCE_IP";
case 0x814200000001: return "LOG_SOURCE_SML_EXT";
case 0x814200000002: return "LOG_SOURCE_SML_CUSTOM";
case 0x814200000003: return "LOG_SOURCE_SML_SERVICE";
case 0x814200000004: return "LOG_SOURCE_SML_WAN";
case 0x814200000005: return "LOG_SOURCE_SML_eHZ";
case 0x814200000006: return "LOG_SOURCE_SML_wMBUS";
case 0x8145000000ff: return "LOG_SOURCE_PUSH_SML";
case 0x8146000000ff: return "LOG_SOURCE_PUSH_IPT_SOURCE";
case 0x8146000002ff: return "PROFILE_ADDRESS";
case 0x8147000000ff: return "LOG_SOURCE_PUSH_IPT_SINK";
case 0x8147170700ff: return "PUSH_TARGET";	// push target name
case 0x814800000000: return "COMPUTER_NAME";
case 0x814800000001: return "LOG_SOURCE_WAN_DHCP";
case 0x814800000002: return "LOG_SOURCE_WAN_IP";
case 0x814800000003: return "LOG_SOURCE_WAN_PPPoE";
case 0x814800320101: return "LAN_PPPoE_ENABLED";
case 0x814800320201: return "LAN_DHCP_ENABLED";
case 0x814800320301: return "LAN_DSL_ENABLED";	// true = DLS, false = LAN
case 0x81480d0600ff: return "ROOT_LAN_DSL";	// see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter
case 0x814817070000: return "CODE_IF_LAN_ADDRESS";	// IPv4 or IPv6 address
case 0x814817070001: return "CODE_IF_DSL_ADDRESS";	// IPv4 or IPv6 address
case 0x8148170700ff: return "IF_LAN_DSL";	// see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter
case 0x814817070100: return "CODE_IF_LAN_SUBNET_MASK";
case 0x814817070200: return "CODE_IF_LAN_GATEWAY";
case 0x814817070400: return "CODE_IF_LAN_DNS_PRIMARY";
case 0x814817070401: return "CODE_IF_LAN_DSL_PRIMARY";	// set/get OBIS_IF_LAN_DSL
case 0x814817070500: return "CODE_IF_LAN_DNS_SECONDARY";
case 0x814817070501: return "CODE_IF_LAN_DSL_SECONDARY";	// set/get OBIS_IF_LAN_DSL
case 0x814817070600: return "CODE_IF_LAN_DNS_TERTIARY";
case 0x814817070601: return "CODE_IF_LAN_DLS_TERTIARY";	// set/get OBIS_IF_LAN_DSL
case 0x814827320601: return "TCP_WAIT_TO_RECONNECT";
case 0x814831320201: return "TCP_CONNECT_RETRIES";
case 0x814831320701: return "TCP_REPLY_ICMP";	// reply to received ICMP packages
case 0x814900000001: return "LOG_SOURCE_WAN_IPT_CONTROLLER";
case 0x814900000002: return "LOG_SOURCE_WAN_IPT";
case 0x8149000010ff: return "PUSH_SERVICE";	// options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX
case 0x81490d0600ff: return "ROOT_IPT_STATE";	// see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status 
case 0x81490d0700ff: return "ROOT_IPT_PARAM";	// see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter 
case 0x814917070000: return "TARGET_IP_ADDRESS";	// ip address of IP-T master 
case 0x814919070000: return "SOURCE_PORT";	// target port of IP-T master 
case 0x81491a070000: return "TARGET_PORT";	// source port of IP-T master 
case 0x8149633c0100: return "IPT_ACCOUNT";
case 0x8149633c0200: return "IPT_PASSWORD";
case 0x8149633c0300: return "IPT_SCRAMBLED";
case 0x814a000000ff: return "LOG_SOURCE_WAN_NTP";
case 0x818100000001: return "PEER_OBISLOG";	// peer address: OBISLOG
case 0x818100000002: return "PEER_SCM";	// peer address: SCM (power)
case 0x818100000010: return "PEER_EHZIF";	// peer address: EHZIF
case 0x818100000011: return "PEER_USERIF";	// peer address: USERIF
case 0x818100000012: return "PEER_WMBUS";	// internal MUC software function
case 0x818100000013: return "PEER_ADDRESS_WANGSM";	// peer address: WAN/GSM
case 0x818100000014: return "PEER_WANPLC";	// internal MUC software function
case 0x8181000000ff: return "PEER_ADDRESS";	// unit is 255
case 0x818100020000: return "VERSION";	// (0.2.0) firmware revision
case 0x818100020001: return "SET_START_FW_UPDATE";
case 0x818100020002: return "FILE_NAME";
case 0x818100020003: return "MSG_COUNTER";	// Anzahl aller Nachrichten zur Übertragung des Binary
case 0x818100020004: return "LAST_MSG";	// Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary
case 0x818100020005: return "MSG_NUMBER";
case 0x8181000202ff: return "BLOCK_NUMBER";
case 0x8181000203ff: return "BINARY_DATA";
case 0x818100030001: return "SET_DISPATCH_FW_UPDATE";
case 0x818110000001: return "LOG_SOURCE_LOG";
case 0x818110000002: return "LOG_SOURCE_SCM";	// internal software function
case 0x818110000003: return "LOG_SOURCE_UPDATE";
case 0x818110000004: return "LOG_SOURCE_SMLC";
case 0x81811000000c: return "LOG_SOURCE_LEDIO";
case 0x818110000014: return "LOG_SOURCE_WANPLC";
case 0x8181100601ff: return "CODE_LIST_1_VISIBLE_DEVICES";	// 1. Liste der sichtbaren Sensoren
case 0x8181100602ff: return "CODE_LIST_2_VISIBLE_DEVICES";	// 2. Liste der sichtbaren Sensoren
case 0x81811006ffff: return "ROOT_VISIBLE_DEVICES";	// visible devices (Liste der sichtbaren Sensoren)
case 0x81811016ffff: return "ROOT_NEW_DEVICES";	// new active devices
case 0x81811026ffff: return "ROOT_INVISIBLE_DEVICES";	// not longer visible devices
case 0x8181110601ff: return "CODE_LIST_1_ACTIVE_DEVICES";	// 1. Liste der aktiven Sensoren)
case 0x8181110602ff: return "CODE_LIST_2_ACTIVE_DEVICES";	// 2. Liste der aktiven Sensoren)
case 0x81811106fbff: return "ACTIVATE_DEVICE";	// activate meter/sensor
case 0x81811106fcff: return "DEACTIVATE_DEVICE";	// deactivate meter/sensor
case 0x81811106fdff: return "DELETE_DEVICE";	// delete meter/sensor
case 0x81811106ffff: return "ROOT_ACTIVE_DEVICES";	// active devices (Liste der aktiven Sensoren)
case 0x81811206ffff: return "ROOT_DEVICE_INFO";	// extended device information
case 0x818127320701: return "OBISLOG_INTERVAL";	//  Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook
case 0x8181613c01ff: return "DATA_USER_NAME";
case 0x8181613c02ff: return "DATA_USER_PWD";
case 0x81818160ffff: return "ROOT_ACCESS_RIGHTS";	// see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte 
case 0x81818161ffff: return "ACCESS_USER_NAME";	// user name for access
case 0x81818162ffff: return "ACCESS_PASSWORD";	// SHA256 encrypted
case 0x81818163ffff: return "ACCESS_PUBLIC_KEY";
case 0x8181c78101ff: return "ROOT_FILE_TRANSFER";	// 7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) 
case 0x8181c7810cff: return "DATA_FILENAME";
case 0x8181c7810dff: return "DATA_APPLICATION";
case 0x8181c7810eff: return "DATA_FIRMWARE";
case 0x8181c7810fff: return "DATA_FILENAME_INDIRECT";
case 0x8181c78110ff: return "DATA_APPLICATION_INDIRECT";
case 0x8181c78123ff: return "DATA_PUSH_DETAILS";
case 0x8181c78201ff: return "ROOT_DEVICE_IDENT";	// see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) 
case 0x8181c78202ff: return "DEVICE_CLASS";	// Geräteklasse (OBIS code or '2D 2D 2D')
case 0x8181c78203ff: return "DATA_MANUFACTURER";	// FLAG
case 0x8181c78204ff: return "SERVER_ID";	// Server ID 
case 0x8181c78205ff: return "DATA_PUBLIC_KEY";
case 0x8181c78206ff: return "ROOT_FIRMWARE";	// Firmware
case 0x8181c78208ff: return "DEVICE_KERNEL";
case 0x8181c78209ff: return "HARDWARE_FEATURES";	// hardware equipment (charge, type, ...) 81 81 C7 82 0A NN
case 0x8181c7820a01: return "DEVICE_MODEL";	// model code (VMET-1KW-221-1F0)
case 0x8181c7820a02: return "DEVICE_SERIAL";	// serial number (3894517)
case 0x8181c7820eff: return "DEVICE_ACTIVATED";
case 0x8181c78241ff: return "DEV_CLASS_BASIC_DIRECT";	// 3 x 230 /400 V and 5 (100) A 
case 0x8181c78242ff: return "DEV_CLASS_BASIC_SEMI";	// 3 x 230 /400 V and 1 (6) A
case 0x8181c78243ff: return "DEV_CLASS_BASIC_INDIRECT";	// 3 x  58 / 100 V and 1 (6) A 
case 0x8181c78245ff: return "DEV_CLASS_IW";	// IW module
case 0x8181c78246ff: return "DEV_CLASS_PSTN";	// PSTN/GPRS
case 0x8181c78247ff: return "DEV_CLASS_GPRS";	// GPRS/PLC
case 0x8181c78248ff: return "DEV_CLASS_KM";	// KM module (LAN/DSL)
case 0x8181c78249ff: return "DEV_CLASS_NK";	// NK/HS
case 0x8181c7824aff: return "DEV_CLASS_EXTERN";	// external load profile collector 
case 0x8181c7824bff: return "DEV_CLASS_GW";	// gateway
case 0x8181c7824fff: return "DEV_CLASS_LAN";	// see DEV_CLASS_MUC_LAN
case 0x8181c78250ff: return "DEV_CLASS_eHZ";
case 0x8181c78252ff: return "DEV_CLASS_3HZ";
case 0x8181c78253ff: return "DEV_CLASS_MUC_LAN";	// (MUC-LAN/DSL)
case 0x8181c78281ff: return "DATA_IP_ADDRESS";
case 0x8181c7838201: return "REBOOT";
case 0x8181c7838205: return "CLEAR_DATA_COLLECTOR";
case 0x8181c7838207: return "UPDATE_FW";
case 0x8181c783820e: return "SET_ACTIVATE_FW";
case 0x8181c78600ff: return "ROOT_SENSOR_PARAMS";	// data mirror root element (Eigenschaften eines Datenspiegels)
case 0x8181c78601ff: return "ROOT_SENSOR_BITMASK";	//  Bitmask to define bits that will be transferred into log
case 0x8181c78602ff: return "AVERAGE_TIME_MS";	// average time between two received data records (milliseconds)
case 0x8181c78603ff: return "DATA_AES_KEY";
case 0x8181c78604ff: return "TIME_REFERENCE";	// 0 == UTC, 1 == UTC + time zone, 2 == local time
case 0x8181c78610ff: return "PROFILE_1_MINUTE";	// 1 minute
case 0x8181c78611ff: return "PROFILE_15_MINUTE";
case 0x8181c78612ff: return "PROFILE_60_MINUTE";
case 0x8181c78613ff: return "PROFILE_24_HOUR";
case 0x8181c78614ff: return "PROFILE_LAST_2_HOURS";	// past two hours
case 0x8181c78615ff: return "PROFILE_LAST_WEEK";	// weekly (on day change from Sunday to Monday)
case 0x8181c78616ff: return "PROFILE_1_MONTH";	// monthly recorded meter readings
case 0x8181c78617ff: return "PROFILE_1_YEAR";	// annually recorded meter readings
case 0x8181c78618ff: return "PROFILE_INITIAL";	// 81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods
case 0x8181c78620ff: return "ROOT_DATA_COLLECTOR";	// data collector root element (Eigenschaften eines Datensammlers)
case 0x8181c78621ff: return "DATA_COLLECTOR_ACTIVE";
case 0x8181c78622ff: return "DATA_COLLECTOR_SIZE";	// max. table size
case 0x8181c7862fff: return "TMPL_VERSION";	// Version identifier
case 0x8181c78630ff: return "TMPL_VERSION_FILTER";	// Version identifier filter
case 0x8181c78631ff: return "TMPL_DEVICE_CLASS";	// Device class, on the basis of which associated sensors / actuators are identified
case 0x8181c78632ff: return "TMPL_DEVICE_CLASS_FILTER";	// Filter for the device class
case 0x8181c78781ff: return "DATA_REGISTER_PERIOD";	// register period in seconds (0 == event driven)
case 0x8181c78801ff: return "ROOT_NTP";
case 0x8181c78802ff: return "NTP_SERVER";	// List of NTP servers
case 0x8181c78803ff: return "NTP_PORT";	// NTP port (123)
case 0x8181c78804ff: return "NTP_TZ";	// timezone
case 0x8181c78805ff: return "CODE_NTP_OFFSET";	// Offset to transmission of the signal for synchronization
case 0x8181c78806ff: return "NTP_ACTIVE";	// NTP enabled/disables
case 0x8181c78810ff: return "ROOT_DEVICE_TIME";	// device time
case 0x8181c789e1ff: return "CLASS_OP_LOG";
case 0x8181c789e2ff: return "CLASS_EVENT";	// event
case 0x8181c78a01ff: return "ROOT_PUSH_OPERATIONS";	// push root element
case 0x8181c78a02ff: return "PUSH_INTERVAL";	// in seconds
case 0x8181c78a03ff: return "PUSH_DELAY";	// in seconds
case 0x8181c78a04ff: return "PUSH_SOURCE";	// options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST
case 0x8181c78a21ff: return "PUSH_SERVICE_IPT";	// SML data response without request - typical IP - T push
case 0x8181c78a22ff: return "PUSH_SERVICE_SML";	// SML data response without request - target is SML client address
case 0x8181c78a23ff: return "DATA_COLLECTOR_REGISTER";	// OBIS list (data mirror)
case 0x8181c78a42ff: return "PUSH_SOURCE_PROFILE";	// new meter/sensor data
case 0x8181c78a43ff: return "PUSH_SOURCE_INSTALL";	// configuration changed
case 0x8181c78a44ff: return "PUSH_SOURCE_VISIBLE_SENSORS";	// list of visible meters changed
case 0x8181c78a45ff: return "PUSH_SOURCE_ACTIVE_SENSORS";	// list of active meters changed
case 0x8181c78a81ff: return "PUSH_SERVER_ID";
case 0x8181c78a82ff: return "PUSH_IDENTIFIERS";	// list of registers to be delivered by the push source
case 0x8181c78a83ff: return "PROFILE";	// encode profiles
case 0x8181c79000ff: return "ROOT_IF";
case 0x8181c79300ff: return "IF_1107";
case 0x8181c79301ff: return "IF_1107_ACTIVE";	//  if true 1107 interface active otherwise SML interface active
case 0x8181c79302ff: return "IF_1107_LOOP_TIME";	// Loop timeout in seconds
case 0x8181c79303ff: return "IF_1107_RETRIES";	// Retry count
case 0x8181c79304ff: return "IF_1107_MIN_TIMEOUT";	// Minimal answer timeout(300)
case 0x8181c79305ff: return "IF_1107_MAX_TIMEOUT";	//  Maximal answer timeout(5000)
case 0x8181c79306ff: return "IF_1107_MAX_DATA_RATE";	// Maximum data bytes(10240)
case 0x8181c79307ff: return "IF_1107_RS485";	// if true RS 485, otherwise RS 323
case 0x8181c79308ff: return "IF_1107_PROTOCOL_MODE";	// Protocol mode(A ... D)
case 0x8181c79309ff: return "IF_1107_METER_LIST";	// Liste der abzufragenden 1107 Zähler
case 0x8181c7930aff: return "IF_1107_METER_ID";
case 0x8181c7930bff: return "IF_1107_BAUDRATE";
case 0x8181c7930cff: return "IF_1107_ADDRESS";
case 0x8181c7930dff: return "IF_1107_P1";
case 0x8181c7930eff: return "IF_1107_W5";
case 0x8181c79310ff: return "IF_1107_AUTO_ACTIVATION";
case 0x8181c79311ff: return "IF_1107_TIME_GRID";	// time grid of load profile readout in seconds
case 0x8181c79313ff: return "IF_1107_TIME_SYNC";	// time sync in seconds
case 0x8181c79314ff: return "IF_1107_MAX_VARIATION";	// seconds
case 0x8181c7c7fd00: return "ATTENTION_OK";	// no error
case 0x8181c7c7fd01: return "ATTENTION_JOB_IS_RUNNINNG";	// attention: job is running
case 0x8181c7c7fe00: return "ATTENTION_UNKNOWN_ERROR";	// attention: unknown error
case 0x8181c7c7fe01: return "ATTENTION_UNKNOWN_SML_ID";	// attention: unknown SML ID
case 0x8181c7c7fe02: return "ATTENTION_NOT_AUTHORIZED";	// attention: not authorized
case 0x8181c7c7fe03: return "ATTENTION_NO_SERVER_ID";	// attention: unable to find recipient for request
case 0x8181c7c7fe04: return "ATTENTION_NO_REQ_FIELD";	// attention: no request field
case 0x8181c7c7fe05: return "ATTENTION_CANNOT_WRITE";	// attention: cannot write
case 0x8181c7c7fe06: return "ATTENTION_CANNOT_READ";	// attention: cannot read
case 0x8181c7c7fe07: return "ATTENTION_COMM_ERROR";	// attention: communication error
case 0x8181c7c7fe08: return "ATTENTION_PARSER_ERROR";	// attention: parser error
case 0x8181c7c7fe09: return "ATTENTION_OUT_OF_RANGE";	// attention: out of range
case 0x8181c7c7fe0a: return "ATTENTION_NOT_EXECUTED";	// attention: not executed
case 0x8181c7c7fe0b: return "ATTENTION_INVALID_CRC";	// attention: invalid CRC
case 0x8181c7c7fe0c: return "ATTENTION_NO_BROADCAST";	// attention: no broadcast
case 0x8181c7c7fe0d: return "ATTENTION_UNEXPECTED_MSG";	// attention: unexpected message
case 0x8181c7c7fe0e: return "ATTENTION_UNKNOWN_OBIS_CODE";	// attention: unknown OBIS code
case 0x8181c7c7fe0f: return "ATTENTION_UNSUPPORTED_DATA_TYPE";	// attention: data type not supported
case 0x8181c7c7fe10: return "ATTENTION_ELEMENT_NOT_OPTIONAL";	// attention: element is not optional
case 0x8181c7c7fe11: return "ATTENTION_NO_ENTRIES";	// attention: no entries
case 0x8181c7c7fe12: return "ATTENTION_END_LIMIT_BEFORE_START";	// attention: end limit before start
case 0x8181c7c7fe13: return "ATTENTION_NO_ENTRIES_IN_RANGE";	// attention: range is empty - not the profile
case 0x8181c7c7fe14: return "ATTENTION_MISSING_CLOSE_MSG";	// attention: missing close message
case 0x9000000000ff: return "ROOT_BROKER";	// 90 00 00 00 00 NN - broker list
case 0x9000000001ff: return "BROKER_LOGIN";
case 0x9000000002ff: return "BROKER_SERVER";	// ip address
case 0x9000000003ff: return "BROKER_SERVICE";	// port
case 0x9000000004ff: return "BROKER_USER";
case 0x9000000005ff: return "BROKER_PWD";
case 0x9000000006ff: return "BROKER_TIMEOUT";
case 0x90000000a0ff: return "BROKER_BLOCKLIST";
case 0x9100000000ff: return "ROOT_SERIAL";
case 0x9100000001ff: return "SERIAL_NAME";	// example: /dev/ttyAPP0
case 0x9100000002ff: return "SERIAL_DATABITS";
case 0x9100000003ff: return "SERIAL_PARITY";
case 0x9100000004ff: return "SERIAL_FLOW_CONTROL";
case 0x9100000005ff: return "SERIAL_STOPBITS";
case 0x9100000006ff: return "SERIAL_SPEED";
case 0x9100000007ff: return "SERIAL_TASK";	// LMN port task
case 0x9200000000ff: return "ROOT_NMS";
case 0x9200000001ff: return "NMS_ADDRESS";
case 0x9200000002ff: return "NMS_PORT";
case 0x9200000003ff: return "NMS_USER";
case 0x9200000004ff: return "NMS_PWD";
case 0x9200000005ff: return "NMS_ENABLED";
case 0x9300000000ff: return "ROOT_REDIRECTOR";
case 0x9300000001ff: return "REDIRECTOR_LOGIN";
case 0x9300000002ff: return "REDIRECTOR_ADDRESS";	// ip address
case 0x9300000003ff: return "REDIRECTOR_SERVICE";	// port
case 0x9300000004ff: return "REDIRECTOR_USER";
case 0x9300000005ff: return "REDIRECTOR_PWD";
case 0x990000000003: return "LIST_CURRENT_DATA_RECORD";	// current data set
case 0x990000000004: return "LIST_SERVICES";
case 0x990000000005: return "FTP_UPDATE";
