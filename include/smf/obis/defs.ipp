	// generated at 2023-03-01 09:07:58
	// 617 OBIS codes (3702 Bytes)

	// #1
	// Abstract objects
	OBIS_CODE_DEFINITION(00, 00, 00, 00, 00, ff, METER_ADDRESS);	// device address - 0-0:0.0.0.255
	OBIS_CODE_DEFINITION(00, 00, 00, 00, 01, ff, IDENTITY_NR_1);	// 0-0:0.0.1.255
	OBIS_CODE_DEFINITION(00, 00, 00, 00, 02, ff, IDENTITY_NR_2);	// 0-0:0.0.2.255
	OBIS_CODE_DEFINITION(00, 00, 00, 00, 03, ff, IDENTITY_NR_3);	// 0-0:0.0.3.255
	OBIS_CODE_DEFINITION(00, 00, 00, 01, 00, ff, RESET_COUNTER);	// u32 - 0.1.0
	OBIS_CODE_DEFINITION(00, 00, 00, 02, 00, ff, FIRMWARE_VERSION);	// COSEM class id 1
	OBIS_CODE_DEFINITION(00, 00, 01, 00, 00, ff, REAL_TIME_CLOCK);	// current time
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 00, ff, SERIAL_NR);	// (C.1.0) Serial number I (assigned by the manufacturer)
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 01, ff, SERIAL_NR_SECOND);	// Serial number II (customer ID, assigned by the manufacturer).
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 02, ff, PARAMETERS_FILE_CODE);	// Parameters file code (C.1.2)
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 03, ff, PRODUCTION_DATE);	// date of manufacture (C.1.3)
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 04, ff, PARAMETERS_CHECK_SUM);	// Parameters check sum (C.1.4)
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 05, ff, FIRMWARE_BUILD_DATE);	// Firmware built date (C.1.5)
	OBIS_CODE_DEFINITION(00, 00, 60, 01, 06, ff, FIRMWARE_CHECK_SUM);	// Firmware check sum (C.1.6)
	OBIS_CODE_DEFINITION(00, 00, 60, 01, ff, ff, FABRICATION_NR);	// fabrication number
	OBIS_CODE_DEFINITION(00, 00, 60, 02, 01, ff, DATE_TIME_PARAMETERISATION);	// Date of last parameterisation (00-03-26)
	OBIS_CODE_DEFINITION(00, 00, 60, 03, 00, ff, PULSE_CONST_ACTIVE);	// u32 - Active pulse constant (C.3.0)
	OBIS_CODE_DEFINITION(00, 00, 60, 03, 01, ff, PULSE_CONST_REACTIVE);	// u32 - Reactive pulse constant (C.3.1)
	OBIS_CODE_DEFINITION(00, 00, 60, 06, 00, ff, COUNTER_POWER_DOWN_TIME);	// u32 - Power down time counter (C.6.0)
	OBIS_CODE_DEFINITION(00, 00, 60, 06, 01, ff, BATTERY_REMAINING_CAPACITY);	// u32 - Battery remaining capacity (C.6.1)
	OBIS_CODE_DEFINITION(00, 00, 60, 06, 02, ff, BATTERY_LAST_CHANGE);	// chrono:tp - Date of last battery change (C.6.2)
	OBIS_CODE_DEFINITION(00, 00, 60, 06, 03, ff, BATTERY_VOLTAGE);	// u32 - Battery Voltage (C.6.3)
	OBIS_CODE_DEFINITION(00, 00, 60, 07, 00, ff, POWER_OUTAGES);	// u32 - Number of power failures 
	OBIS_CODE_DEFINITION(00, 00, 60, 08, 00, ff, SECONDS_INDEX);	// [SML_Time] seconds index
	OBIS_CODE_DEFINITION(00, 00, 60, 10, 00, ff, LOGICAL_NAME);
	OBIS_CODE_DEFINITION(00, 00, 60, 48, 00, ff, COUNTER_UNDER_VOLTAGE);	// u32 - Number of under-voltages
	OBIS_CODE_DEFINITION(00, 00, 60, 49, 00, ff, COUNTER_OVER_VOLTAGE);	// u32 - Number of over-voltages
	OBIS_CODE_DEFINITION(00, 00, 60, 4a, 00, ff, COUNTER_OVER_LOAD);	// u32 - Number of over-loads (over-current) 
	OBIS_CODE_DEFINITION(00, 00, 60, f0, 0d, ff, HARDWARE_TYPE);	// s - name
	OBIS_CODE_DEFINITION(00, 00, 61, 61, 00, ff, MBUS_STATE);	// Status according to EN13757-3 (error register)
	OBIS_CODE_DEFINITION(00, 00, 61, 61, 01, ff, MBUS_STATE_1);	// not relevant under calibration law
	OBIS_CODE_DEFINITION(00, 00, 61, 61, 02, ff, MBUS_STATE_2);	// not relevant under calibration law
	OBIS_CODE_DEFINITION(00, 00, 61, 61, 03, ff, MBUS_STATE_3);	// not relevant under calibration law
	OBIS_CODE_DEFINITION(00, 01, 60, 02, 05, ff, LAST_CALIBRATION);	// Date and time of last calibration (L&G)
	OBIS_CODE_DEFINITION(00, 02, 60, f0, 08, ff, HARDWARE_ID_EXT);	// Hardware ID of extension board (L&G)
	OBIS_CODE_DEFINITION(00, 02, 60, f0, 09, ff, HARDWARE_REF_EXT);	// Reference hardware ID of extension board (L&G)
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 00, ff, STORAGE_TIME_SHIFT);	// root push operations
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 01, 01, HAS_WAN);	// WAN on customer interface
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 03, 01, HAS_SSL_CONFIG);	// SSL/TSL configuration available
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 04, ff, SSL_CERTIFICATES);	// list of SSL certificates
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 10, ff, ROOT_MEMORY_USAGE);	// request memory usage
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 11, ff, ROOT_MEMORY_MIRROR);
	OBIS_CODE_DEFINITION(00, 80, 80, 00, 12, ff, ROOT_MEMORY_TMP);
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 00, ff, ROOT_SECURITY);	// list some basic security information
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 01, ff, SECURITY_SERVER_ID);
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 02, ff, SECURITY_OWNER);
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 05, ff, SECURITY_05);	// 3
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 06, 10, SECURITY_06_10);	// 3
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 06, ff, SECURITY_06);	// 3
	OBIS_CODE_DEFINITION(00, 80, 80, 01, 07, ff, SECURITY_07);	// 1
	OBIS_CODE_DEFINITION(00, 80, 80, 10, 00, 01, CLASS_OP_LSM_STATUS);	// LSM status
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 00, ff, ACTUATORS);	// list of actuators
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 10, ff, CLASS_OP_LSM_ACTOR_ID);	// ServerID des Aktors (uint16)
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 11, 01, CLASS_OP_LSM_SWITCH);	// Schaltanforderung
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 11, ff, CLASS_OP_LSM_CONNECT);	// Connection state
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 13, ff, CLASS_OP_LSM_FEEDBACK);	// feedback configuration
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 14, ff, CLASS_OP_LSM_LOAD);	// number of loads
	OBIS_CODE_DEFINITION(00, 80, 80, 11, 15, ff, CLASS_OP_LSM_POWER);	// total power
	OBIS_CODE_DEFINITION(00, 80, 80, 11, a0, ff, CLASS_STATUS);	// see: 2.2.1.3 Status der Aktoren (Kontakte)
	OBIS_CODE_DEFINITION(00, 80, 80, 11, a1, ff, CLASS_OP_LSM_VERSION);	// LSM version
	OBIS_CODE_DEFINITION(00, 80, 80, 11, a2, ff, CLASS_OP_LSM_TYPE);	// LSM type
	OBIS_CODE_DEFINITION(00, 80, 80, 11, a9, 01, CLASS_OP_LSM_ACTIVE_RULESET);	// active ruleset
	OBIS_CODE_DEFINITION(00, 80, 80, 11, a9, 02, CLASS_OP_LSM_PASSIVE_RULESET);
	OBIS_CODE_DEFINITION(00, 80, 80, 14, 03, ff, CLASS_OP_LSM_JOB);	// Schaltauftrags ID ((octet string)	
	OBIS_CODE_DEFINITION(00, 80, 80, 14, 20, ff, CLASS_OP_LSM_POSITION);	// current position
	OBIS_CODE_DEFINITION(00, b0, 00, 02, 00, 00, CLASS_MBUS);
	OBIS_CODE_DEFINITION(00, b0, 00, 02, 00, 01, CLASS_MBUS_RO_INTERVAL);	// readout interval in seconds % 3600
	OBIS_CODE_DEFINITION(00, b0, 00, 02, 00, 02, CLASS_MBUS_SEARCH_INTERVAL);	// search interval in seconds
	OBIS_CODE_DEFINITION(00, b0, 00, 02, 00, 03, CLASS_MBUS_SEARCH_DEVICE);	// bool - search device now and by restart
	OBIS_CODE_DEFINITION(00, b0, 00, 02, 00, 04, CLASS_MBUS_AUTO_ACTIVATE);	// bool - automatic activation of meters
	OBIS_CODE_DEFINITION(00, b0, 00, 02, 00, 05, CLASS_MBUS_BITRATE);	// used baud rates(bitmap)
	// #72
	// Electricity
	OBIS_CODE_DEFINITION(01, 00, 00, 00, 00, ff, SERVER_ID_1_1);	// Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
	OBIS_CODE_DEFINITION(01, 00, 00, 00, 01, ff, SERVER_ID_1_2);	// Identifikationsnummer 1.2
	OBIS_CODE_DEFINITION(01, 00, 00, 00, 02, ff, SERVER_ID_1_3);	// Identifikationsnummer 1.3
	OBIS_CODE_DEFINITION(01, 00, 00, 00, 03, ff, SERVER_ID_1_4);	// Identifikationsnummer 1.4
	OBIS_CODE_DEFINITION(01, 00, 00, 00, 09, ff, DEVICE_ID);	// encode profiles
	OBIS_CODE_DEFINITION(01, 00, 00, 01, 00, ff, REG_MD_RESET_COUNTER);	// MD reset counter
	OBIS_CODE_DEFINITION(01, 00, 00, 01, 02, ff, REG_MD_RESET_TIMESTAMP);	// MD reset timestamp
	OBIS_CODE_DEFINITION(01, 00, 00, 02, 00, ff, SOFTWARE_ID);	// binary
	OBIS_CODE_DEFINITION(01, 00, 00, 02, 01, ff, REG_PARAMETERS_SCHEME_ID);	// Parameters scheme ID
	OBIS_CODE_DEFINITION(01, 00, 00, 02, 02, ff, REG_TARIFF);	// Tariff program ID
	OBIS_CODE_DEFINITION(01, 00, 00, 03, 00, ff, REG_ACTIVE_ENERGY_METER_CONSTANT);	// Active energy meter constant
	OBIS_CODE_DEFINITION(01, 00, 00, 03, 02, ff, REG_SM_PULSE_VALUE);	// S0- Impulswertigkeit (0.3.2)
	OBIS_CODE_DEFINITION(01, 00, 00, 03, 03, ff, REG_SM_PULSE_DURATION);	// Pulse length (0.3.3)
	OBIS_CODE_DEFINITION(01, 00, 00, 04, 02, ff, REG_SM_RATIO_CONVERTER);	// Converter factor (0.4.2)
	OBIS_CODE_DEFINITION(01, 00, 00, 04, 03, ff, REG_SM_RATIO_VOLTAGE);	// Voltage transformer (0.4.3)
	OBIS_CODE_DEFINITION(01, 00, 00, 05, 02, ff, REG_SM_MEASURING_PERIOD);	// Measuring period (0.2.2)
	OBIS_CODE_DEFINITION(01, 00, 00, 08, 00, ff, REG_DP2_DEMAND_INTERVAL);	// Duration of measurement interval for current power value (1-0.0:8.0*255)
	OBIS_CODE_DEFINITION(01, 00, 00, 08, 04, ff, REG_LOAD_PROFILE_PERIOD);	// Load profile period
	OBIS_CODE_DEFINITION(01, 00, 00, 09, 01, ff, REG_DT_CURRENT_TIME);	// Current time at time of transmission (hh:mm:ss)
	OBIS_CODE_DEFINITION(01, 00, 00, 09, 02, ff, REG_DATE);	// Date(YY.MM.DD or DD.MM.YY)
	OBIS_CODE_DEFINITION(01, 00, 00, 09, 04, ff, REG_DATE_TIME);	// Date and Time(YYMMDDhhmmss)
	OBIS_CODE_DEFINITION(01, 00, 00, 09, 0b, 00, CURRENT_UTC);	// readout time in UTC
	OBIS_CODE_DEFINITION(01, 00, 01, 01, 02, ff, REG_BATTERY_VOLTAGE);	// Battery voltage
	OBIS_CODE_DEFINITION(01, 00, 01, 01, 06, ff, REG_READOUTS);	// Successful readouts since restart
	OBIS_CODE_DEFINITION(01, 00, 01, 01, 09, ff, REG_TEMPERATURE);	// Temperature
	OBIS_CODE_DEFINITION(01, 00, 01, 02, 00, ff, REG_POS_ACT_CMD);	// Positive active cumulative maximum demand(A+) total
	OBIS_CODE_DEFINITION(01, 00, 01, 04, 00, ff, REG_POS_ACT_DCDP);	// Positive active demand in a current demand period(A+)
	OBIS_CODE_DEFINITION(01, 00, 01, 05, 00, ff, REG_POS_ACT_DLCDP);	// Positive active demand in the last completed demand period(A+)
	OBIS_CODE_DEFINITION(01, 00, 01, 06, 00, ff, REG_POS_ACT_MD);	// Positive active maximum demand(A+) total
	OBIS_CODE_DEFINITION(01, 00, 01, 06, 01, ff, REG_POS_ACT_MD_T1);	// Positive active maximum demand(A+) in tariff 1
	OBIS_CODE_DEFINITION(01, 00, 01, 06, 02, ff, REG_POS_ACT_MD_T2);	// Positive active maximum demand(A+) in tariff 2
	OBIS_CODE_DEFINITION(01, 00, 01, 06, 03, ff, REG_POS_ACT_MD_T3);	// Positive active maximum demand(A+) in tariff 3
	OBIS_CODE_DEFINITION(01, 00, 01, 06, 04, ff, REG_POS_ACT_MD_T4);	// Positive active maximum demand(A+) in tariff 4
	OBIS_CODE_DEFINITION(01, 00, 01, 07, 00, ff, REG_POS_ACT_IP);	// Positive active instantaneous power(A+)
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, 60, EDL21_LAST_24h);	// Consumption over the last 24 hours
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, 61, EDL21_LAST_7d);	// Consumption over the last 7 days
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, 62, EDL21_LAST_30d);	// Consumption over the last 30 days
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, 63, EDL21_LAST_365d);	// Consumption over the last 365 days
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, 64, EDL21_LAST_RESET);	// Consumption since last reset
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, ff, REG_POS_ACT_E);	// Positive active energy(A+), current value
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 01, ff, REG_POS_ACT_E_T1);	// Positive active energy(A+) in tariff T1
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 02, ff, REG_POS_ACT_E_T2);	// Positive active energy(A+) in tariff T2
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 03, ff, REG_POS_ACT_E_T3);	// Positive active energy(A+) in tariff T3
	OBIS_CODE_DEFINITION(01, 00, 01, 08, 04, ff, REG_POS_ACT_E_T4);	// Positive active energy(A+) in tariff T4
	OBIS_CODE_DEFINITION(01, 00, 02, 02, 00, ff, REG_NEG_ACT_CMD);	// Negative active cumulative maximum demand(A-) total
	OBIS_CODE_DEFINITION(01, 00, 02, 04, 00, ff, REG_NEG_ACT_DCDP);	// Negative active demand in a current demand period(A-)
	OBIS_CODE_DEFINITION(01, 00, 02, 05, 00, ff, REG_NEG_ACT_DLCDP);	// Negative active demand in the last completed demand period(A-)
	OBIS_CODE_DEFINITION(01, 00, 02, 06, 00, ff, REG_NEG_ACT_MD);	// Negative active maximum demand(A-) total
	OBIS_CODE_DEFINITION(01, 00, 02, 06, 01, ff, REG_NEG_ACT_MD_T1);	// Negative active maximum demand(A-) in tariff 1
	OBIS_CODE_DEFINITION(01, 00, 02, 06, 02, ff, REG_NEG_ACT_MD_T2);	// Negative active maximum demand(A-) in tariff 2
	OBIS_CODE_DEFINITION(01, 00, 02, 06, 03, ff, REG_NEG_ACT_MD_T3);	// Negative active maximum demand(A-) in tariff 3
	OBIS_CODE_DEFINITION(01, 00, 02, 06, 04, ff, REG_NEG_ACT_MD_T4);	// Negative active maximum demand(A-) in tariff 4
	OBIS_CODE_DEFINITION(01, 00, 02, 07, 00, ff, REG_NEG_ACT_IP);	// Negative active instantaneous power(A-)
	OBIS_CODE_DEFINITION(01, 00, 02, 08, 00, ff, REG_NEG_ACT_E);	// Negative active energy(A+) total
	OBIS_CODE_DEFINITION(01, 00, 02, 08, 01, ff, REG_NEG_ACT_E_T1);	// Negative active energy(A+) in tariff T1
	OBIS_CODE_DEFINITION(01, 00, 02, 08, 02, ff, REG_NEG_ACT_E_T2);	// Negative active energy(A+) in tariff T2
	OBIS_CODE_DEFINITION(01, 00, 02, 08, 03, ff, REG_NEG_ACT_E_T3);	// Negative active energy(A+) in tariff T3
	OBIS_CODE_DEFINITION(01, 00, 02, 08, 04, ff, REG_NEG_ACT_E_T4);	// Negative active energy(A+) in tariff T4
	OBIS_CODE_DEFINITION(01, 00, 03, 02, 00, ff, REG_PRCMD);	// Positive reactive cumulative maximum demand(Q+) total
	OBIS_CODE_DEFINITION(01, 00, 03, 04, 00, ff, REG_PRDCDP);	// Positive reactive demand in a current demand period(Q+)
	OBIS_CODE_DEFINITION(01, 00, 03, 05, 00, ff, REG_RDLCDP);	// Positive reactive demand in the last completed demand period(Q+)
	OBIS_CODE_DEFINITION(01, 00, 03, 06, 00, ff, REG_PRMD);	// Positive reactive maximum demand(Q+) total
	OBIS_CODE_DEFINITION(01, 00, 03, 07, 00, ff, REG_PRIP);	// Positive reactive instantaneous power(Q+)
	OBIS_CODE_DEFINITION(01, 00, 03, 08, 00, ff, REG_PRE);	// Positive reactive energy(Q+) total
	OBIS_CODE_DEFINITION(01, 00, 04, 02, 00, ff, REG_NRCMD);	// Negative reactive cumulative maximum demand(Q-) total
	OBIS_CODE_DEFINITION(01, 00, 04, 04, 00, ff, REG_NRDCDP);	// Negative reactive demand in a current demand period(Q-)
	OBIS_CODE_DEFINITION(01, 00, 04, 05, 00, ff, REG_NRDLCDP);	// Negative reactive demand in the last completed demand period(Q-)
	OBIS_CODE_DEFINITION(01, 00, 04, 06, 00, ff, REG_NRMD);	// Negative reactive maximum demand(Q-) total
	OBIS_CODE_DEFINITION(01, 00, 04, 07, 00, ff, REG_NRIP);	// Negative reactive instantaneous power(Q-)
	OBIS_CODE_DEFINITION(01, 00, 04, 08, 00, ff, REG_NRE);	// Negative reactive energy(Q-) total
	OBIS_CODE_DEFINITION(01, 00, 05, 02, 00, ff, REG_RCMDQ1);	// Reactive cumulative maximum demand in Q1(Q1) total
	OBIS_CODE_DEFINITION(01, 00, 05, 04, 00, ff, REG_RDCDPQ1);	// Reactive demand in a current demand period in Q1(Q1)
	OBIS_CODE_DEFINITION(01, 00, 05, 05, 00, ff, REG_RDLCDPQ1);	// Reactive demand in the last completed demand period in Q1(Q1)
	OBIS_CODE_DEFINITION(01, 00, 05, 06, 00, ff, REG_RMDQ1);	// Reactive maximum demand in Q1(Q1) total
	OBIS_CODE_DEFINITION(01, 00, 05, 08, 00, ff, REG_IIRE);	// Imported inductive reactive energy in 1 - st quadrant(Q1) total
	OBIS_CODE_DEFINITION(01, 00, 06, 02, 00, ff, REG_RCMDQ2);	// Reactive cumulative maximum demand in Q2(Q2) total
	OBIS_CODE_DEFINITION(01, 00, 06, 04, 00, ff, REG_RDCDPQ2);	// Reactive demand in a current demand period in Q2(Q2)
	OBIS_CODE_DEFINITION(01, 00, 06, 05, 00, ff, REG_RDLCDPQ2);	// Reactive demand in the last completed demand period in Q2(Q2)
	OBIS_CODE_DEFINITION(01, 00, 06, 06, 00, ff, REG_RMDQ2);	// Reactive maximum demand in Q2(Q2) total
	OBIS_CODE_DEFINITION(01, 00, 06, 08, 00, ff, REG_ICRE);	// Imported capacitive reactive energy in 2 - nd quadrant(Q2) total
	OBIS_CODE_DEFINITION(01, 00, 07, 02, 00, ff, REG_RCMDQ3);	// Reactive cumulative maximum demand in Q3(Q3) total
	OBIS_CODE_DEFINITION(01, 00, 07, 04, 00, ff, REG_RDCDPQ3);	// Reactive demand in a current demand period in Q3(Q3)
	OBIS_CODE_DEFINITION(01, 00, 07, 05, 00, ff, REG_RDLCDPQ3);	// Reactive demand in the last completed demand period in Q3(Q3)
	OBIS_CODE_DEFINITION(01, 00, 07, 06, 00, ff, REG_RMDQ3);	// Reactive maximum demand in Q3(Q3) total
	OBIS_CODE_DEFINITION(01, 00, 07, 08, 00, ff, REG_EIRE);	// Exported inductive reactive energy in 3 - rd quadrant(Q3) total
	OBIS_CODE_DEFINITION(01, 00, 08, 02, 00, ff, REG_RCMDQ4);	// Reactive cumulative maximum demand in Q4(Q4) total
	OBIS_CODE_DEFINITION(01, 00, 08, 04, 00, ff, REG_RDCDPQ4);	// Reactive demand in a current demand period in Q4(Q4)
	OBIS_CODE_DEFINITION(01, 00, 08, 05, 00, ff, REG_RDLCDPQ4);	// Reactive demand in the last completed demand period in Q4(Q4)
	OBIS_CODE_DEFINITION(01, 00, 08, 06, 00, ff, REG_RMDQ4);	// Reactive maximum demand in Q4(Q4) total
	OBIS_CODE_DEFINITION(01, 00, 08, 08, 00, ff, REG_ECRE);	// Exported capacitive reactive energy in 4 - th quadrant(Q4) total
	OBIS_CODE_DEFINITION(01, 00, 09, 02, 00, ff, REG_RCMDT);	// Apparent cumulative maximum demand(S+) total
	OBIS_CODE_DEFINITION(01, 00, 09, 04, 00, ff, REG_RDCDPT);	// Apparent demand in a current demand period(S+)
	OBIS_CODE_DEFINITION(01, 00, 09, 05, 00, ff, REG_ADLCDP);	// Apparent demand in the last completed demand period(S+)
	OBIS_CODE_DEFINITION(01, 00, 09, 06, 00, ff, REG_AMDT);	// Apparent maximum demand(S+) total
	OBIS_CODE_DEFINITION(01, 00, 09, 06, 01, ff, REG_AMDT_T1);	// Apparent maximum demand(S+) in tariff 1
	OBIS_CODE_DEFINITION(01, 00, 09, 06, 02, ff, REG_AMDT_T2);	// Apparent maximum demand(S+) in tariff 2
	OBIS_CODE_DEFINITION(01, 00, 09, 06, 03, ff, REG_AMDT_T3);	// Apparent maximum demand(S+) in tariff 3
	OBIS_CODE_DEFINITION(01, 00, 09, 06, 04, ff, REG_AMDT_T4);	// Apparent maximum demand(S+) in tariff 4
	OBIS_CODE_DEFINITION(01, 00, 09, 07, 00, ff, REG_AIP);	// Apparent instantaneous power(S+)
	OBIS_CODE_DEFINITION(01, 00, 09, 08, 00, ff, REG_AE);	// Apparent energy(S+) total
	OBIS_CODE_DEFINITION(01, 00, 0a, 06, 01, ff, REG_TODO_T1);	// Instantaneous apparent power average (S+) in tariff 1
	OBIS_CODE_DEFINITION(01, 00, 0a, 06, 02, ff, REG_TODO_T2);	// Instantaneous apparent power average (S+) in tariff 2
	OBIS_CODE_DEFINITION(01, 00, 0a, 06, 03, ff, REG_TODO_T3);	// Instantaneous apparent power average (S+) in tariff 3
	OBIS_CODE_DEFINITION(01, 00, 0a, 06, 04, ff, REG_TODO_T4);	// Instantaneous apparent power average (S+) in tariff 4
	OBIS_CODE_DEFINITION(01, 00, 0a, 08, 00, ff, REG_AED);	// Apparent energy total - Delivery (10.8.0)
	OBIS_CODE_DEFINITION(01, 00, 0a, 08, 01, ff, REG_AED_T1);	// Apparent energy tariff 1 - Delivery (10.8.1)
	OBIS_CODE_DEFINITION(01, 00, 0a, 08, 02, ff, REG_AED_T2);	// Apparent energy tariff 2 - Delivery (10.8.2)
	OBIS_CODE_DEFINITION(01, 00, 0a, 08, 03, ff, REG_AED_T3);	// Apparent energy tariff 3 - Delivery (10.8.3)
	OBIS_CODE_DEFINITION(01, 00, 0a, 08, 04, ff, REG_AED_T4);	// Apparent energy tariff 4 - Delivery (10.8.4)
	OBIS_CODE_DEFINITION(01, 00, 0b, 06, 00, ff, REG_MC);	// Maximum current(I max)
	OBIS_CODE_DEFINITION(01, 00, 0b, 07, 00, ff, REG_IC);	// Instantaneous current(I)
	OBIS_CODE_DEFINITION(01, 00, 0c, 07, 00, ff, REG_IV);	// Instantaneous voltage(U)
	OBIS_CODE_DEFINITION(01, 00, 0d, 07, 00, ff, REG_IPF);	// Instantaneous power factor
	OBIS_CODE_DEFINITION(01, 00, 0e, 07, 00, ff, REG_FREQ);	// Frequency
	OBIS_CODE_DEFINITION(01, 00, 0f, 02, 00, ff, REG_AACMD);	// Absolute active cumulative maximum demand(|A| ) total
	OBIS_CODE_DEFINITION(01, 00, 0f, 04, 00, ff, REG_AADCDP);	// Absolute active demand in a current demand period(|A| )
	OBIS_CODE_DEFINITION(01, 00, 0f, 05, 00, ff, REG_AADLCDP);	// Absolute active demand in the last completed demand period(|A| )
	OBIS_CODE_DEFINITION(01, 00, 0f, 06, 00, ff, REG_AAMD);	// Absolute active maximum demand(|A| ) total
	OBIS_CODE_DEFINITION(01, 00, 0f, 07, 00, ff, REG_AAIP);	// Absolute active instantaneous power(|A|)
	OBIS_CODE_DEFINITION(01, 00, 0f, 08, 00, ff, REG_AAE);	// Absolute active energy(A+) total
	OBIS_CODE_DEFINITION(01, 00, 10, 07, 00, ff, REG_SIP);	// Sum active instantaneous power(A+ - A-)
	OBIS_CODE_DEFINITION(01, 00, 10, 08, 00, ff, REG_SAE);	// Sum active energy without reverse blockade(A + -A - ) total
	OBIS_CODE_DEFINITION(01, 00, 15, 07, 00, ff, REG_POS_ACT_INST_PPOW_L1);	// Positive active instantaneous power(A+) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 15, 08, 00, ff, REG_POS_ACT_EL1);	// Positive active energy(A+) in phase L1 total
	OBIS_CODE_DEFINITION(01, 00, 16, 07, 00, ff, REG_NEG_ACT_IPL1);	// Negative active instantaneous power(A-) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 16, 08, 00, ff, REG_NEG_ACT_EL1T);	// Negative active energy(A-) in phase L1 total
	OBIS_CODE_DEFINITION(01, 00, 17, 07, 00, ff, REG_PRIPL1);	// Positive reactive instantaneous power(Q+) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 18, 07, 00, ff, REG_NRIPL1);	// Negative reactive instantaneous power(Q-) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 1d, 07, 00, ff, REG_AIPL1);	// Apparent instantaneous power(S+) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 1f, 06, 00, ff, REG_MCL1);	// Maximum current(I max) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 1f, 07, 00, ff, REG_ICL1);	// Instantaneous current(I) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 20, 07, 00, ff, REG_IVL1);	// Instantaneous voltage(U) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 21, 07, 00, ff, REG_IPFL1);	// Instantaneous power factor in phase L1
	OBIS_CODE_DEFINITION(01, 00, 23, 07, 00, ff, REG_AAIPL1);	// Absolute active instantaneous power(|A|) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 23, 08, 00, ff, REG_AAEL1);	// Absolute active energy(|A|) in phase L1 total
	OBIS_CODE_DEFINITION(01, 00, 24, 07, 00, ff, REG_SIPL1);	// Sum active instantaneous power(A + - A-) in phase L1
	OBIS_CODE_DEFINITION(01, 00, 29, 07, 00, ff, REG_POS_ACT_INST_PPOW_L2);	// Positive active instantaneous power(A+) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 29, 08, 00, ff, REG_POS_ACT_EL2);	// Positive active energy(A+) in phase L2 total
	OBIS_CODE_DEFINITION(01, 00, 2a, 07, 00, ff, REG_NEG_ACT_IPL2);	// Negative active instantaneous power(A-) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 2a, 08, 00, ff, REG_NEG_ACT_EL2T);	// Negative active energy(A-) in phase L2 total
	OBIS_CODE_DEFINITION(01, 00, 2b, 07, 00, ff, REG_PRIPL2);	// Positive reactive instantaneous power(Q+) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 2c, 07, 00, ff, REG_NRIPL2);	// Negative reactive instantaneous power(Q-) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 31, 07, 00, ff, REG_AIPL2);	// Apparent instantaneous power(S+) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 33, 06, 00, ff, REG_MCL2);	// Maximum current(I max) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 33, 07, 00, ff, REG_ICL2);	// Instantaneous current(I) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 34, 07, 00, ff, REG_IVL2);	// Instantaneous voltage(U) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 35, 07, 00, ff, REG_IPFL2);	// Instantaneous power factor in phase L2
	OBIS_CODE_DEFINITION(01, 00, 37, 07, 00, ff, REG_AAIPL2);	// Absolute active instantaneous power(|A|) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 37, 08, 00, ff, REG_AAEL2);	// Absolute active energy(|A|) in phase L2 total
	OBIS_CODE_DEFINITION(01, 00, 38, 07, 00, ff, REG_SIPL2);	// Sum active instantaneous power(A + - A-) in phase L2
	OBIS_CODE_DEFINITION(01, 00, 3d, 07, 00, ff, REG_PPOS_ACT_INST_PPOW_L3);	// Positive active instantaneous power(A+) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 3d, 08, 00, ff, REG_POS_ACT_EL1T);	// Positive active energy(A+) in phase L3 total
	OBIS_CODE_DEFINITION(01, 00, 3d, 61, 00, ff, REG_FATAL_ERROR_METER_STATUS);	// Fatal error meter status
	OBIS_CODE_DEFINITION(01, 00, 3e, 07, 00, ff, REG_NEG_ACT_IPL3);	// Negative active instantaneous power(A-) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 3e, 08, 00, ff, REG_NEG_ACT_EL3T);	// Negative active energy(A-) in phase L3 total
	OBIS_CODE_DEFINITION(01, 00, 3f, 07, 00, ff, REG_PRIPL3);	// Positive reactive instantaneous power(Q+) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 40, 07, 00, ff, REG_NRIPL3);	// Negative reactive instantaneous power(Q-) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 45, 07, 00, ff, REG_AIPL3);	// Apparent instantaneous power(S+) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 47, 06, 00, ff, REG_MCL3);	// Maximum current(I max) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 47, 07, 00, ff, REG_ICL3);	// Instantaneous current(I) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 48, 07, 00, ff, REG_IVL3);	// Instantaneous voltage(U) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 49, 07, 00, ff, REG_IPFL3);	// Instantaneous power factor in phase L3
	OBIS_CODE_DEFINITION(01, 00, 4b, 07, 00, ff, REG_AAIPL3);	// Absolute active instantaneous power(|A|) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 4b, 08, 00, ff, REG_AAEL3);	// Absolute active energy(|A|) in phase L3 total
	OBIS_CODE_DEFINITION(01, 00, 4c, 07, 00, ff, REG_SIPL3);	// Sum active instantaneous power(A + - A-) in phase L3
	OBIS_CODE_DEFINITION(01, 00, 5b, 06, 00, ff, REG_MCN);	// Maximum current(I max) in neutral
	OBIS_CODE_DEFINITION(01, 00, 5b, 07, 00, ff, REG_ICN);	// Instantaneous current(I) in neutral
	OBIS_CODE_DEFINITION(01, 00, 60, 02, 00, ff, REG_EVT_PARAM_CHANGED_COUNTER);	// Event parameters change
	OBIS_CODE_DEFINITION(01, 00, 60, 02, 01, ff, REG_EVT_PARAM_CHANGED_TIMESTAMP);	// Event parameters change
	OBIS_CODE_DEFINITION(01, 00, 60, 07, 00, ff, REG_EVT_POWER_DOWN_COUNTER);	// Event power down
	OBIS_CODE_DEFINITION(01, 00, 60, 07, 0a, ff, REG_EVT_POWER_DOWN_TIMESTAMP);	// Event power down
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 01, ff, REG_EVT_1);	// Event terminal cover opened
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 02, ff, REG_EVT_2);	// Event terminal cover opened
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 03, ff, REG_EVT_3);	// Event main cover opened - counter
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 04, ff, REG_EVT_4);	// Event main cover opened 
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 05, ff, REG_EVT_5);	// Event magnetic field detection start - counter
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 06, ff, REG_EVT_6);	// Event magnetic field detection start
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 07, ff, REG_EVT_7);	// Event reverse power flow
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 08, ff, REG_EVT_8);	// Event reverse power flow
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 0d, ff, REG_EVT_13);	// Event power up
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 0e, ff, REG_EVT_14);	// Event power up
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 0f, ff, REG_EVT_15);	// Event RTC(Real Time Clock) set
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 10, ff, REG_EVT_16);	// Event RTC(Real Time Clock) set
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 15, ff, REG_EVT_21);	// Event terminal cover closed
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 16, ff, REG_EVT_22);	// Event terminal cover closed
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 17, ff, REG_EVT_23);	// Event main cover closed
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 18, ff, REG_EVT_24);	// Event main cover closed
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 19, ff, REG_EVT_25);	// Event log - book 1 erased
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 1a, ff, REG_EVT_26);	// Event log - book 1 erased
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 1b, ff, REG_EVT_27);	// Event fraud start
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 1c, ff, REG_EVT_28);	// Event fraud start
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 1d, ff, REG_EVT_29);	// Event fraud stop
	OBIS_CODE_DEFINITION(01, 00, 60, 33, 1e, ff, REG_EVT_30);	// Event fraud stop
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 01, ff, REG_TAMPER_ENERGY_1);	// Tamper 1 energy
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 02, ff, REG_TAMPER_ENERGY_2);	// Tamper 2 energy
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 03, ff, REG_TAMPER_ENERGY_3);	// Tamper 3 energy
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 04, ff, REG_TAMPER_ENERGY_4);	// Tamper 4 energy
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 05, ff, REG_TAMPER_TIME_1);	// Tamper 1 time counter register
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 06, ff, REG_TAMPER_TIME_2);	// Tamper 2 time counter register
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 07, ff, REG_TAMPER_TIME_3);	// Tamper 3 time counter register
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 09, ff, REG_TAMPER_TIME_4);	// Tamper 4 time counter register
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 0a, ff, REG_TAMPER_TIME_5);	// Tamper 5 time counter register
	OBIS_CODE_DEFINITION(01, 00, 60, 35, 0b, ff, REG_TAMPER_ENERGY_5);	// Tamper 5 energy
	OBIS_CODE_DEFINITION(01, 00, 60, 57, 00, ff, REG_ACTIVE_TARFIFF);	// Active tariff
	OBIS_CODE_DEFINITION(01, 00, 60, 60, 09, ff, REG_FRAUD_FLAG);	// Fraud flag
	OBIS_CODE_DEFINITION(01, 01, 00, 04, 02, ff, CURRENT_TRANSFORMER_RATIO);	// Current transformer ratio (L&G)
	OBIS_CODE_DEFINITION(01, 01, 00, 04, 03, ff, VOLTAGE_TRANSFORMER_RATIO);	// Voltage transformer ratio (L&G)
	OBIS_CODE_DEFINITION(01, 01, 01, 1d, 00, ff, PROFILE_POWER_POS_ACTIVE);	// d - Load profile (+A) Active energy import
	OBIS_CODE_DEFINITION(01, 01, 02, 1d, 00, ff, PROFILE_POWER_NEG_ACTIVE);	// d - Load profile (-A) Active energy export
	OBIS_CODE_DEFINITION(01, 01, 05, 1d, 00, ff, PROFILE_REACTIVE_Q1);	// d - Load profile Reactive Q1
	OBIS_CODE_DEFINITION(01, 01, 06, 1d, 00, ff, PROFILE_REACTIVE_Q2);	// d - Load profile Reactive Q2
	OBIS_CODE_DEFINITION(01, 01, 07, 1d, 00, ff, PROFILE_REACTIVE_Q3);	// d - Load profile Reactive Q3
	OBIS_CODE_DEFINITION(01, 01, 08, 1d, 00, ff, PROFILE_REACTIVE_Q4);	// d - Load profile Reactive Q4
	OBIS_CODE_DEFINITION(01, 01, 62, 17, 00, ff, REG_SM_POWER_THRESHOLD);	// Power threshold (D.23.0)
	// #286
	// Heat Cost Allocators
	OBIS_CODE_DEFINITION(04, 00, 00, 01, 0a, ff, REG_HCA_10);	// Local date at set date
	OBIS_CODE_DEFINITION(04, 00, 00, 09, 01, ff, REG_HCA_F);	// Current time at time of transmission
	OBIS_CODE_DEFINITION(04, 00, 00, 09, 02, ff, REG_HCA_G);	// Current date at time of transmission
	OBIS_CODE_DEFINITION(04, 00, 01, 00, 01, ff, REG_HCA_0);	// Unrated integral, current value
	OBIS_CODE_DEFINITION(04, 00, 01, 02, 00, ff, REG_HCA_2);	// Unrated integral, set date value
	// #291
	// Cooling
	OBIS_CODE_DEFINITION(05, 01, 01, 1d, 00, ff, PROFILE_COLD);	// d - Load profile Cold
	// #292
	// Heat
	OBIS_CODE_DEFINITION(06, 00, 00, 08, 00, ff, REG_HEAT_AED);	// Power (energy flow) (P ), average, current value
	OBIS_CODE_DEFINITION(06, 00, 01, 00, 00, ff, REG_HEAT_CURRENT);	// Energy (A), total, current value
	OBIS_CODE_DEFINITION(06, 00, 01, 02, 00, ff, REG_HEAT_SET_DATE);	// Energy (A), total, set date value
	OBIS_CODE_DEFINITION(06, 01, 01, 1d, 00, ff, PROFILE_HEAT);	// d - Load profile Heat
	OBIS_CODE_DEFINITION(06, 01, 1f, 1d, 00, ff, PROFILE_HEAT_POS_OUTPUT);	// d - Load profile Heat (+E) Energy output
	// #297
	// Gas
	OBIS_CODE_DEFINITION(07, 00, 00, 08, 1c, ff, REG_GAS_INT);	// Averaging duration for actual flow rate value
	OBIS_CODE_DEFINITION(07, 00, 00, 2a, 02, ff, REG_GAS_BP);	// defined Pressure, absolute, at base conditions
	OBIS_CODE_DEFINITION(07, 00, 03, 00, 00, ff, REG_GAS_MC_0_0);	// Volume (meter), 
	OBIS_CODE_DEFINITION(07, 00, 03, 01, 00, ff, REG_GAS_MC_1_0);	// Volume (meter), temperature converted (Vtc), forward, absolute, current value
	OBIS_CODE_DEFINITION(07, 00, 03, 02, 00, ff, REG_GAS_MC_2_0);	// Volume (meter), base conditions (Vb), forward, absolute, current value
	OBIS_CODE_DEFINITION(07, 00, 2b, 0f, 00, ff, REG_GAS_FR_15);	// Flow rate at measuring conditions, averaging period 1 (default period = 5 min), current interval 
	OBIS_CODE_DEFINITION(07, 00, 2b, 10, 00, ff, REG_GAS_FR_16);	// Flow rate, temperature converted, averaging period 1(default period = 5 min), current interval
	OBIS_CODE_DEFINITION(07, 00, 2b, 11, 00, ff, REG_GAS_FR_17);	// Flow rate at base conditions, averaging period 1 (default period = 5 min), current interval
	OBIS_CODE_DEFINITION(07, 01, 0b, 1d, 00, ff, PROFILE_GAS_POS_OUTPUT);	// d - Load profile (+Vb) Operating volume Uninterrupted delivery
	// #306
	// Water (cold)
	OBIS_CODE_DEFINITION(08, 00, 01, 00, 00, ff, WATER_CURRENT);	// Volume (V), accumulated, total, current value
	OBIS_CODE_DEFINITION(08, 00, 01, 02, 00, ff, WATER_DATE);	// Volume (V), accumulated, total, due date value
	OBIS_CODE_DEFINITION(08, 00, 02, 00, 00, ff, WATER_FLOW_RATE);	// Flow rate, average (Va/t), current value 
	OBIS_CODE_DEFINITION(08, 01, 01, 1d, 00, ff, PROFILE_WATER_POS_OUTPUT);	// d - Load profile (+Vol) Volume output
	// #310
	// Water (hot)
	OBIS_CODE_DEFINITION(09, 01, 01, 1d, 00, ff, PROFILE_HOT_WATER);	// d - Load profile Hot Water
	// #311
	// next group
	OBIS_CODE_DEFINITION(81, 00, 00, 09, 0b, 00, ACT_SENSOR_TIME);	// actSensorTime - current UTC time
	OBIS_CODE_DEFINITION(81, 00, 00, 09, 0b, 01, TZ_OFFSET);	// u16 - offset to actual time zone in minutes (-720 .. +720)
	OBIS_CODE_DEFINITION(81, 00, 60, 05, 00, 00, CLASS_OP_LOG_STATUS_WORD);
	OBIS_CODE_DEFINITION(81, 01, 00, 00, 00, ff, LOG_SOURCE_ETH_AUX);
	OBIS_CODE_DEFINITION(81, 01, 00, 00, 01, 00, INTERFACE_01_NAME);	// s - interface name
	OBIS_CODE_DEFINITION(81, 02, 00, 00, 00, ff, LOG_SOURCE_ETH_CUSTOM);
	OBIS_CODE_DEFINITION(81, 02, 00, 00, 01, 00, INTERFACE_02_NAME);	// s - interface name
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 00, ff, ROOT_CUSTOM_INTERFACE);	// see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 01, ff, CUSTOM_IF_IP_REF);	// u8 - 0 == manual, 1 == DHCP
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 01, CUSTOM_IF_DHCP_LOCAL_IP_MASK);	// ip:address
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 02, CUSTOM_IF_DHCP_DEFAULT_GW);	// ip:address
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 03, CUSTOM_IF_DHCP_DNS);	// ip:address
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 04, CUSTOM_IF_DHCP_START_ADDRESS);	// ip:address
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, 05, CUSTOM_IF_DHCP_END_ADDRESS);	// ip:address
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 02, ff, CUSTOM_IF_DHCP);	// bool - if true use a DHCP server
	OBIS_CODE_DEFINITION(81, 02, 00, 07, 10, ff, ROOT_CUSTOM_PARAM);	// see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle 
	OBIS_CODE_DEFINITION(81, 02, 17, 07, 00, 00, CUSTOM_IF_IP_CURRENT_1);	// ip:address - current IP address (customer interface)
	OBIS_CODE_DEFINITION(81, 02, 17, 07, 00, 01, CUSTOM_IF_IP_ADDRESS_1);	// ip:address - first manual set IP address
	OBIS_CODE_DEFINITION(81, 02, 17, 07, 00, 02, CUSTOM_IF_IP_ADDRESS_2);	// ip:address - second manual set IP address
	OBIS_CODE_DEFINITION(81, 02, 17, 07, 01, 01, CUSTOM_IF_IP_MASK_1);	// ip:address
	OBIS_CODE_DEFINITION(81, 02, 17, 07, 01, 02, CUSTOM_IF_IP_MASK_2);	// ip:address
	OBIS_CODE_DEFINITION(81, 03, 00, 00, 00, ff, LOG_SOURCE_RS232);
	OBIS_CODE_DEFINITION(81, 03, 00, 00, 01, 00, INTERFACE_03_NAME);	// s - interface name
	OBIS_CODE_DEFINITION(81, 04, 00, 00, 00, ff, LOG_SOURCE_ETH);	// WAN interface
	OBIS_CODE_DEFINITION(81, 04, 00, 00, 01, 00, INTERFACE_04_NAME);	// s - interface name
	OBIS_CODE_DEFINITION(81, 04, 00, 06, 00, ff, ROOT_WAN);	// see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status 
	OBIS_CODE_DEFINITION(81, 04, 00, 07, 00, ff, ROOT_WAN_PARAM);	// see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter 
	OBIS_CODE_DEFINITION(81, 04, 02, 07, 00, ff, ROOT_GSM);	// see: Datenstruktur zum Lesen/Setzen der GSM Parameter 
	OBIS_CODE_DEFINITION(81, 04, 0d, 06, 00, 00, CLASS_OP_LOG_PROVIDER);	// u32 - 	aktueller Provider-Identifier(uint32)
	OBIS_CODE_DEFINITION(81, 04, 0d, 06, 00, ff, GSM_ADMISSIBLE_OPERATOR);
	OBIS_CODE_DEFINITION(81, 04, 0d, 07, 00, ff, ROOT_GPRS_PARAM);	// see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter 
	OBIS_CODE_DEFINITION(81, 04, 0d, 08, 00, ff, ROOT_GSM_STATUS);
	OBIS_CODE_DEFINITION(81, 04, 0e, 06, 00, ff, PLC_STATUS);
	OBIS_CODE_DEFINITION(81, 04, 17, 07, 00, 00, CLASS_OP_LOG_AREA_CODE);	// u16 - aktueller Location - oder Areacode
	OBIS_CODE_DEFINITION(81, 04, 18, 07, 00, ff, IF_PLC);
	OBIS_CODE_DEFINITION(81, 04, 1a, 07, 00, 00, CLASS_OP_LOG_CELL);	// u16 - aktuelle Zelleninformation
	OBIS_CODE_DEFINITION(81, 04, 2b, 07, 00, 00, CLASS_OP_LOG_FIELD_STRENGTH);
	OBIS_CODE_DEFINITION(81, 04, 62, 3c, 01, 01, PPPoE_USERNAME);	// s - PPPoE username
	OBIS_CODE_DEFINITION(81, 04, 62, 3c, 02, 01, PPPoE_PASSWORD);	// s - PPPoE passphrase
	OBIS_CODE_DEFINITION(81, 04, 62, 3c, 03, 01, PPPoE_MODE);	// u8 - PPPoE mode
	OBIS_CODE_DEFINITION(81, 05, 00, 00, 00, ff, LOG_SOURCE_eHZ);
	OBIS_CODE_DEFINITION(81, 05, 0d, 07, 00, 01, IF_EDL_PROTOCOL);	// u8 - always 1
	OBIS_CODE_DEFINITION(81, 05, 0d, 07, 00, 02, IF_EDL_BAUDRATE);	// 0 = auto, 6 = 9600, 10 = 115200 baud
	OBIS_CODE_DEFINITION(81, 05, 0d, 07, 00, ff, IF_EDL);	// M-Bus EDL (RJ10)
	OBIS_CODE_DEFINITION(81, 06, 00, 00, 00, ff, LOG_SOURCE_wMBUS);
	OBIS_CODE_DEFINITION(81, 06, 00, 00, 01, 00, W_MBUS_ADAPTER_MANUFACTURER);
	OBIS_CODE_DEFINITION(81, 06, 00, 00, 03, 00, W_MBUS_ADAPTER_ID);
	OBIS_CODE_DEFINITION(81, 06, 00, 02, 00, 00, W_MBUS_FIRMWARE);	// s
	OBIS_CODE_DEFINITION(81, 06, 00, 02, 03, ff, W_MBUS_HARDWARE);	// s
	OBIS_CODE_DEFINITION(81, 06, 00, 02, 04, ff, W_MBUS_FIELD_STRENGTH);	// u16 - dbm
	OBIS_CODE_DEFINITION(81, 06, 00, 03, 74, ff, W_MBUS_LAST_RECEPTION);	// Time since last radio telegram reception
	OBIS_CODE_DEFINITION(81, 06, 0f, 06, 00, ff, ROOT_W_MBUS_STATUS);	// see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
	OBIS_CODE_DEFINITION(81, 06, 19, 07, 00, ff, ROOT_W_MBUS_PARAM);	// Wireless M-BUS
	OBIS_CODE_DEFINITION(81, 06, 19, 07, 01, ff, W_MBUS_PROTOCOL);
	OBIS_CODE_DEFINITION(81, 06, 19, 07, 02, ff, W_MBUS_MODE_S);
	OBIS_CODE_DEFINITION(81, 06, 19, 07, 03, ff, W_MBUS_MODE_T);
	OBIS_CODE_DEFINITION(81, 06, 19, 07, 04, ff, W_MBUS_POWER);	// u8 - 0 = default, 1 = low, 2 = medium, 3 = high
	OBIS_CODE_DEFINITION(81, 06, 19, 07, 11, ff, W_MBUS_INSTALL_MODE);
	OBIS_CODE_DEFINITION(81, 06, 27, 32, 03, 01, W_MBUS_REBOOT);	// u32
	OBIS_CODE_DEFINITION(81, 06, 2b, 07, 00, 00, W_MBUS_FIELD_STRENGTH_2);	// u8 - Receive field strength
	OBIS_CODE_DEFINITION(81, 06, 64, 3c, 01, 01, W_MBUS_MAX_MSG_TIMEOUT);	// u8
	OBIS_CODE_DEFINITION(81, 06, 64, 3c, 01, 04, W_MBUS_MAX_SML_TIMEOUT_IN);	// u16 - max timeout between SML close request and SML open response from device to gateway
	OBIS_CODE_DEFINITION(81, 06, 64, 3c, 01, 05, W_MBUS_MAX_SML_TIMEOUT_OUT);	// u16 - max timeout between SML close request and SML open response from gateway to device
	OBIS_CODE_DEFINITION(81, 41, 00, 00, 00, ff, LOG_SOURCE_IP);
	OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 01, LOG_SOURCE_SML_EXT);
	OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 02, LOG_SOURCE_SML_CUSTOM);
	OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 03, LOG_SOURCE_SML_SERVICE);
	OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 04, LOG_SOURCE_SML_WAN);
	OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 05, LOG_SOURCE_SML_eHZ);
	OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 06, LOG_SOURCE_SML_wMBUS);
	OBIS_CODE_DEFINITION(81, 45, 00, 00, 00, ff, LOG_SOURCE_PUSH_SML);
	OBIS_CODE_DEFINITION(81, 46, 00, 00, 00, ff, LOG_SOURCE_PUSH_IPT_SOURCE);
	OBIS_CODE_DEFINITION(81, 46, 00, 00, 02, ff, PROFILE_ADDRESS);
	OBIS_CODE_DEFINITION(81, 47, 00, 00, 00, ff, LOG_SOURCE_PUSH_IPT_SINK);
	OBIS_CODE_DEFINITION(81, 47, 17, 07, 00, ff, PUSH_TARGET);	// push target name
	OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 00, COMPUTER_NAME);
	OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 01, LOG_SOURCE_WAN_DHCP);
	OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 02, LOG_SOURCE_WAN_IP);
	OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 03, LOG_SOURCE_WAN_PPPoE);
	OBIS_CODE_DEFINITION(81, 48, 00, 32, 01, 01, LAN_PPPoE_ENABLED);	// bool
	OBIS_CODE_DEFINITION(81, 48, 00, 32, 02, 01, LAN_DHCP_ENABLED);	// bool
	OBIS_CODE_DEFINITION(81, 48, 00, 32, 03, 01, LAN_DSL_ENABLED);	// bool - true = DLS, false = LAN
	OBIS_CODE_DEFINITION(81, 48, 0d, 06, 00, ff, ROOT_LAN_DSL);	// see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 00, 00, CODE_IF_LAN_ADDRESS);	// ip:address - IPv4 or IPv6 address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 00, 01, CODE_IF_DSL_ADDRESS);	// ip:address - IPv4 or IPv6 address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 00, ff, IF_LAN_DSL);	// see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 01, 00, CODE_IF_LAN_SUBNET_MASK);	// ip:address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 02, 00, CODE_IF_LAN_GATEWAY);	// ip:address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 04, 00, CODE_IF_LAN_DNS_PRIMARY);	// ip:address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 04, 01, CODE_IF_LAN_DSL_PRIMARY);	// ip:address - set/get OBIS_IF_LAN_DSL
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 05, 00, CODE_IF_LAN_DNS_SECONDARY);	// ip:address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 05, 01, CODE_IF_LAN_DSL_SECONDARY);	// ip:address - set/get OBIS_IF_LAN_DSL
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 06, 00, CODE_IF_LAN_DNS_TERTIARY);	// ip:address
	OBIS_CODE_DEFINITION(81, 48, 17, 07, 06, 01, CODE_IF_LAN_DLS_TERTIARY);	// ip:address - set/get OBIS_IF_LAN_DSL
	OBIS_CODE_DEFINITION(81, 48, 27, 32, 06, 01, TCP_WAIT_TO_RECONNECT);	// u8
	OBIS_CODE_DEFINITION(81, 48, 31, 32, 02, 01, TCP_CONNECT_RETRIES);	// u32
	OBIS_CODE_DEFINITION(81, 48, 31, 32, 07, 01, TCP_REPLY_ICMP);	// bool - reply to received ICMP packages
	OBIS_CODE_DEFINITION(81, 49, 00, 00, 00, 01, LOG_SOURCE_WAN_IPT_CONTROLLER);
	OBIS_CODE_DEFINITION(81, 49, 00, 00, 00, 02, LOG_SOURCE_WAN_IPT);
	OBIS_CODE_DEFINITION(81, 49, 00, 00, 10, ff, PUSH_SERVICE);	// options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX
	OBIS_CODE_DEFINITION(81, 49, 0d, 06, 00, ff, ROOT_IPT_STATE);	// see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status 
	OBIS_CODE_DEFINITION(81, 49, 0d, 07, 00, ff, ROOT_IPT_PARAM);	// see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter 
	OBIS_CODE_DEFINITION(81, 49, 17, 07, 00, 00, TARGET_IP_ADDRESS);	// ip:address - ip address of IP-T master 
	OBIS_CODE_DEFINITION(81, 49, 19, 07, 00, 00, SOURCE_PORT);	// u16 - target port of IP-T master 
	OBIS_CODE_DEFINITION(81, 49, 1a, 07, 00, 00, TARGET_PORT);	// u16 - source port of IP-T master 
	OBIS_CODE_DEFINITION(81, 49, 63, 3c, 01, 00, IPT_ACCOUNT);	// s
	OBIS_CODE_DEFINITION(81, 49, 63, 3c, 02, 00, IPT_PASSWORD);	// s
	OBIS_CODE_DEFINITION(81, 49, 63, 3c, 03, 00, IPT_SCRAMBLED);
	OBIS_CODE_DEFINITION(81, 4a, 00, 00, 00, ff, LOG_SOURCE_WAN_NTP);
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 01, PEER_OBISLOG);	// peer address: OBISLOG
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 02, PEER_SCM);	// peer address: SCM (power)
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 10, PEER_EHZIF);	// peer address: EHZIF
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 11, PEER_USERIF);	// peer address: USERIF
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 12, PEER_WMBUS);	// internal MUC software function
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 13, PEER_ADDRESS_WANGSM);	// peer address: WAN/GSM
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 14, PEER_WANPLC);	// internal MUC software function
	OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, ff, PEER_ADDRESS);	// unit is 255
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 00, VERSION);	// s - (0.2.0) firmware revision
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 01, SET_START_FW_UPDATE);
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 02, FILE_NAME);	// s
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 03, MSG_COUNTER);	// u32 - Anzahl aller Nachrichten zur Übertragung des Binary
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 04, LAST_MSG);	// u32 - Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 05, MSG_NUMBER);
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 02, ff, BLOCK_NUMBER);
	OBIS_CODE_DEFINITION(81, 81, 00, 02, 03, ff, BINARY_DATA);
	OBIS_CODE_DEFINITION(81, 81, 00, 03, 00, 01, SET_DISPATCH_FW_UPDATE);
	OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 01, LOG_SOURCE_LOG);
	OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 02, LOG_SOURCE_SCM);	// internal software function
	OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 03, LOG_SOURCE_UPDATE);
	OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 04, LOG_SOURCE_SMLC);
	OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 0c, LOG_SOURCE_LEDIO);
	OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 14, LOG_SOURCE_WANPLC);
	OBIS_CODE_DEFINITION(81, 81, 10, 06, 01, ff, CODE_LIST_1_VISIBLE_DEVICES);	// tpl - 1. Liste der sichtbaren Sensoren
	OBIS_CODE_DEFINITION(81, 81, 10, 06, 02, ff, CODE_LIST_2_VISIBLE_DEVICES);	// tpl - 2. Liste der sichtbaren Sensoren
	OBIS_CODE_DEFINITION(81, 81, 10, 06, ff, ff, ROOT_VISIBLE_DEVICES);	// visible devices (Liste der sichtbaren Sensoren)
	OBIS_CODE_DEFINITION(81, 81, 10, 16, ff, ff, ROOT_NEW_DEVICES);	// new active devices
	OBIS_CODE_DEFINITION(81, 81, 10, 26, ff, ff, ROOT_INVISIBLE_DEVICES);	// not longer visible devices
	OBIS_CODE_DEFINITION(81, 81, 11, 06, 01, ff, CODE_LIST_1_ACTIVE_DEVICES);	// tpl - 1. Liste der aktiven Sensoren)
	OBIS_CODE_DEFINITION(81, 81, 11, 06, 02, ff, CODE_LIST_2_ACTIVE_DEVICES);	// tpl - 2. Liste der aktiven Sensoren)
	OBIS_CODE_DEFINITION(81, 81, 11, 06, fb, ff, ACTIVATE_DEVICE);	// activate meter/sensor
	OBIS_CODE_DEFINITION(81, 81, 11, 06, fc, ff, DEACTIVATE_DEVICE);	// deactivate meter/sensor
	OBIS_CODE_DEFINITION(81, 81, 11, 06, fd, ff, DELETE_DEVICE);	// delete meter/sensor
	OBIS_CODE_DEFINITION(81, 81, 11, 06, ff, ff, ROOT_ACTIVE_DEVICES);	// active devices (Liste der aktiven Sensoren)
	OBIS_CODE_DEFINITION(81, 81, 12, 06, ff, ff, ROOT_DEVICE_INFO);	// extended device information
	OBIS_CODE_DEFINITION(81, 81, 27, 32, 07, 01, OBISLOG_INTERVAL);	// u16 -  Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook
	OBIS_CODE_DEFINITION(81, 81, 61, 3c, 01, ff, DATA_USER_NAME);
	OBIS_CODE_DEFINITION(81, 81, 61, 3c, 02, ff, DATA_USER_PWD);
	OBIS_CODE_DEFINITION(81, 81, 81, 60, ff, ff, ROOT_ACCESS_RIGHTS);	// see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte 
	OBIS_CODE_DEFINITION(81, 81, 81, 61, ff, ff, ACCESS_USER_NAME);	// user name for access
	OBIS_CODE_DEFINITION(81, 81, 81, 62, ff, ff, ACCESS_PASSWORD);	// SHA256 encrypted
	OBIS_CODE_DEFINITION(81, 81, 81, 63, ff, ff, ACCESS_PUBLIC_KEY);
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 01, ff, ROOT_FILE_TRANSFER);	// 7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) 
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 0c, ff, DATA_FILENAME);
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 0d, ff, DATA_APPLICATION);
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 0e, ff, DATA_FIRMWARE);
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 0f, ff, DATA_FILENAME_INDIRECT);
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 10, ff, DATA_APPLICATION_INDIRECT);
	OBIS_CODE_DEFINITION(81, 81, c7, 81, 23, ff, DATA_PUSH_DETAILS);	// s
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 01, ff, ROOT_DEVICE_IDENT);	// see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) 
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 02, ff, DEVICE_CLASS);	// Geräteklasse (OBIS code or '2D 2D 2D')
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 03, ff, DATA_MANUFACTURER);	// s - FLAG
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 04, ff, SERVER_ID);	// Server ID 
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 05, ff, DATA_PUBLIC_KEY);
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 06, ff, ROOT_FIRMWARE);	// Firmware
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 08, ff, DEVICE_KERNEL);	// s
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 09, ff, HARDWARE_FEATURES);	// hardware equipment (charge, type, ...) 81 81 C7 82 0A NN
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 0a, 01, DEVICE_MODEL);	// s - model code (VMET-1KW-221-1F0)
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 0a, 02, DEVICE_SERIAL);	// s - serial number (3894517)
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 0e, ff, DEVICE_ACTIVATED);
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 41, ff, DEV_CLASS_BASIC_DIRECT);	// 3 x 230 /400 V and 5 (100) A 
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 42, ff, DEV_CLASS_BASIC_SEMI);	// 3 x 230 /400 V and 1 (6) A
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 43, ff, DEV_CLASS_BASIC_INDIRECT);	// 3 x  58 / 100 V and 1 (6) A 
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 45, ff, DEV_CLASS_IW);	// IW module
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 46, ff, DEV_CLASS_PSTN);	// PSTN/GPRS
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 47, ff, DEV_CLASS_GPRS);	// GPRS/PLC
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 48, ff, DEV_CLASS_KM);	// KM module (LAN/DSL)
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 49, ff, DEV_CLASS_NK);	// NK/HS
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 4a, ff, DEV_CLASS_EXTERN);	// external load profile collector 
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 4b, ff, DEV_CLASS_GW);	// gateway
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 4f, ff, DEV_CLASS_LAN);	// see DEV_CLASS_MUC_LAN
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 50, ff, DEV_CLASS_eHZ);
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 52, ff, DEV_CLASS_3HZ);
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 53, ff, DEV_CLASS_MUC_LAN);	// (MUC-LAN/DSL)
	OBIS_CODE_DEFINITION(81, 81, c7, 82, 81, ff, DATA_IP_ADDRESS);
	OBIS_CODE_DEFINITION(81, 81, c7, 83, 82, 01, REBOOT);
	OBIS_CODE_DEFINITION(81, 81, c7, 83, 82, 05, CLEAR_DATA_COLLECTOR);
	OBIS_CODE_DEFINITION(81, 81, c7, 83, 82, 07, UPDATE_FW);
	OBIS_CODE_DEFINITION(81, 81, c7, 83, 82, 0e, SET_ACTIVATE_FW);
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 00, ff, ROOT_SENSOR_PARAMS);	// data mirror root element (Eigenschaften eines Datenspiegels)
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 01, ff, ROOT_SENSOR_BITMASK);	// u16 -  Bitmask to define bits that will be transferred into log
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 02, ff, AVERAGE_TIME_MS);	// average time between two received data records (milliseconds)
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 03, ff, DATA_AES_KEY);
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 04, ff, TIME_REFERENCE);	// u8 - 0 == UTC, 1 == UTC + time zone, 2 == local time
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 10, ff, PROFILE_1_MINUTE);	// 1 minute
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 11, ff, PROFILE_15_MINUTE);
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 12, ff, PROFILE_60_MINUTE);
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 13, ff, PROFILE_24_HOUR);
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 14, ff, PROFILE_LAST_2_HOURS);	// past two hours
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 15, ff, PROFILE_LAST_WEEK);	// weekly (on day change from Sunday to Monday)
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 16, ff, PROFILE_1_MONTH);	// monthly recorded meter readings
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 17, ff, PROFILE_1_YEAR);	// annually recorded meter readings
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 18, ff, PROFILE_INITIAL);	// 81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 20, ff, ROOT_DATA_COLLECTOR);	// data collector root element (Eigenschaften eines Datensammlers)
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 21, ff, DATA_COLLECTOR_ACTIVE);	// bool
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 22, ff, DATA_COLLECTOR_SIZE);	// max. table size
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 2f, ff, TMPL_VERSION);	// s - Version identifier
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 30, ff, TMPL_VERSION_FILTER);	// s - Version identifier filter
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 31, ff, TMPL_DEVICE_CLASS);	// s - Device class, on the basis of which associated sensors / actuators are identified
	OBIS_CODE_DEFINITION(81, 81, c7, 86, 32, ff, TMPL_DEVICE_CLASS_FILTER);	// s - Filter for the device class
	OBIS_CODE_DEFINITION(81, 81, c7, 87, 81, ff, DATA_REGISTER_PERIOD);	// u32 - register period in seconds (0 == event driven)
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 01, ff, ROOT_NTP);
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 02, ff, NTP_SERVER);	// List of NTP servers
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 03, ff, NTP_PORT);	// u16 - NTP port (123)
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 04, ff, NTP_TZ);	// u32 - timezone
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 05, ff, CODE_NTP_OFFSET);	// Offset to transmission of the signal for synchronization
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 06, ff, NTP_ACTIVE);	// NTP enabled/disables
	OBIS_CODE_DEFINITION(81, 81, c7, 88, 10, ff, ROOT_DEVICE_TIME);	// device time
	OBIS_CODE_DEFINITION(81, 81, c7, 89, e1, ff, CLASS_OP_LOG);
	OBIS_CODE_DEFINITION(81, 81, c7, 89, e2, ff, CLASS_EVENT);	// u32 - event
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 01, ff, ROOT_PUSH_OPERATIONS);	// push root element
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 02, ff, PUSH_INTERVAL);	// in seconds
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 03, ff, PUSH_DELAY);	// in seconds
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 04, ff, PUSH_SOURCE);	// options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 21, ff, PUSH_SERVICE_IPT);	// SML data response without request - typical IP - T push
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 22, ff, PUSH_SERVICE_SML);	// SML data response without request - target is SML client address
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 23, ff, DATA_COLLECTOR_REGISTER);	// OBIS list (data mirror)
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 42, ff, PUSH_SOURCE_PROFILE);	// new meter/sensor data
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 43, ff, PUSH_SOURCE_INSTALL);	// configuration changed
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 44, ff, PUSH_SOURCE_VISIBLE_SENSORS);	// list of visible meters changed
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 45, ff, PUSH_SOURCE_ACTIVE_SENSORS);	// list of active meters changed
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 81, ff, PUSH_SERVER_ID);
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 82, ff, PUSH_IDENTIFIERS);	// list of registers to be delivered by the push source
	OBIS_CODE_DEFINITION(81, 81, c7, 8a, 83, ff, PROFILE);	// encode profiles
	OBIS_CODE_DEFINITION(81, 81, c7, 90, 00, ff, ROOT_IF);
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 00, ff, IF_1107);
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 01, ff, IF_1107_ACTIVE);	// bool -  if true 1107 interface active otherwise SML interface active
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 02, ff, IF_1107_LOOP_TIME);	// Loop timeout in seconds
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 03, ff, IF_1107_RETRIES);	// Retry count
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 04, ff, IF_1107_MIN_TIMEOUT);	// Minimal answer timeout(300)
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 05, ff, IF_1107_MAX_TIMEOUT);	//  Maximal answer timeout(5000)
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 06, ff, IF_1107_MAX_DATA_RATE);	// Maximum data bytes(10240)
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 07, ff, IF_1107_RS485);	// bool - if true RS 485, otherwise RS 323
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 08, ff, IF_1107_PROTOCOL_MODE);	// u8 - Protocol mode(A ... D)
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 09, ff, IF_1107_METER_LIST);	// Liste der abzufragenden 1107 Zähler
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 0a, ff, IF_1107_METER_ID);	// s
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 0b, ff, IF_1107_BAUDRATE);
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 0c, ff, IF_1107_ADDRESS);	// s
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 0d, ff, IF_1107_P1);	// s
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 0e, ff, IF_1107_W5);	// s
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 10, ff, IF_1107_AUTO_ACTIVATION);
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 11, ff, IF_1107_TIME_GRID);	// time grid of load profile readout in seconds
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 13, ff, IF_1107_TIME_SYNC);	// time sync in seconds
	OBIS_CODE_DEFINITION(81, 81, c7, 93, 14, ff, IF_1107_MAX_VARIATION);	// seconds
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fd, 00, ATTENTION_OK);	// no error
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fd, 01, ATTENTION_JOB_IS_RUNNINNG);	// attention: job is running
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 00, ATTENTION_UNKNOWN_ERROR);	// attention: unknown error
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 01, ATTENTION_UNKNOWN_SML_ID);	// attention: unknown SML ID
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 02, ATTENTION_NOT_AUTHORIZED);	// attention: not authorized
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 03, ATTENTION_NO_SERVER_ID);	// attention: unable to find recipient for request
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 04, ATTENTION_NO_REQ_FIELD);	// attention: no request field
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 05, ATTENTION_CANNOT_WRITE);	// attention: cannot write
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 06, ATTENTION_CANNOT_READ);	// attention: cannot read
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 07, ATTENTION_COMM_ERROR);	// attention: communication error
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 08, ATTENTION_PARSER_ERROR);	// attention: parser error
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 09, ATTENTION_OUT_OF_RANGE);	// attention: out of range
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 0a, ATTENTION_NOT_EXECUTED);	// attention: not executed
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 0b, ATTENTION_INVALID_CRC);	// attention: invalid CRC
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 0c, ATTENTION_NO_BROADCAST);	// attention: no broadcast
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 0d, ATTENTION_UNEXPECTED_MSG);	// attention: unexpected message
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 0e, ATTENTION_UNKNOWN_OBIS_CODE);	// attention: unknown OBIS code
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 0f, ATTENTION_UNSUPPORTED_DATA_TYPE);	// attention: data type not supported
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 10, ATTENTION_ELEMENT_NOT_OPTIONAL);	// attention: element is not optional
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 11, ATTENTION_NO_ENTRIES);	// attention: no entries
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 12, ATTENTION_END_LIMIT_BEFORE_START);	// attention: end limit before start
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 13, ATTENTION_NO_ENTRIES_IN_RANGE);	// attention: range is empty - not the profile
	OBIS_CODE_DEFINITION(81, 81, c7, c7, fe, 14, ATTENTION_MISSING_CLOSE_MSG);	// attention: missing close message
	// #587
	// next group
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 00, ff, ROOT_BROKER);	// 90 00 00 00 00 NN - broker list
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 01, ff, BROKER_LOGIN);
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 02, ff, BROKER_SERVER);	// ip address
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 03, ff, BROKER_SERVICE);	// u16 - port
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 04, ff, BROKER_USER);
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 05, ff, BROKER_PWD);
	OBIS_CODE_DEFINITION(90, 00, 00, 00, 06, ff, BROKER_TIMEOUT);
	OBIS_CODE_DEFINITION(90, 00, 00, 00, a0, ff, BROKER_BLOCKLIST);
	// #595
	// next group
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 00, ff, ROOT_SERIAL);
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 01, ff, SERIAL_NAME);	// example: /dev/ttyAPP0
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 02, ff, SERIAL_DATABITS);
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 03, ff, SERIAL_PARITY);
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 04, ff, SERIAL_FLOW_CONTROL);
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 05, ff, SERIAL_STOPBITS);
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 06, ff, SERIAL_SPEED);
	OBIS_CODE_DEFINITION(91, 00, 00, 00, 07, ff, SERIAL_TASK);	// LMN port task
	// #603
	// next group
	OBIS_CODE_DEFINITION(92, 00, 00, 00, 00, ff, ROOT_NMS);
	OBIS_CODE_DEFINITION(92, 00, 00, 00, 01, ff, NMS_ADDRESS);
	OBIS_CODE_DEFINITION(92, 00, 00, 00, 02, ff, NMS_PORT);
	OBIS_CODE_DEFINITION(92, 00, 00, 00, 03, ff, NMS_USER);
	OBIS_CODE_DEFINITION(92, 00, 00, 00, 04, ff, NMS_PWD);
	OBIS_CODE_DEFINITION(92, 00, 00, 00, 05, ff, NMS_ENABLED);
	// #609
	// next group
	OBIS_CODE_DEFINITION(93, 00, 00, 00, 00, ff, ROOT_REDIRECTOR);
	OBIS_CODE_DEFINITION(93, 00, 00, 00, 01, ff, REDIRECTOR_LOGIN);
	OBIS_CODE_DEFINITION(93, 00, 00, 00, 02, ff, REDIRECTOR_ADDRESS);	// ip address
	OBIS_CODE_DEFINITION(93, 00, 00, 00, 03, ff, REDIRECTOR_SERVICE);	// u16 - port
	OBIS_CODE_DEFINITION(93, 00, 00, 00, 04, ff, REDIRECTOR_USER);
	OBIS_CODE_DEFINITION(93, 00, 00, 00, 05, ff, REDIRECTOR_PWD);
	// #615
	// next group
	OBIS_CODE_DEFINITION(99, 00, 00, 00, 00, 03, LIST_CURRENT_DATA_RECORD);	// current data set
	OBIS_CODE_DEFINITION(99, 00, 00, 00, 00, 04, LIST_SERVICES);
	OBIS_CODE_DEFINITION(99, 00, 00, 00, 00, 05, FTP_UPDATE);
