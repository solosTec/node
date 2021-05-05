/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

//#include <smf/obis/defs.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/intrinsics/edis.h>
#include <cyng/obj/tag.hpp>
#include <cyng/io/ostream.h>

#include <iostream>
#include <map>
#include <fstream>
#include <iomanip>

#define DEFINE_OBIS(p1, p2, p3, p4, p5, p6)	\
	cyng::obis(0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

 /**
  * internal class to describe an OBIS code
  */
class obis_ctx {
public:
	obis_ctx(char const* name, std::size_t type, char const* desc)	
		: name_(name)
		, type_(type)
		, desc_(desc)
	{}
	char const* get_name() const {
		return name_;
	}
	char const* get_desc() const {
		return desc_;
	}
	std::size_t get_type() const {
		return type_;
	}
private:
	char const* name_;
	std::size_t const type_;
	char const* desc_;
};

class edis_ctx {
public:
	edis_ctx(char const* s, char const* name, char const* unit)
		: short_(s)
		, name_(name)
		, unit_(unit)
	{}
	char const* get_short() const {
		return short_;
	}
	char const* get_name() const {
		return name_;
	}
	char const* get_unit() const {
		return unit_;
	}
private:
	char const* short_;
	char const* name_;
	char const* unit_;
};


 /**
  * Use this program to verify if there are duplicates in the OBIS db
  */
int main(int argc, char** argv) {

	using edis_map_t = std::map< cyng::edis, edis_ctx >;

	static edis_map_t edis_map {

		//Active energy registers :
		{ cyng::edis(1, 8, 0), {"REG_PAE", "Positive active energy(A+)", "kWh" }},
		{ cyng::edis(1, 8, 1), {"REG_PAE_T1", "Positive active energy(A+) in tariff T1", "kWh" }},
		{ cyng::edis(1, 8, 2), {"REG_PAE_T2", "Positive active energy(A+) in tariff T2", "kWh" }},
		{ cyng::edis(1, 8, 3), {"REG_PAE_T3", "Positive active energy(A+) in tariff T3", "kWh" }},
		{ cyng::edis(1, 8, 4), {"REG_PAE_T4", "Positive active energy(A+) in tariff T4", "kWh" }},

		{ cyng::edis(2, 8, 0), {"REG_NAE", "Negative active energy(A+) total", "kWh" }},
		{ cyng::edis(2, 8, 1), {"REG_NAE_T1", "Negative active energy(A+) in tariff T1", "kWh" }},
		{ cyng::edis(2, 8, 2), {"REG_NAE_T2", "Negative active energy(A+) in tariff T2", "kWh" }},
		{ cyng::edis(2, 8, 3), {"REG_NAE_T3", "Negative active energy(A+) in tariff T3", "kWh" }},
		{ cyng::edis(2, 8, 4), {"REG_NAE_T4", "Negative active energy(A+) in tariff T4", "kWh" }},

		{ cyng::edis(15, 8, 0), {"REG_AAE", "Absolute active energy(A+) total", "kWh" }},
		//15.8.1	Absolute active energy(A+) in tariff T1", "kWh" }},
		//15.8.2	Absolute active energy(A+) in tariff T2", "kWh" }},
		//15.8.3	Absolute active energy(A+) in tariff T3", "kWh" }},
		//15.8.4	Absolute active energy(A+) in tariff T4", "kWh" }},
		{ cyng::edis(16, 8, 0), {"REG_SAE", "Sum active energy without reverse blockade(A + -A - ) total", "kWh" }},
		//16.8.Sum active energy without reverse blockade(A + -A - ) in tariff T1", "kWh" }},
		//16.8.2	Sum active energy without reverse blockade(A + -A - ) in tariff T2", "kWh" }},
		//16.8.3	Sum active energy without reverse blockade(A + -A - ) in tariff T3", "kWh" }},
		//16.8.4	Sum active energy without reverse blockade(A + -A - ) in tariff T4", "kWh" }},
		//2. Reactive energy registers
		{ cyng::edis(3, 8, 0), {"REG_PRE", "Positive reactive energy(Q+) total", "kvarh" }},
		//3.8.1	Positive reactive energy(Q+) in tariff T1", "kvarh" }},
		//3.8.2	Positive reactive energy(Q+) in tariff T2", "kvarh" }},
		//3.8.3	Positive reactive energy(Q+) in tariff T3", "kvarh" }},
		//3.8.4	Positive reactive energy(Q+) in tariff T4", "kvarh" }},
		{ cyng::edis(4, 8, 0), {"REG_NRE", "Negative reactive energy(Q-) total", "kvarh" }},
		//4.8.1	Negative reactive energy(Q-) in tariff T1", "kvarh" }},
		//4.8.2	Negative reactive energy(Q-) in tariff T2", "kvarh" }},
		//4.8.3	Negative reactive energy(Q-) in tariff T3", "kvarh" }},
		//4.8.4	Negative reactive energy(Q-) in tariff T4", "kvarh" }},
		{ cyng::edis(5, 8, 0), {"REG_IIRE", "Imported inductive reactive energy in 1 - st quadrant(Q1) total", "kvarh" }},
		//5.8.1	Imported inductive reactive energy in 1 - st quadrant(Q1) in tariff T1", "kvarh" }},
		//5.8.2	Imported inductive reactive energy in 1 - st quadrant(Q1) in tariff T2", "kvarh" }},
		//5.8.3	Imported inductive reactive energy in 1 - st quadrant(Q1) in tariff T3", "kvarh" }},
		//5.8.4	Imported inductive reactive energy in 1 - st quadrant(Q1) in tariff T4", "kvarh" }},
		{ cyng::edis(6, 8, 0), {"REG_ICRE", "Imported capacitive reactive energy in 2 - nd quadrant(Q2) total", "kvarh" }},
		//6.8.1	Imported capacitive reactive energy in 2 - nd quadr. (Q2)in tariff T1", "kvarh" }},
		//6.8.2	Imported capacitive reactive energy in 2 - nd quadr. (Q2)in tariff T2", "kvarh" }},
		//6.8.3	Imported capacitive reactive energy in 2 - nd quadr. (Q2)in tariff T3", "kvarh" }},
		//6.8.4	Imported capacitive reactive energy in 2 - nd quadr. (Q2)in tariff T4", "kvarh" }},
		{ cyng::edis(7, 8, 0), {"REG_EIRE", "Exported inductive reactive energy in 3 - rd quadrant(Q3) total", "kvarh" }},
		//7.8.1	Exported inductive reactive energy in 3 - rd quadrant(Q3) in tariff T1", "kvarh" }},
		//7.8.2	Exported inductive reactive energy in 3 - rd quadrant(Q3) in tariff T2", "kvarh" }},
		//7.8.3	Exported inductive reactive energy in 3 - rd quadrant(Q3) in tariff T3", "kvarh" }},
		//7.8.4	Exported inductive reactive energy in 3 - rd quadrant(Q3) in tariff T4", "kvarh" }},
		{ cyng::edis(8, 8, 0), {"REG_ECRE", "Exported capacitive reactive energy in 4 - th quadrant(Q4) total", "kvarh" }},
		//8.8.1	Exported capacitive reactive energy in 4 - th quadr. (Q4)in tariff T1", "kvarh" }},
		//8.8.2	Exported capacitive reactive energy in 4 - th quadr. (Q4)in tariff T2", "kvarh" }},
		//8.8.3	Exported capacitive reactive energy in 4 - th quadr. (Q4)in tariff T3", "kvarh" }},
		//8.8.4	Exported capacitive reactive energy in 4 - th quadr. (Q4)in tariff T4", "kvarh" }},

		//3. Apparent energy registers
		{ cyng::edis(9, 8, 0), {"REG_AE", "Apparent energy(S + ) total", "kVAh" }},
		//9.8.1	Apparent energy(S + ) in tariff T1", "kVAh" }},
		//9.8.2	Apparent energy(S + ) in tariff T2", "kVAh" }},
		//9.8.3	Apparent energy(S + ) in tariff T3", "kVAh" }},
		//9.8.4	Apparent energy(S + ) in tariff T4", "kVAh" }},
		{ cyng::edis(10, 8, 0), {"REG_AED", "Apparent energy total - Delivery (10.8.0)", "" }},
		{ cyng::edis(10, 8, 1), {"REG_AED_T1", "Apparent energy tariff 1 - Delivery (10.8.1)", "" }},
		{ cyng::edis(10, 8, 2), {"REG_AED_T2", "Apparent energy tariff 2 - Delivery (10.8.2)", "" }},
		{ cyng::edis(10, 8, 3), {"REG_AED_T3", "Apparent energy tariff 3 - Delivery (10.8.3)", "" }},
		{ cyng::edis(10, 8, 4), {"REG_AED_T4", "Apparent energy tariff 4 - Delivery (10.8.4)", "" }},

		//4. Registers of active energy per phases
		{ cyng::edis(21, 8, 0), {"REG_PAEL1", "Positive active energy(A+) in phase L1 total", "kWh" }},
		{ cyng::edis(41, 8, 0), {"REG_PAEL2", "Positive active energy(A+) in phase L2 total", "kWh" }},
		{ cyng::edis(61, 8, 0), {"REG_PAEL1T", "Positive active energy(A+) in phase L3 total", "kWh" }},
		{ cyng::edis(22, 8, 0), {"REG_NAEL1T", "Negative active energy(A-) in phase L1 total", "kWh" }},
		{ cyng::edis(42, 8, 0), {"REG_NAEL2T", "Negative active energy(A-) in phase L2 total", "kWh" }},
		{ cyng::edis(62, 8, 0), {"REG_NAEL3T", "Negative active energy(A-) in phase L3 total", "kWh" }},
		{ cyng::edis(35, 8, 0), {"REG_AAEL1", "Absolute active energy(|A|) in phase L1 total", "kWh" }},
		{ cyng::edis(55, 8, 0), {"REG_AAEL2", "Absolute active energy(|A|) in phase L2 total", "kWh" }},
		{ cyng::edis(75, 8, 0), {"REG_AAEL3", "Absolute active energy(|A|) in phase L3 total", "kWh" }},

		//5. Maximum demand registers :
		{ cyng::edis(1, 6, 0), {"REG_PAMD", "Positive active maximum demand(A+) total", "kW" }},
		{ cyng::edis(1, 6, 1), {"REG_PAMD_T1", "Positive active maximum demand(A+) in tariff 1", "kW" }},
		{ cyng::edis(1, 6, 2), {"REG_PAMD_T2", "Positive active maximum demand(A+) in tariff 2", "kW" }},
		{ cyng::edis(1, 6, 3), {"REG_PAMD_T3", "Positive active maximum demand(A+) in tariff 3", "kW" }},
		{ cyng::edis(1, 6, 4), {"REG_PAMD_T4", "Positive active maximum demand(A+) in tariff 4", "kW" }},

		{ cyng::edis(2, 6, 0), {"REG_NAMD", "Negative active maximum demand(A-) total", "kW" }},
		{ cyng::edis(2, 6, 1), {"REG_NAMD_T1", "Negative active maximum demand(A-) in tariff 1", "kW" }},
		{ cyng::edis(2, 6, 2), {"REG_NAMD_T2", "Negative active maximum demand(A-) in tariff 2", "kW" }},
		{ cyng::edis(2, 6, 3), {"REG_NAMD_T3", "Negative active maximum demand(A-) in tariff 3", "kW" }},
		{ cyng::edis(2, 6, 4), {"REG_NAMD_T4", "Negative active maximum demand(A-) in tariff 4", "kW" }},

		{ cyng::edis(15, 6, 0), {"REG_AAMD", "Absolute active maximum demand(|A| ) total", "kW" }},
		//15.6.1	Absolute active maximum demand(|A| ) in tariff T1", "kW" }},
		//15.6.2	Absolute active maximum demand(|A| ) in tariff T2", "kW" }},
		//15.6.3	Absolute active maximum demand(|A| ) in tariff T3", "kW" }},
		//15.6.4	Absolute active maximum demand(|A| ) in tariff T4", "kW" }},
		{ cyng::edis(3, 6, 0), {"REG_PRMD", "Positive reactive maximum demand(Q+) total", "kvar" }},
		{ cyng::edis(4, 6, 0), {"REG_NRMD", "Negative reactive maximum demand(Q-) total", "kvar" }},
		{ cyng::edis(5, 6, 0), {"REG_RMDQ1", "Reactive maximum demand in Q1(Q1) total", "kvar" }},
		{ cyng::edis(6, 6, 0), {"REG_RMDQ2", "Reactive maximum demand in Q2(Q2) total", "kvar" }},
		{ cyng::edis(7, 6, 0), {"REG_RMDQ3", "Reactive maximum demand in Q3(Q3) total", "kvar" }},
		{ cyng::edis(8, 6, 0), {"REG_RMDQ4", "Reactive maximum demand in Q4(Q4) total", "kvar" }},
		{ cyng::edis(9, 6, 0), {"REG_AMDT", "Apparent maximum demand(S+) total", "kVA" }},
		{ cyng::edis(9, 6, 1), {"REG_AMDT_T1", "Apparent maximum demand(S+) in tariff 1", "kVA" }},
		{ cyng::edis(9, 6, 2), {"REG_AMDT_T2", "Apparent maximum demand(S+) in tariff 2", "kVA" } },
		{ cyng::edis(9, 6, 3), {"REG_AMDT_T3", "Apparent maximum demand(S+) in tariff 3", "kVA" } },
		{ cyng::edis(9, 6, 4), {"REG_AMDT_T4", "Apparent maximum demand(S+) in tariff 4", "kVA" } },

		//6. Cumulative maximum demand registers
		{ cyng::edis(1, 2, 0), {"REG_PACMD", "Positive active cumulative maximum demand(A+) total", "kW" }},
		//1.2.1	Positive active cumulative maximum demand(A+) in tariff T1", "kW" }},
		//1.2.2	Positive active cumulative maximum demand(A+) in tariff T2", "kW" }},
		//1.2.3	Positive active cumulative maximum demand(A+) in tariff T3", "kW" }},
		//1.2.4	Positive active cumulative maximum demand(A+) in tariff T4", "kW" }},
		{ cyng::edis(2, 2, 0), {"REG_NACMD", "Negative active cumulative maximum demand(A-) total", "kW" }},
		//2.2.1	Negative active cumulative maximum demand(A-) in tariff T1", "kW" }},
		//2.2.2	Negative active cumulative maximum demand(A-) in tariff T2", "kW" }},
		//2.2.3	Negative active cumulative maximum demand(A-) in tariff T3", "kW" }},
		//2.2.4	Negative active cumulative maximum demand(A-) in tariff T4", "kW" }},
		{ cyng::edis(15, 2, 0), {"REG_AACMD", "Absolute active cumulative maximum demand(|A| ) total", "kW" }},
		//15.2.1	Absolute active cumulative maximum demand(|A| ) in tariff T1", "kW" }},
		//15.2.2	Absolute active cumulative maximum demand(|A| ) in tariff T2", "kW" }},
		//15.2.3	Absolute active cumulative maximum demand(|A| ) in tariff T3", "kW" }},
		//15.2.4	Absolute active cumulative maximum demand(|A| ) in tariff T4", "kW" }},
		{ cyng::edis(3, 2, 0), {"REG_PRCMD", "Positive reactive cumulative maximum demand(Q+) total", "kvar" }},
		{ cyng::edis(4, 2, 0), {"REG_NRCMD", "Negative reactive cumulative maximum demand(Q-) total", "kvar" }},
		{ cyng::edis(5, 2, 0), {"REG_RCMDQ1", "Reactive cumulative maximum demand in Q1(Q1) total", "kvar" }},
		{ cyng::edis(6, 2, 0), {"REG_RCMDQ2", "Reactive cumulative maximum demand in Q2(Q2) total", "kvar" }},
		{ cyng::edis(7, 2, 0), {"REG_RCMDQ3", "Reactive cumulative maximum demand in Q3(Q3) total", "kvar" }},
		{ cyng::edis(8, 2, 0), {"REG_RCMDQ4", "Reactive cumulative maximum demand in Q4(Q4) total", "kvar" }},
		{ cyng::edis(9, 2, 0), {"REG_RCMDT", "Apparent cumulative maximum demand(S + ) total", "kVA" }},

		//7. Demands in a current demand period
		{ cyng::edis(1, 4, 0), {"REG_PADCDP", "Positive active demand in a current demand period(A+)", "kW" }},
		{ cyng::edis(2, 4, 0), {"REG_NADCDP", "Negative active demand in a current demand period(A-)", "kW" }},
		{ cyng::edis(15, 4, 0), {"REG_AADCDP", "Absolute active demand in a current demand period(|A| )", "kW" }},
		{ cyng::edis(3, 4, 0), {"REG_PRDCDP", "Positive reactive demand in a current demand period(Q+)", "kvar" }},
		{ cyng::edis(4, 4, 0), {"REG_NRDCDP", "Negative reactive demand in a current demand period(Q-)", "kvar" }},
		{ cyng::edis(5, 4, 0), {"REG_RDCDPQ1", "Reactive demand in a current demand period in Q1(Q1)", "kvar" }},
		{ cyng::edis(6, 4, 0), {"REG_RDCDPQ2", "Reactive demand in a current demand period in Q2(Q2)", "kvar" }},
		{ cyng::edis(7, 4, 0), {"REG_RDCDPQ3", "Reactive demand in a current demand period in Q3(Q3)", "kvar" }},
		{ cyng::edis(8, 4, 0), {"REG_RDCDPQ4", "Reactive demand in a current demand period in Q4(Q4)", "kvar" }},
		{ cyng::edis(9, 4, 0), {"REG_RDCDPT", "Apparent demand in a current demand period(S+)", "kVA" }},

		//8. Demands in the last completed demand period
		{ cyng::edis(1, 5, 0), {"REG_PADLCDP", "Positive active demand in the last completed demand period(A+)", "kW" }},
		{ cyng::edis(2, 5, 0), {"REG_NADLCDP", "Negative active demand in the last completed demand period(A-)", "kW" }},
		{ cyng::edis(15, 5, 0), {"REG_AADLCDP", "Absolute active demand in the last completed demand period(|A| )", "kW" }},
		{ cyng::edis(3, 5, 0), {"REG_RDLCDP", "Positive reactive demand in the last completed demand period(Q+)", "kvar" }},
		{ cyng::edis(4, 5, 0), {"REG_NRDLCDP", "Negative reactive demand in the last completed demand period(Q-)", "kvar" }},
		{ cyng::edis(5, 5, 0), {"REG_RDLCDPQ1", "Reactive demand in the last completed demand period in Q1(Q1)", "kvar" }},
		{ cyng::edis(6, 5, 0), {"REG_RDLCDPQ2", "Reactive demand in the last completed demand period in Q2(Q2)", "kvar" }},
		{ cyng::edis(7, 5, 0), {"REG_RDLCDPQ3", "Reactive demand in the last completed demand period in Q3(Q3)", "kvar" }},
		{ cyng::edis(8, 5, 0), {"REG_RDLCDPQ4", "Reactive demand in the last completed demand period in Q4(Q4)", "kvar" }},
		{ cyng::edis(9, 5, 0), {"REG_ADLCDP", "Apparent demand in the last completed demand period(S + )", "kVA" }},

		{ cyng::edis(10, 6, 1), {"REG_TODO_T1", "Instantaneous apparent power average (S+) in tariff 1", "kVA" } },
		{ cyng::edis(10, 6, 2), {"REG_TODO_T2", "Instantaneous apparent power average (S+) in tariff 2", "kVA" } },
		{ cyng::edis(10, 6, 3), {"REG_TODO_T3", "Instantaneous apparent power average (S+) in tariff 3", "kVA" } },
		{ cyng::edis(10, 6, 4), {"REG_TODO_T4", "Instantaneous apparent power average (S+) in tariff 4", "kVA" } },

		//9. Instantaneous power registers
		{ cyng::edis(1, 7, 0), {"REG_PAIP", "Positive active instantaneous power(A+)", "kW" }},
		{ cyng::edis(21, 7, 0), {"REG_PAIPL1", "Positive active instantaneous power(A+) in phase L1", "kW" }},
		{ cyng::edis(41, 7, 0), {"REG_PAIPL2", "Positive active instantaneous power(A+) in phase L2", "kW" }},
		{ cyng::edis(61, 7, 0), {"REG_PAIPL3", "Positive active instantaneous power(A+) in phase L3", "kW" }},
		{ cyng::edis(2, 7, 0), {"REG_NAIP", "Negative active instantaneous power(A-)", "kW" }},
		{ cyng::edis(22, 7, 0), {"REG_NAIPL1", "Negative active instantaneous power(A-) in phase L1", "kW" }},
		{ cyng::edis(42, 7, 0), {"REG_NAIPL2", "Negative active instantaneous power(A-) in phase L2", "kW" }},
		{ cyng::edis(62, 7, 0), {"REG_NAIPL3", "Negative active instantaneous power(A-) in phase L3", "kW" }},
		{ cyng::edis(15, 7, 0), {"REG_AAIP", "Absolute active instantaneous power(|A| )", "kW" }},
		{ cyng::edis(35, 7, 0), {"REG_AAIPL1", "Absolute active instantaneous power(|A| ) in phase L1", "kW" }},
		{ cyng::edis(55, 7, 0), {"REG_AAIPL2", "Absolute active instantaneous power(|A| ) in phase L2", "kW" }},
		{ cyng::edis(75, 7, 0), {"REG_AAIPL3", "Absolute active instantaneous power(|A| ) in phase L3", "kW" }},
		{ cyng::edis(16, 7, 0), {"REG_SIP", "Sum active instantaneous power(A + -A - )", "kW" }},
		{ cyng::edis(36, 7, 0), {"REG_SIPL1", "Sum active instantaneous power(A + -A - ) in phase L1", "kW" } },
		{ cyng::edis(56, 7, 0), {"REG_SIPL2", "Sum active instantaneous power(A + -A - ) in phase L2", "kW" } },
		{ cyng::edis(76, 7, 0), {"REG_SIPL3", "Sum active instantaneous power(A + -A - ) in phase L3", "kW" } },
		{ cyng::edis(3, 7, 0), {"REG_PRIP", "Positive reactive instantaneous power(Q+)", "kvar" } },
		{ cyng::edis(23, 7, 0), {"REG_PRIPL1", "Positive reactive instantaneous power(Q+) in phase L1", "kvar" } },
		{ cyng::edis(43, 7, 0), {"REG_PRIPL2", "Positive reactive instantaneous power(Q+) in phase L2", "kvar" } },
		{ cyng::edis(63, 7, 0), {"REG_PRIPL3", "Positive reactive instantaneous power(Q+) in phase L3", "kvar" } },
		{ cyng::edis(4, 7, 0), {"REG_NRIP", "Negative reactive instantaneous power(Q-)", "kvar" } },
		{ cyng::edis(24, 7, 0), {"REG_NRIPL1", "Negative reactive instantaneous power(Q-) in phase L1", "kvar" } },
		{ cyng::edis(44, 7, 0), {"REG_NRIPL2", "Negative reactive instantaneous power(Q-) in phase L2", "kvar" } },
		{ cyng::edis(64, 7, 0), {"REG_NRIPL3", "Negative reactive instantaneous power(Q-) in phase L3", "kvar" } },
		{ cyng::edis(9, 7, 0), {"REG_AIP", "Apparent instantaneous power(S + )", "kVA" } },
		{ cyng::edis(29, 7, 0), {"REG_AIPL1", "Apparent instantaneous power(S + ) in phase L1", "kVA" } },
		{ cyng::edis(49, 7, 0), {"REG_AIPL2", "Apparent instantaneous power(S + ) in phase L2", "kVA" } },
		{ cyng::edis(69, 7, 0), {"REG_AIPL3", "Apparent instantaneous power(S + ) in phase L3", "kVA" } },

		//10. Electricity network quality registers
		{ cyng::edis(11, 7, 0), {"REG_IC", "Instantaneous current(I)", "A" } },
		{ cyng::edis(31, 7, 0), {"REG_ICL1", "Instantaneous current(I) in phase L1", "A" } },
		{ cyng::edis(51, 7, 0), {"REG_ICL2", "Instantaneous current(I) in phase L2", "A" } },
		{ cyng::edis(71, 7, 0), {"REG_ICL3", "Instantaneous current(I) in phase L3", "A" } },
		{ cyng::edis(91, 7, 0), {"REG_ICN", "Instantaneous current(I) in neutral", "A" } },
		{ cyng::edis(11, 6, 0), {"REG_MC", "Maximum current(I max)", "A" } },
		{ cyng::edis(31, 6, 0), {"REG_MCL1", "Maximum current(I max) in phase L1", "A" } },
		{ cyng::edis(51, 6, 0), {"REG_MCL2", "Maximum current(I max) in phase L2", "A" } },
		{ cyng::edis(71, 6, 0), {"REG_MCL3", "Maximum current(I max) in phase L3", "A" } },
		{ cyng::edis(91, 6, 0), {"REG_MCN", "Maximum current(I max) in neutral", "A" } },
		{ cyng::edis(12, 7, 0), {"REG_IV", "Instantaneous voltage(U)", "V" } },
		{ cyng::edis(32, 7, 0), {"REG_IVL1", "Instantaneous voltage(U) in phase L1", "V" } },
		{ cyng::edis(52, 7, 0), {"REG_IVL2", "Instantaneous voltage(U) in phase L2", "V" } },
		{ cyng::edis(72, 7, 0), {"REG_IVL3", "Instantaneous voltage(U) in phase L3", "V" } },
		{ cyng::edis(13, 7, 0), {"REG_IPF", "Instantaneous power factor", "" } },
		{ cyng::edis(33, 7, 0), {"REG_IPFL1", "Instantaneous power factor in phase L1", "" } },
		{ cyng::edis(53, 7, 0), {"REG_IPFL2", "Instantaneous power factor in phase L2", "" } },
		{ cyng::edis(73, 7, 0), {"REG_IPFL3", "Instantaneous power factor in phase L3", "" } },
		{ cyng::edis(14, 7, 0), {"REG_FREQ", "Frequency", "Hz" } },

		//11. Tamper registers(energy registers and registers of elapsed time)
		{ cyng::edis(0x60, 53, 1), {"REG_TAMPER_ENERGY_1", "Tamper 1 energy", "register" }},
		{ cyng::edis(0x60, 53, 2), {"REG_TAMPER_ENERGY_2", "Tamper 2 energy", "register" }},
		{ cyng::edis(0x60, 53, 3), {"REG_TAMPER_ENERGY_3", "Tamper 3 energy", "register" }},
		{ cyng::edis(0x60, 53, 4), {"REG_TAMPER_ENERGY_4", "Tamper 4 energy", "register" }},
		{ cyng::edis(0x60, 53, 11), {"REG_TAMPER_ENERGY_5", "Tamper 5 energy", "register" }},
		{ cyng::edis(0x60, 53, 5), {"REG_TAMPER_TIME_1", "Tamper 1 time counter register", "" }},
		{ cyng::edis(0x60, 53, 6), {"REG_TAMPER_TIME_2", "Tamper 2 time counter register", "" }},
		{ cyng::edis(0x60, 53, 7), {"REG_TAMPER_TIME_3", "Tamper 3 time counter register", "" }},
		{ cyng::edis(0x60, 53, 9), {"REG_TAMPER_TIME_4", "Tamper 4 time counter register", "" }},
		{ cyng::edis(0x60, 53, 10), {"REG_TAMPER_TIME_5", "Tamper 5 time counter register", "" }},

		//12. Events registers(counters and time - stamps)
		{ cyng::edis(0x60, 2, 0), {"REG_EVT_PARAM_CHANGED_COUNTER", "Event parameters change", "counter" }},
		{ cyng::edis(0x60, 2, 1), {"REG_EVT_PARAM_CHANGED_TIMESTAMP", "Event parameters change", "timestamp" }},
		{ cyng::edis(0x60, 51, 1), {"REG_EVT_1", "Event terminal cover opened", "counter" }},
		{ cyng::edis(0x60, 51, 2), {"REG_EVT_2", "Event terminal cover opened", "timestamp" }},
		{ cyng::edis(0x60, 51, 3), {"REG_EVT_3", "Event main cover opened - counter", "" }},
		{ cyng::edis(0x60, 51, 4), {"REG_EVT_4", "Event main cover opened ", "timestamp" } },
		{ cyng::edis(0x60, 51, 5), {"REG_EVT_5", "Event magnetic field detection start - counter", "" }},
		{ cyng::edis(0x60, 51, 6), {"REG_EVT_6", "Event magnetic field detection start", "timestamp" } },
		{ cyng::edis(0x60, 51, 7), {"REG_EVT_7", "Event reverse power flow", "counter" } },
		{ cyng::edis(0x60, 51, 8), {"REG_EVT_8", "Event reverse power flow", "timestamp" } },
		{ cyng::edis(0x60, 7, 0), {"REG_EVT_POWER_DOWN_COUNTER", "Event power down", "counter" } },
		{ cyng::edis(0x60, 7, 10), {"REG_EVT_POWER_DOWN_TIMESTAMP", "Event power down", "timestamp" } },
		{ cyng::edis(0x60, 51, 13), {"REG_EVT_13", "Event power up", "counter" } },
		{ cyng::edis(0x60, 51, 14), {"REG_EVT_14", "Event power up", "timestamp" } },
		{ cyng::edis(0x60, 51, 15), {"REG_EVT_15", "Event RTC(Real Time Clock) set", "counter" } },
		{ cyng::edis(0x60, 51, 16), {"REG_EVT_16", "Event RTC(Real Time Clock) set", "timestamp" } },
		{ cyng::edis(0x60, 51, 21), {"REG_EVT_21", "Event terminal cover closed", "counter" } },
		{ cyng::edis(0x60, 51, 22), {"REG_EVT_22", "Event terminal cover closed", "timestamp" } },
		{ cyng::edis(0x60, 51, 23), {"REG_EVT_23", "Event main cover closed", "counter" } },
		{ cyng::edis(0x60, 51, 24), {"REG_EVT_24", "Event main cover closed", "timestamp" } },
		{ cyng::edis(0x60, 51, 25), {"REG_EVT_25", "Event log - book 1 erased", "counter" } },
		{ cyng::edis(0x60, 51, 26), {"REG_EVT_26", "Event log - book 1 erased", "timestamp" } },
		{ cyng::edis(0x60, 51, 27), {"REG_EVT_27", "Event fraud start", "counter" } },
		{ cyng::edis(0x60, 51, 28), {"REG_EVT_28", "Event fraud start", "timestamp" } },
		{ cyng::edis(0x60, 51, 29), {"REG_EVT_29", "Event fraud stop", "counter" } },
		{ cyng::edis(0x60, 51, 30), {"REG_EVT_30", "Event fraud stop", "timestamp" } },
		{ cyng::edis(0x60, 87, 0), {"REG_ACTIVE_TARFIFF", "Active tariff", "" } },
		{ cyng::edis(0x60, 0x60, 9), {"REG_FRAUD_FLAG", "Fraud flag", "" } },

		//13. Miscellaneous registers used in sequences
		{ cyng::edis(0, 1, 0), {"REG_MD_RESET_COUNTER", "MD reset counter", "" } },
		{ cyng::edis(0, 1, 2), {"REG_MD_RESET_TIMESTAMP", "MD reset timestamp", "" } },
		{ cyng::edis(0, 2, 1), {"REG_PARAMETERS_SCHEME_ID", "Parameters scheme ID", "" } },
		{ cyng::edis(0, 2, 2), {"REG_TARIFF", "Tariff program ID", "" } },
		{ cyng::edis(0, 3, 0), {"REG_ACTIVE_ENERGY_METER_CONSTANT", "Active energy meter constant", "" } },
		{ cyng::edis(0, 3, 2), {"REG_SM_PULSE_VALUE", "S0- Impulswertigkeit (0.3.2)", "" } },
		{ cyng::edis(0, 3, 3), {"REG_SM_PULSE_DURATION", "Pulse length (0.3.3)", "" } },
		{ cyng::edis(0, 4, 2), {"REG_SM_RATIO_CONVERTER", "Converter factor (0.4.2)", "ratio" } },
		{ cyng::edis(0, 4, 3), {"REG_SM_RATIO_VOLTAGE", "Voltage transformer (0.4.3)", "ratio" } },
		{ cyng::edis(0, 5, 2), {"REG_SM_MEASURING_PERIOD", "Measuring period (0.2.2)", "" } },
		{ cyng::edis(0, 8, 0), {"REG_DP2_DEMAND_INTERVAL", "Duration of measurement interval for current power value (1-0.0:8.0*255)", "min" } },
		{ cyng::edis(0, 8, 4), {"REG_LOAD_PROFILE_PERIOD", "Load profile period", "min" } },
		{ cyng::edis(0, 9, 1), {"REG_DT_CURRENT_TIME", "Current time at time of transmission (hh:mm:ss)", "" }},
		{ cyng::edis(0, 9, 2), {"REG_DATE", "Date(YY.MM.DD or DD.MM.YY)", "" }},
		{ cyng::edis(0, 9, 4), {"REG_DATE_TIME", "Date and Time(YYMMDDhhmmss)", "" }},
		{ cyng::edis(61, 0x61, 0), {"REG_FATAL_ERROR_METER_STATUS", "Fatal error meter status", "" }}
	};


	using obis_map_t = std::map< cyng::obis, obis_ctx >;
	obis_map_t obis_map {

		{ DEFINE_OBIS(00, 00, 00, 00, 00, ff), {"METER_ADDRESS", cyng::TC_NULL, "0-0:0.0.0.255" }},
		{ DEFINE_OBIS(00, 00, 00, 00, 01, ff), {"IDENTITY_NR_1", cyng::TC_NULL, "0-0:0.0.1.255" }},
		{ DEFINE_OBIS(00, 00, 00, 00, 02, ff), {"IDENTITY_NR_2", cyng::TC_NULL, "0-0:0.0.2.255" }},
		{ DEFINE_OBIS(00, 00, 00, 00, 03, ff), {"IDENTITY_NR_3", cyng::TC_NULL, "0-0:0.0.3.255" }},
		{ DEFINE_OBIS(00, 00, 00, 01, 00, ff), {"RESET_COUNTER", cyng::TC_UINT32, "0.1.0" }},
		{ DEFINE_OBIS(00, 00, 00, 02, 00, ff), {"FIRMWARE_VERSION", cyng::TC_NULL, "COSEM class id 1" }},
		{ DEFINE_OBIS(00, 00, 01, 00, 00, ff), {"REAL_TIME_CLOCK", cyng::TC_NULL, "current time" }},

		{ DEFINE_OBIS(00, 00, 60, 01, 00, ff), {"SERIAL_NR", cyng::TC_NULL, "(C.1.0) Serial number I (assigned by the manufacturer)" }},
		{ DEFINE_OBIS(00, 00, 60, 01, 01, ff), {"SERIAL_NR_SECOND", cyng::TC_NULL, "Serial number II (assigned by the manufacturer)." }},
		{ DEFINE_OBIS(00, 00, 60, 01, 02, ff), {"PARAMETERS_FILE_CODE", cyng::TC_NULL, "Parameters file code (C.1.2)" }},
		{ DEFINE_OBIS(00, 00, 60, 01, 03, ff), {"PRODUCTION_DATE", cyng::TC_NULL, "date of manufacture (C.1.3)" }},
		{ DEFINE_OBIS(00, 00, 60, 01, 04, ff), {"PARAMETERS_CHECK_SUM", cyng::TC_NULL, "Parameters check sum (C.1.4)" }},
		{ DEFINE_OBIS(00, 00, 60, 01, 05, ff), {"FIRMWARE_BUILD_DATE", cyng::TC_NULL, "Firmware built date (C.1.5)" }},
		{ DEFINE_OBIS(00, 00, 60, 01, 06, ff), {"FIRMWARE_CHECK_SUM", cyng::TC_NULL, "Firmware check sum (C.1.6)" }},

		{ DEFINE_OBIS(00, 00, 60, 01, ff, ff), {"FABRICATION_NR", cyng::TC_NULL, "fabrication number" }},
		{ DEFINE_OBIS(00, 00, 60, 02, 01, ff), {"DATE_TIME_PARAMETERISATION", cyng::TC_NULL, "Date of last parameterisation (00-03-26)" }},
		{ DEFINE_OBIS(00, 00, 60, 03, 00, ff), {"PULSE_CONST_ACTIVE", cyng::TC_UINT32, "Active pulse constant (C.3.0)" }},
		{ DEFINE_OBIS(00, 00, 60, 03, 01, ff), {"PULSE_CONST_REACTIVE", cyng::TC_UINT32, "Reactive pulse constant (C.3.1)" }},

		{ DEFINE_OBIS(00, 00, 60, 06, 00, ff), {"COUNTER_POWER_DOWN_TIME", cyng::TC_UINT32, "Power down time counter (C.6.0)" }},
		{ DEFINE_OBIS(00, 00, 60, 06, 01, ff), {"BATTERY_REMAINING_CAPACITY", cyng::TC_UINT32, "Battery remaining capacity (C.6.1)" }},

		{ DEFINE_OBIS(00, 00, 60, 07, 00, ff), {"POWER_OUTAGES", cyng::TC_UINT32, "Number of power failures " }},
		{ DEFINE_OBIS(00, 00, 60, 08, 00, ff), {"SECONDS_INDEX", cyng::TC_NULL, "[SML_Time] seconds index" }},
		{ DEFINE_OBIS(00, 00, 60, 10, 00, ff), {"LOGICAL_NAME", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, 00, 60, 48, 00, ff), {"COUNTER_UNDER_VOLTAGE", cyng::TC_UINT32, "Number of under-voltages" }},
		{ DEFINE_OBIS(00, 00, 60, 49, 00, ff), {"COUNTER_OVER_VOLTAGE", cyng::TC_UINT32, "Number of over-voltages" }},
		{ DEFINE_OBIS(00, 00, 60, 4A, 00, ff), {"COUNTER_OVER_LOAD", cyng::TC_UINT32, "Number of over-loads (over-current) " }},
		{ DEFINE_OBIS(00, 00, 60, f0, 0d, ff), {"HARDWARE_TYPE", cyng::TC_STRING, "name" }},
		{ DEFINE_OBIS(00, 00, 61, 61, 00, ff), {"MBUS_STATE", cyng::TC_NULL, "Status according to EN13757-3 (error register)" }},
		{ DEFINE_OBIS(00, 00, 61, 61, 01, ff), {"MBUS_STATE_1", cyng::TC_NULL, "not relevant under calibration law" }},
		{ DEFINE_OBIS(00, 00, 61, 61, 02, ff), {"MBUS_STATE_2", cyng::TC_NULL, "not relevant under calibration law" }},
		{ DEFINE_OBIS(00, 00, 61, 61, 03, ff), {"MBUS_STATE_3", cyng::TC_NULL, "not relevant under calibration law" }},
		{ DEFINE_OBIS(00, 80, 80, 00, 00, ff), {"STORAGE_TIME_SHIFT", cyng::TC_NULL, "root push operations" }},
		{ DEFINE_OBIS(00, 80, 80, 00, 03, 01), {"HAS_SSL_CONFIG", cyng::TC_NULL, "SSL/TSL configuration available" }},
		{ DEFINE_OBIS(00, 80, 80, 00, 04, ff), {"SSL_CERTIFICATES", cyng::TC_NULL, "list of SSL certificates" }},
		{ DEFINE_OBIS(00, 80, 80, 00, 10, ff), {"ROOT_MEMORY_USAGE", cyng::TC_NULL, "request memory usage" }},
		{ DEFINE_OBIS(00, 80, 80, 00, 11, ff), {"ROOT_MEMORY_MIRROR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, 80, 80, 00, 12, ff), {"ROOT_MEMORY_TMP", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 00, ff), {"ROOT_SECURITY", cyng::TC_NULL, "list some basic security information" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 01, ff), {"SECURITY_SERVER_ID", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 02, ff), {"SECURITY_OWNER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 05, ff), {"SECURITY_05", cyng::TC_NULL, "3" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 06, 10), {"SECURITY_06_10", cyng::TC_NULL, "3" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 06, ff), {"SECURITY_06", cyng::TC_NULL, "3" }},
		{ DEFINE_OBIS(00, 80, 80, 01, 07, ff), {"SECURITY_07", cyng::TC_NULL, "1" }},
		{ DEFINE_OBIS(00, 80, 80, 10, 00, 01), {"CLASS_OP_LSM_STATUS", cyng::TC_NULL, "LSM status" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 00, ff), {"ACTUATORS", cyng::TC_NULL, "list of actuators" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 10, ff), {"CLASS_OP_LSM_ACTOR_ID", cyng::TC_NULL, "ServerID des Aktors (uint16)" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 11, 01), {"CLASS_OP_LSM_SWITCH", cyng::TC_NULL, "Schaltanforderung" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 11, ff), {"CLASS_OP_LSM_CONNECT", cyng::TC_NULL, "Connection state" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 13, ff), {"CLASS_OP_LSM_FEEDBACK", cyng::TC_NULL, "feedback configuration" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 14, ff), {"CLASS_OP_LSM_LOAD", cyng::TC_NULL, "number of loads" }},
		{ DEFINE_OBIS(00, 80, 80, 11, 15, ff), {"CLASS_OP_LSM_POWER", cyng::TC_NULL, "total power" }},
		{ DEFINE_OBIS(00, 80, 80, 11, a0, ff), {"CLASS_STATUS", cyng::TC_NULL, "see: 2.2.1.3 Status der Aktoren (Kontakte)" }},
		{ DEFINE_OBIS(00, 80, 80, 11, a1, ff), {"CLASS_OP_LSM_VERSION", cyng::TC_NULL, "LSM version" }},
		{ DEFINE_OBIS(00, 80, 80, 11, a2, ff), {"CLASS_OP_LSM_TYPE", cyng::TC_NULL, "LSM type" }},
		{ DEFINE_OBIS(00, 80, 80, 11, a9, 01), {"CLASS_OP_LSM_ACTIVE_RULESET", cyng::TC_NULL, "active ruleset" }},
		{ DEFINE_OBIS(00, 80, 80, 11, a9, 02), {"CLASS_OP_LSM_PASSIVE_RULESET", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, 80, 80, 14, 03, ff), {"CLASS_OP_LSM_JOB", cyng::TC_NULL, "Schaltauftrags ID ((octet string)	" }},
		{ DEFINE_OBIS(00, 80, 80, 14, 20, ff), {"CLASS_OP_LSM_POSITION", cyng::TC_NULL, "current position" }},
		{ DEFINE_OBIS(00, b0, 00, 02, 00, 00), {"CLASS_MBUS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(00, b0, 00, 02, 00, 01), {"CLASS_MBUS_RO_INTERVAL", cyng::TC_NULL, "readout interval in seconds % 3600" }},
		{ DEFINE_OBIS(00, b0, 00, 02, 00, 02), {"CLASS_MBUS_SEARCH_INTERVAL", cyng::TC_NULL, "search interval in seconds" }},
		{ DEFINE_OBIS(00, b0, 00, 02, 00, 03), {"CLASS_MBUS_SEARCH_DEVICE", cyng::TC_BOOL, "search device now and by restart" }},
		{ DEFINE_OBIS(00, b0, 00, 02, 00, 04), {"CLASS_MBUS_AUTO_ACTIVATE", cyng::TC_BOOL, "automatic activation of meters" }},
		{ DEFINE_OBIS(00, b0, 00, 02, 00, 05), {"CLASS_MBUS_BITRATE", cyng::TC_NULL, "used baud rates(bitmap)" }},

		{ DEFINE_OBIS(01, 00, 00, 00, 00, ff), {"SERVER_ID_1_1", cyng::TC_NULL, "Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)" }},
		{ DEFINE_OBIS(01, 00, 00, 00, 01, ff), {"SERVER_ID_1_2", cyng::TC_NULL, "Identifikationsnummer 1.2" }},
		{ DEFINE_OBIS(01, 00, 00, 00, 02, ff), {"SERVER_ID_1_3", cyng::TC_NULL, "Identifikationsnummer 1.3" }},
		{ DEFINE_OBIS(01, 00, 00, 00, 03, ff), {"SERVER_ID_1_4", cyng::TC_NULL, "Identifikationsnummer 1.4" }},
		{ DEFINE_OBIS(01, 00, 00, 00, 09, ff), {"DEVICE_ID", cyng::TC_NULL, "encode profiles" }},
		{ DEFINE_OBIS(01, 00, 00, 02, 00, ff), {"SOFTWARE_ID", cyng::TC_BUFFER, "" }},
		{ DEFINE_OBIS(01, 00, 00, 09, 0b, 00), {"CURRENT_UTC", cyng::TC_NULL, "readout time in UTC" }},

		//	EDL21
		{ DEFINE_OBIS(01, 00, 01, 08, 00, 60), {"EDL21_LAST_24h", cyng::TC_NULL, "Consumption over the last 24 hours" }},
		{ DEFINE_OBIS(01, 00, 01, 08, 00, 61), {"EDL21_LAST_7d", cyng::TC_NULL, "Consumption over the last 7 days" }},
		{ DEFINE_OBIS(01, 00, 01, 08, 00, 62), {"EDL21_LAST_30d", cyng::TC_NULL, "Consumption over the last 30 days" }},
		{ DEFINE_OBIS(01, 00, 01, 08, 00, 63), {"EDL21_LAST_365d", cyng::TC_NULL, "Consumption over the last 365 days" }},
		{ DEFINE_OBIS(01, 00, 01, 08, 00, 64), {"EDL21_LAST_RESET", cyng::TC_NULL, "Consumption since last reset" }},

		//	SMART METER Register
		{ DEFINE_OBIS(01, 01, 62, 17, 00, FF), {"REG_SM_POWER_THRESHOLD", cyng::TC_NULL, "Power threshold (D.23.0)" }},

		{ DEFINE_OBIS(81, 00, 00, 09, 0b, 00), {"ACT_SENSOR_TIME", cyng::TC_NULL, "actSensorTime - current UTC time" }},
		{ DEFINE_OBIS(81, 00, 00, 09, 0b, 01), {"TZ_OFFSET", cyng::TC_UINT16, "offset to actual time zone in minutes (-720 .. +720)" }},
		{ DEFINE_OBIS(81, 00, 60, 05, 00, 00), {"CLASS_OP_LOG_STATUS_WORD", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 01, 00, 00, 00, ff), {"LOG_SOURCE_ETH_AUX", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 01, 00, 00, 01, 00), {"INTERFACE_01_NAME", cyng::TC_STRING, "interface name" }},
		{ DEFINE_OBIS(81, 02, 00, 00, 00, ff), {"LOG_SOURCE_ETH_CUSTOM", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 02, 00, 00, 01, 00), {"INTERFACE_02_NAME", cyng::TC_STRING, "interface name" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 00, ff), {"ROOT_CUSTOM_INTERFACE", cyng::TC_NULL, "see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 01, ff), {"CUSTOM_IF_IP_REF", cyng::TC_UINT8, "0 == manual, 1 == DHCP" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 02, 01), {"CUSTOM_IF_DHCP_LOCAL_IP_MASK", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 02, 02), {"CUSTOM_IF_DHCP_DEFAULT_GW", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 02, 03), {"CUSTOM_IF_DHCP_DNS", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 02, 04), {"CUSTOM_IF_DHCP_START_ADDRESS", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 02, 05), {"CUSTOM_IF_DHCP_END_ADDRESS", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 02, ff), {"CUSTOM_IF_DHCP", cyng::TC_BOOL, "if true use a DHCP server" }},
		{ DEFINE_OBIS(81, 02, 00, 07, 10, ff), {"ROOT_CUSTOM_PARAM", cyng::TC_NULL, "see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle " }},
		{ DEFINE_OBIS(81, 02, 17, 07, 00, 00), {"CUSTOM_IF_IP_CURRENT_1", cyng::TC_IP_ADDRESS, "current IP address (customer interface)" }},
		{ DEFINE_OBIS(81, 02, 17, 07, 00, 01), {"CUSTOM_IF_IP_ADDRESS_1", cyng::TC_IP_ADDRESS, "first manual set IP address" }},
		{ DEFINE_OBIS(81, 02, 17, 07, 00, 02), {"CUSTOM_IF_IP_ADDRESS_2", cyng::TC_IP_ADDRESS, "second manual set IP address" }},
		{ DEFINE_OBIS(81, 02, 17, 07, 01, 01), {"CUSTOM_IF_IP_MASK_1", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 02, 17, 07, 01, 02), {"CUSTOM_IF_IP_MASK_2", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 03, 00, 00, 00, ff), {"LOG_SOURCE_RS232", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 03, 00, 00, 01, 00), {"INTERFACE_03_NAME", cyng::TC_STRING, "interface name" }},
		{ DEFINE_OBIS(81, 04, 00, 00, 00, ff), {"LOG_SOURCE_ETH", cyng::TC_NULL, "WAN interface" }},
		{ DEFINE_OBIS(81, 04, 00, 00, 01, 00), {"INTERFACE_04_NAME", cyng::TC_STRING, "interface name" }},
		{ DEFINE_OBIS(81, 04, 00, 06, 00, ff), {"ROOT_WAN", cyng::TC_NULL, "see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status " }},
		{ DEFINE_OBIS(81, 04, 00, 07, 00, ff), {"ROOT_WAN_PARAM", cyng::TC_NULL, "see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter " }},
		{ DEFINE_OBIS(81, 04, 02, 07, 00, ff), {"ROOT_GSM", cyng::TC_NULL, "see: Datenstruktur zum Lesen/Setzen der GSM Parameter " }},
		{ DEFINE_OBIS(81, 04, 0d, 06, 00, 00), {"CLASS_OP_LOG_PROVIDER", cyng::TC_UINT32, "	aktueller Provider-Identifier(uint32)" }},
		{ DEFINE_OBIS(81, 04, 0d, 06, 00, ff), {"GSM_ADMISSIBLE_OPERATOR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 04, 0d, 07, 00, ff), {"ROOT_GPRS_PARAM", cyng::TC_NULL, "see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter " }},
		{ DEFINE_OBIS(81, 04, 0d, 08, 00, ff), {"ROOT_GSM_STATUS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 04, 0e, 06, 00, ff), {"PLC_STATUS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 04, 17, 07, 00, 00), {"CLASS_OP_LOG_AREA_CODE", cyng::TC_UINT16, "aktueller Location - oder Areacode" }},
		{ DEFINE_OBIS(81, 04, 18, 07, 00, ff), {"IF_PLC", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 04, 1a, 07, 00, 00), {"CLASS_OP_LOG_CELL", cyng::TC_UINT16, "aktuelle Zelleninformation" }},
		{ DEFINE_OBIS(81, 04, 2b, 07, 00, 00), {"CLASS_OP_LOG_FIELD_STRENGTH", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 05, 00, 00, 00, ff), {"LOG_SOURCE_eHZ", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 05, 0d, 07, 00, 01), {"IF_EDL_PROTOCOL", cyng::TC_UINT8, "always 1" }},
		{ DEFINE_OBIS(81, 05, 0d, 07, 00, 02), {"IF_EDL_BAUDRATE", cyng::TC_NULL, "0 = auto, 6 = 9600, 10 = 115200 baud" }},
		{ DEFINE_OBIS(81, 05, 0d, 07, 00, ff), {"IF_EDL", cyng::TC_NULL, "M-Bus EDL (RJ10)" }},
		{ DEFINE_OBIS(81, 06, 00, 00, 00, ff), {"LOG_SOURCE_wMBUS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 00, 00, 01, 00), {"W_MBUS_ADAPTER_MANUFACTURER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 00, 00, 03, 00), {"W_MBUS_ADAPTER_ID", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 00, 02, 00, 00), {"W_MBUS_FIRMWARE", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 06, 00, 02, 03, ff), {"W_MBUS_HARDWARE", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 06, 00, 02, 04, ff), {"W_MBUS_FIELD_STRENGTH", cyng::TC_UINT16, "dbm" }},
		{ DEFINE_OBIS(81, 06, 00, 03, 74, ff), {"W_MBUS_LAST_RECEPTION", cyng::TC_NULL, "Time since last radio telegram reception" }},
		{ DEFINE_OBIS(81, 06, 0f, 06, 00, ff), {"ROOT_W_MBUS_STATUS", cyng::TC_NULL, "see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status " }},
		{ DEFINE_OBIS(81, 06, 19, 07, 00, ff), {"IF_wMBUS", cyng::TC_NULL, "Wireless M-BUS" }},
		{ DEFINE_OBIS(81, 06, 19, 07, 01, ff), {"W_MBUS_PROTOCOL", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 19, 07, 02, ff), {"W_MBUS_MODE_S", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 19, 07, 03, ff), {"W_MBUS_MODE_T", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 19, 07, 04, ff), {"W_MBUS_POWER", cyng::TC_UINT8, "0 = default, 1 = low, 2 = medium, 3 = high" }},
		{ DEFINE_OBIS(81, 06, 19, 07, 11, ff), {"W_MBUS_INSTALL_MODE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 06, 27, 32, 03, 01), {"W_MBUS_REBOOT", cyng::TC_UINT32, "" }},
		{ DEFINE_OBIS(81, 06, 64, 3c, 01, 01), {"W_MBUS_MAX_MSG_TIMEOUT", cyng::TC_UINT8, "" }},
		{ DEFINE_OBIS(81, 06, 64, 3c, 01, 04), {"W_MBUS_MAX_SML_TIMEOUT_IN", cyng::TC_UINT16, "max timeout between SML close request and SML open response from device to gateway" }},
		{ DEFINE_OBIS(81, 06, 64, 3c, 01, 05), {"W_MBUS_MAX_SML_TIMEOUT_OUT", cyng::TC_UINT16, "max timeout between SML close request and SML open response from gateway to device" }},
		{ DEFINE_OBIS(81, 41, 00, 00, 00, ff), {"LOG_SOURCE_IP", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 42, 00, 00, 00, 01), {"LOG_SOURCE_SML_EXT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 42, 00, 00, 00, 02), {"LOG_SOURCE_SML_CUSTOM", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 42, 00, 00, 00, 03), {"LOG_SOURCE_SML_SERVICE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 42, 00, 00, 00, 04), {"LOG_SOURCE_SML_WAN", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 42, 00, 00, 00, 05), {"LOG_SOURCE_SML_eHZ", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 42, 00, 00, 00, 06), {"LOG_SOURCE_SML_wMBUS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 45, 00, 00, 00, ff), {"LOG_SOURCE_PUSH_SML", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 46, 00, 00, 00, ff), {"LOG_SOURCE_PUSH_IPT_SOURCE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 46, 00, 00, 02, ff), {"PROFILE_ADDRESS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 47, 00, 00, 00, ff), {"LOG_SOURCE_PUSH_IPT_SINK", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 47, 17, 07, 00, ff), {"PUSH_TARGET", cyng::TC_NULL, "push target name" }},
		{ DEFINE_OBIS(81, 48, 00, 00, 00, 00), {"COMPUTER_NAME", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 48, 00, 00, 00, 01), {"LOG_SOURCE_WAN_DHCP", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 48, 00, 00, 00, 02), {"LOG_SOURCE_WAN_IP", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 48, 00, 00, 00, 03), {"LOG_SOURCE_WAN_PPPoE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 48, 00, 32, 02, 01), {"LAN_DHCP_ENABLED", cyng::TC_BOOL, "" }},
		{ DEFINE_OBIS(81, 48, 0d, 06, 00, ff), {"ROOT_LAN_DSL", cyng::TC_NULL, "see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 00, 00), {"CODE_IF_LAN_ADDRESS", cyng::TC_IP_ADDRESS, "IPv4 or IPv6 address" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 00, ff), {"IF_LAN_DSL", cyng::TC_NULL, "see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 01, 00), {"CODE_IF_LAN_SUBNET_MASK", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 02, 00), {"CODE_IF_LAN_GATEWAY", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 04, 00), {"CODE_IF_LAN_DNS_PRIMARY", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 05, 00), {"CODE_IF_LAN_DNS_SECONDARY", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 48, 17, 07, 06, 00), {"CODE_IF_LAN_DNS_TERTIARY", cyng::TC_IP_ADDRESS, "" }},
		{ DEFINE_OBIS(81, 48, 27, 32, 06, 01), {"TCP_WAIT_TO_RECONNECT", cyng::TC_UINT8, "" }},
		{ DEFINE_OBIS(81, 48, 31, 32, 02, 01), {"TCP_CONNECT_RETRIES", cyng::TC_UINT32, "" }},
		{ DEFINE_OBIS(81, 49, 00, 00, 00, 01), {"LOG_SOURCE_WAN_IPT_CONTROLLER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 49, 00, 00, 00, 02), {"LOG_SOURCE_WAN_IPT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 49, 00, 00, 10, ff), {"PUSH_SERVICE", cyng::TC_NULL, "options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX" }},
		{ DEFINE_OBIS(81, 49, 0d, 06, 00, ff), {"ROOT_IPT_STATE", cyng::TC_NULL, "see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status " }},
		{ DEFINE_OBIS(81, 49, 0d, 07, 00, ff), {"ROOT_IPT_PARAM", cyng::TC_NULL, "see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter " }},
		{ DEFINE_OBIS(81, 49, 17, 07, 00, 00), {"TARGET_IP_ADDRESS", cyng::TC_IP_ADDRESS, "ip address of IP-T master " }},
		{ DEFINE_OBIS(81, 49, 19, 07, 00, 00), {"SOURCE_PORT", cyng::TC_UINT16, "target port of IP-T master " }},
		{ DEFINE_OBIS(81, 49, 1a, 07, 00, 00), {"TARGET_PORT", cyng::TC_UINT16, "source port of IP-T master " }},
		{ DEFINE_OBIS(81, 49, 63, 3c, 01, 00), {"IPT_ACCOUNT", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 49, 63, 3c, 02, 00), {"IPT_PASSWORD", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 49, 63, 3c, 03, 00), {"IPT_SCRAMBLED", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 4a, 00, 00, 00, ff), {"LOG_SOURCE_WAN_NTP", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 01), {"PEER_OBISLOG", cyng::TC_NULL, "peer address: OBISLOG" }},
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 02), {"PEER_SCM", cyng::TC_NULL, "peer address: SCM (power)" }},
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 10), {"PEER_EHZIF", cyng::TC_NULL, "peer address: EHZIF" } },
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 11), {"PEER_USERIF", cyng::TC_NULL, "peer address: USERIF" }},
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 12), {"PEER_WMBUS", cyng::TC_NULL, "internal MUC software function" } },
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 13), {"PEER_ADDRESS_WANGSM", cyng::TC_NULL, "peer address: WAN/GSM" }},
		{ DEFINE_OBIS(81, 81, 00, 00, 00, 14), {"PEER_WANPLC", cyng::TC_NULL, "internal MUC software function" } },
		{ DEFINE_OBIS(81, 81, 00, 00, 00, ff), {"PEER_ADDRESS", cyng::TC_NULL, "unit is 255" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 00, 00), {"VERSION", cyng::TC_STRING, "(0.2.0) firmware revision" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 00, 01), {"SET_START_FW_UPDATE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 00, 02), {"FILE_NAME", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 00, 03), {"MSG_COUNTER", cyng::TC_UINT32, "Anzahl aller Nachrichten zur Übertragung des Binary" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 00, 04), {"LAST_MSG", cyng::TC_UINT32, "Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 00, 05), {"MSG_NUMBER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 02, ff), {"BLOCK_NUMBER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 00, 02, 03, ff), {"BINARY_DATA", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 00, 03, 00, 01), {"SET_DISPATCH_FW_UPDATE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 10, 00, 00, 01), {"LOG_SOURCE_LOG", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 10, 00, 00, 02), {"LOG_SOURCE_SCM", cyng::TC_NULL, "internal software function" }},
		{ DEFINE_OBIS(81, 81, 10, 00, 00, 03), {"LOG_SOURCE_UPDATE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 10, 00, 00, 04), {"LOG_SOURCE_SMLC", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 10, 00, 00, 0c), {"LOG_SOURCE_LEDIO", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 10, 00, 00, 14), {"LOG_SOURCE_WANPLC", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 10, 06, 01, ff), {"CODE_LIST_1_VISIBLE_DEVICES", cyng::TC_NULL, "1. Liste der sichtbaren Sensoren" }},
		{ DEFINE_OBIS(81, 81, 10, 06, ff, ff), {"ROOT_VISIBLE_DEVICES", cyng::TC_NULL, "visible devices (Liste der sichtbaren Sensoren)" }},
		{ DEFINE_OBIS(81, 81, 10, 16, ff, ff), {"ROOT_NEW_DEVICES", cyng::TC_NULL, "new active devices" }},
		{ DEFINE_OBIS(81, 81, 10, 26, ff, ff), {"ROOT_INVISIBLE_DEVICES", cyng::TC_NULL, "not longer visible devices" }},
		{ DEFINE_OBIS(81, 81, 11, 06, 01, ff), {"CODE_LIST_1_ACTIVE_DEVICES", cyng::TC_NULL, "1. Liste der aktiven Sensoren)" }},
		{ DEFINE_OBIS(81, 81, 11, 06, fb, ff), {"ACTIVATE_DEVICE", cyng::TC_NULL, "activate meter/sensor" }},
		{ DEFINE_OBIS(81, 81, 11, 06, fc, ff), {"DEACTIVATE_DEVICE", cyng::TC_NULL, "deactivate meter/sensor" }},
		{ DEFINE_OBIS(81, 81, 11, 06, fd, ff), {"DELETE_DEVICE", cyng::TC_NULL, "delete meter/sensor" }},
		{ DEFINE_OBIS(81, 81, 11, 06, ff, ff), {"ROOT_ACTIVE_DEVICES", cyng::TC_NULL, "active devices (Liste der aktiven Sensoren)" }},
		{ DEFINE_OBIS(81, 81, 12, 06, ff, ff), {"ROOT_DEVICE_INFO", cyng::TC_NULL, "extended device information" }},
		{ DEFINE_OBIS(81, 81, 27, 32, 07, 01), {"OBISLOG_INTERVAL", cyng::TC_UINT16, " Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook" }},
		{ DEFINE_OBIS(81, 81, 61, 3c, 01, ff), {"DATA_USER_NAME", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 61, 3c, 02, ff), {"DATA_USER_PWD", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, 81, 60, ff, ff), {"ROOT_ACCESS_RIGHTS", cyng::TC_NULL, "see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte " }},
		{ DEFINE_OBIS(81, 81, 81, 61, ff, ff), {"ACCESS_USER_NAME", cyng::TC_NULL, "user name for access" }},
		{ DEFINE_OBIS(81, 81, 81, 62, ff, ff), {"ACCESS_PASSWORD", cyng::TC_NULL, "SHA256 encrypted" }},
		{ DEFINE_OBIS(81, 81, 81, 63, ff, ff), {"ACCESS_PUBLIC_KEY", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 81, 01, ff), {"ROOT_FILE_TRANSFER", cyng::TC_NULL, "7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) " }},
		{ DEFINE_OBIS(81, 81, c7, 81, 0c, ff), {"DATA_FILENAME", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 81, 0d, ff), {"DATA_APPLICATION", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 81, 0e, ff), {"DATA_FIRMWARE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 81, 0f, ff), {"DATA_FILENAME_INDIRECT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 81, 10, ff), {"DATA_APPLICATION_INDIRECT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 81, 23, ff), {"DATA_PUSH_DETAILS", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 01, ff), {"ROOT_DEVICE_IDENT", cyng::TC_NULL, "see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) " }},
		{ DEFINE_OBIS(81, 81, c7, 82, 02, ff), {"DEVICE_CLASS", cyng::TC_NULL, "Geräteklasse (OBIS code or '2D 2D 2D')" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 03, ff), {"DATA_MANUFACTURER", cyng::TC_STRING, "FLAG" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 04, ff), {"SERVER_ID", cyng::TC_NULL, "Server ID " }},
		{ DEFINE_OBIS(81, 81, c7, 82, 05, ff), {"DATA_PUBLIC_KEY", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 06, ff), {"ROOT_FIRMWARE", cyng::TC_NULL, "Firmware" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 08, ff), {"DEVICE_KERNEL", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 09, ff), {"HARDWARE_FEATURES", cyng::TC_NULL, "hardware equipment (charge, type, ...) 81 81 C7 82 0A NN" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 0e, ff), {"DEVICE_ACTIVATED", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 41, ff), {"DEV_CLASS_BASIC_DIRECT", cyng::TC_NULL, "3 x 230 /400 V and 5 (100) A " }},
		{ DEFINE_OBIS(81, 81, c7, 82, 42, ff), {"DEV_CLASS_BASIC_SEMI", cyng::TC_NULL, "3 x 230 /400 V and 1 (6) A" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 43, ff), {"DEV_CLASS_BASIC_INDIRECT", cyng::TC_NULL, "3 x  58 / 100 V and 1 (6) A " }},
		{ DEFINE_OBIS(81, 81, c7, 82, 45, ff), {"DEV_CLASS_IW", cyng::TC_NULL, "IW module" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 46, ff), {"DEV_CLASS_PSTN", cyng::TC_NULL, "PSTN/GPRS" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 47, ff), {"DEV_CLASS_GPRS", cyng::TC_NULL, "GPRS/PLC" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 48, ff), {"DEV_CLASS_KM", cyng::TC_NULL, "KM module (LAN/DSL)" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 49, ff), {"DEV_CLASS_NK", cyng::TC_NULL, "NK/HS" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 4a, ff), {"DEV_CLASS_EXTERN", cyng::TC_NULL, "external load profile collector " }},
		{ DEFINE_OBIS(81, 81, c7, 82, 4b, ff), {"DEV_CLASS_GW", cyng::TC_NULL, "gateway" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 4f, ff), {"DEV_CLASS_LAN", cyng::TC_NULL, "see DEV_CLASS_MUC_LAN" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 50, ff), {"DEV_CLASS_eHZ", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 52, ff), {"DEV_CLASS_3HZ", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 53, ff), {"DEV_CLASS_MUC_LAN", cyng::TC_NULL, "(MUC-LAN/DSL)" }},
		{ DEFINE_OBIS(81, 81, c7, 82, 81, ff), {"DATA_IP_ADDRESS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 83, 82, 01), {"REBOOT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 83, 82, 05), {"CLEAR_DATA_COLLECTOR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 83, 82, 07), {"UPDATE_FW", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 83, 82, 0e), {"SET_ACTIVATE_FW", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 00, ff), {"ROOT_SENSOR_PARAMS", cyng::TC_NULL, "data mirror root element (Eigenschaften eines Datenspiegels)" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 01, ff), {"ROOT_SENSOR_BITMASK", cyng::TC_UINT16, " Bitmask to define bits that will be transferred into log" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 02, ff), {"AVERAGE_TIME_MS", cyng::TC_NULL, "average time between two received data records (milliseconds)" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 03, ff), {"DATA_AES_KEY", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 04, ff), {"TIME_REFERENCE", cyng::TC_UINT8, "0 == UTC, 1 == UTC + time zone, 2 == local time" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 10, ff), {"PROFILE_1_MINUTE", cyng::TC_NULL, "1 minute" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 11, ff), {"PROFILE_15_MINUTE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 12, ff), {"PROFILE_60_MINUTE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 13, ff), {"PROFILE_24_HOUR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 14, ff), {"PROFILE_LAST_2_HOURS", cyng::TC_NULL, "past two hours" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 15, ff), {"PROFILE_LAST_WEEK", cyng::TC_NULL, "weekly (on day change from Sunday to Monday)" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 16, ff), {"PROFILE_1_MONTH", cyng::TC_NULL, "monthly recorded meter readings" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 17, ff), {"PROFILE_1_YEAR", cyng::TC_NULL, "annually recorded meter readings" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 18, ff), {"PROFILE_INITIAL", cyng::TC_NULL, "81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 20, ff), {"ROOT_DATA_COLLECTOR", cyng::TC_NULL, "data collector root element (Eigenschaften eines Datensammlers)" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 21, ff), {"DATA_COLLECTOR_ACTIVE", cyng::TC_BOOL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 86, 22, ff), {"DATA_COLLECTOR_SIZE", cyng::TC_NULL, "max. table size" }},
		{ DEFINE_OBIS(81, 81, c7, 87, 81, ff), {"DATA_REGISTER_PERIOD", cyng::TC_UINT32, "register period in seconds (0 == event driven)" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 01, ff), {"ROOT_NTP", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 02, ff), {"CODE_NTP_SERVER", cyng::TC_NULL, "List of NTP servers" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 03, ff), {"CODE_NTP_PORT", cyng::TC_UINT16, "NTP port (123)" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 04, ff), {"CODE_NTP_TZ", cyng::TC_UINT32, "timezone" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 05, ff), {"CODE_NTP_OFFSET", cyng::TC_NULL, "Offset to transmission of the signal for synchronization" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 06, ff), {"CODE_NTP_ACTIVE", cyng::TC_NULL, "NTP enabled/disables" }},
		{ DEFINE_OBIS(81, 81, c7, 88, 10, ff), {"ROOT_DEVICE_TIME", cyng::TC_NULL, "device time" }},
		{ DEFINE_OBIS(81, 81, c7, 89, e1, ff), {"CLASS_OP_LOG", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 89, e2, ff), {"CLASS_EVENT", cyng::TC_UINT32, "event" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 01, ff), {"ROOT_PUSH_OPERATIONS", cyng::TC_NULL, "push root element" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 02, ff), {"PUSH_INTERVAL", cyng::TC_NULL, "in seconds" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 03, ff), {"PUSH_DELAY", cyng::TC_NULL, "in seconds" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 04, ff), {"PUSH_SOURCE", cyng::TC_NULL, "options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 21, ff), {"PUSH_SERVICE_IPT", cyng::TC_NULL, "SML data response without request - typical IP - T push" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 22, ff), {"PUSH_SERVICE_SML", cyng::TC_NULL, "SML data response without request - target is SML client address" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 23, ff), {"DATA_COLLECTOR_REGISTER", cyng::TC_NULL, "OBIS list (data mirror)" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 42, ff), {"PUSH_SOURCE_PROFILE", cyng::TC_NULL, "new meter/sensor data" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 43, ff), {"PUSH_SOURCE_INSTALL", cyng::TC_NULL, "configuration changed" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 44, ff), {"PUSH_SOURCE_VISIBLE_SENSORS", cyng::TC_NULL, "list of visible meters changed" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 45, ff), {"PUSH_SOURCE_ACTIVE_SENSORS", cyng::TC_NULL, "list of active meters changed" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 81, ff), {"PUSH_SERVER_ID", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 82, ff), {"PUSH_IDENTIFIERS", cyng::TC_NULL, "list of identifiers of the values to be delivered by the push source" }},
		{ DEFINE_OBIS(81, 81, c7, 8a, 83, ff), {"PROFILE", cyng::TC_NULL, "encode profiles" }},
		{ DEFINE_OBIS(81, 81, c7, 90, 00, ff), {"ROOT_IF", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 00, ff), {"IF_1107", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 01, ff), {"IF_1107_ACTIVE", cyng::TC_BOOL, " if true 1107 interface active otherwise SML interface active" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 02, ff), {"IF_1107_LOOP_TIME", cyng::TC_NULL, "Loop timeout in seconds" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 03, ff), {"IF_1107_RETRIES", cyng::TC_NULL, "Retry count" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 04, ff), {"IF_1107_MIN_TIMEOUT", cyng::TC_NULL, "Minimal answer timeout(300)" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 05, ff), {"IF_1107_MAX_TIMEOUT", cyng::TC_NULL, " Maximal answer timeout(5000)" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 06, ff), {"IF_1107_MAX_DATA_RATE", cyng::TC_NULL, "Maximum data bytes(10240)" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 07, ff), {"IF_1107_RS485", cyng::TC_BOOL, "if true RS 485, otherwise RS 323" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 08, ff), {"IF_1107_PROTOCOL_MODE", cyng::TC_UINT8, "Protocol mode(A ... D)" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 09, ff), {"IF_1107_METER_LIST", cyng::TC_NULL, "Liste der abzufragenden 1107 Zähler" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 0a, ff), {"IF_1107_METER_ID", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 0b, ff), {"IF_1107_BAUDRATE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 0c, ff), {"IF_1107_ADDRESS", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 0d, ff), {"IF_1107_P1", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 0e, ff), {"IF_1107_W5", cyng::TC_STRING, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 10, ff), {"IF_1107_AUTO_ACTIVATION", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 11, ff), {"IF_1107_TIME_GRID", cyng::TC_NULL, "time grid of load profile readout in seconds" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 13, ff), {"IF_1107_TIME_SYNC", cyng::TC_NULL, "time sync in seconds" }},
		{ DEFINE_OBIS(81, 81, c7, 93, 14, ff), {"IF_1107_MAX_VARIATION", cyng::TC_NULL, "seconds" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fd, 00), {"ATTENTION_OK", cyng::TC_NULL, "no error" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fd, 01), {"ATTENTION_JOB_IS_RUNNINNG", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 00), {"ATTENTION_UNKNOWN_ERROR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 01), {"ATTENTION_UNKNOWN_SML_ID", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 02), {"ATTENTION_NOT_AUTHORIZED", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 03), {"ATTENTION_NO_SERVER_ID", cyng::TC_NULL, "unable to find recipient for request" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 04), {"ATTENTION_NO_REQ_FIELD", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 05), {"ATTENTION_CANNOT_WRITE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 06), {"ATTENTION_CANNOT_READ", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 07), {"ATTENTION_COMM_ERROR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 08), {"ATTENTION_PARSER_ERROR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 09), {"ATTENTION_OUT_OF_RANGE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 0a), {"ATTENTION_NOT_EXECUTED", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 0b), {"ATTENTION_INVALID_CRC", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 0c), {"ATTENTION_NO_BROADCAST", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 0d), {"ATTENTION_UNEXPECTED_MSG", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 0e), {"ATTENTION_UNKNOWN_OBIS_CODE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 0f), {"ATTENTION_UNSUPPORTED_DATA_TYPE", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 10), {"ATTENTION_ELEMENT_NOT_OPTIONAL", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 11), {"ATTENTION_NO_ENTRIES", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 12), {"ATTENTION_END_LIMIT_BEFORE_START", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 13), {"ATTENTION_NO_ENTRIES_IN_RANGE", cyng::TC_NULL, "range is empty - not the profile" }},
		{ DEFINE_OBIS(81, 81, c7, c7, fe, 14), {"ATTENTION_MISSING_CLOSE_MSG", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 00, ff), {"ROOT_BROKER", cyng::TC_NULL, "90 00 00 00 00 NN - broker list" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 01, ff), {"BROKER_LOGIN", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 02, ff), {"BROKER_SERVER", cyng::TC_NULL, "ip address" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 03, ff), {"BROKER_SERVICE", cyng::TC_UINT16, "port" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 04, ff), {"BROKER_USER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 05, ff), {"BROKER_PWD", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(90, 00, 00, 00, 06, ff), {"BROKER_TIMEOUT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(90, 00, 00, 00, a0, ff), {"BROKER_BLOCKLIST", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 00, ff), {"ROOT_SERIAL", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 01, ff), {"SERIAL_NAME", cyng::TC_NULL, "example: /dev/ttyAPP0" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 02, ff), {"SERIAL_DATABITS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 03, ff), {"SERIAL_PARITY", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 04, ff), {"SERIAL_FLOW_CONTROL", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 05, ff), {"SERIAL_STOPBITS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 06, ff), {"SERIAL_SPEED", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(91, 00, 00, 00, 07, ff), {"SERIAL_TASK", cyng::TC_NULL, "LMN port task" }},
		{ DEFINE_OBIS(92, 00, 00, 00, 00, ff), {"ROOT_NMS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(92, 00, 00, 00, 01, ff), {"NMS_ADDRESS", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(92, 00, 00, 00, 02, ff), {"NMS_PORT", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(92, 00, 00, 00, 03, ff), {"NMS_USER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(92, 00, 00, 00, 04, ff), {"NMS_PWD", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(92, 00, 00, 00, 05, ff), {"NMS_ENABLED", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(93, 00, 00, 00, 00, ff), {"ROOT_REDIRECTOR", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(93, 00, 00, 00, 01, ff), {"REDIRECTOR_LOGIN", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(93, 00, 00, 00, 02, ff), {"REDIRECTOR_ADDRESS", cyng::TC_NULL, "ip address" }},
		{ DEFINE_OBIS(93, 00, 00, 00, 03, ff), {"REDIRECTOR_SERVICE", cyng::TC_UINT16, "port" }},
		{ DEFINE_OBIS(93, 00, 00, 00, 04, ff), {"REDIRECTOR_USER", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(93, 00, 00, 00, 05, ff), {"REDIRECTOR_PWD", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(99, 00, 00, 00, 00, 03), {"LIST_CURRENT_DATA_RECORD", cyng::TC_NULL, "current data set" }},
		{ DEFINE_OBIS(99, 00, 00, 00, 00, 04), {"LIST_SERVICES", cyng::TC_NULL, "" }},
		{ DEFINE_OBIS(99, 00, 00, 00, 00, 05), {"FTP_UPDATE", cyng::TC_NULL, "" }}
	};

	//
	//	add EDIS codes
	//
	for (auto const& edx : edis_map) {
		//{ DEFINE_OBIS(00, 00, 00, 00, 00, ff), { "METER_ADDRESS", cyng::TC_NULL, "" }}
		cyng::obis const o(1
			, 0
			, edx.first[cyng::edis::value_group::VG_INDICATOR]
			, edx.first[cyng::edis::value_group::VG_MODE]
			, edx.first[cyng::edis::value_group::VG_QUANTITY]
			, 0xff);
		auto const r = obis_map.emplace(o, obis_ctx( edx.second.get_short(), cyng::TC_NULL, edx.second.get_name()));
		if (r.second) {
			std::cout << "edis " << edx.second.get_short() << ": " << edx.second.get_name() << std::endl;
		}
		else {
			using cyng::operator<<;
			std::cout << "edis " << edx.first << " failed"  << std::endl;
		}
	}

	std::ofstream ofs(__OUTPUT_PATH, std::ios::trunc);
	if (ofs.is_open()) {

		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		std::time_t start_time = std::chrono::system_clock::to_time_t(now);
		char buffer[100];
		struct tm buf;
		errno_t err = localtime_s(&buf, &start_time);
		if (std::strftime(buffer, sizeof(buffer), "%F %H:%M:%S", &buf)) {
			ofs 
				<< "\t// generated at " 
				<< buffer 
				<< std::endl
				;
		}

		ofs
			<< "\t// "
			<< obis_map.size()
			<< " OBIS codes ("
			<< (obis_map.size() * 6u)
			<< " Bytes)"
			<< std::endl
			<< std::endl
			;

		ofs << std::setw(2)
			<< std::setfill('0')
			<< std::hex
			;


		std::size_t counter{ 0 };
		unsigned group = std::numeric_limits<unsigned>::max();
		for (auto const& ctx : obis_map) {

			++counter;
			std::cout 
				<< counter
				<< " - "
				<< cyng::traits::names[ctx.second.get_type()]
				<< " - "
				<< ctx.second.get_name()
				<< " "
				<< ctx.second.get_desc()
				<< std::endl;

			auto const next_group = static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_MEDIUM]);
			if (group != next_group) {

				ofs 
					<< "\t// #"
					<< std::dec
					<< counter
					<< std::hex
					<< std::endl;

				switch (next_group) {
				case cyng::obis::media::MEDIA_ABSTRACT:
					ofs << "\t// Abstract objects" << std::endl;
					break;
				case cyng::obis::media::MEDIA_ELECTRICITY:
					ofs << "\t// Electricity" << std::endl;
					break;
				case cyng::obis::media::MEDIA_HEAT_COST_ALLOCATOR:
					ofs << "\t// Heat Cost Allocators" << std::endl;
					break;
				case cyng::obis::media::MEDIA_COOLING:
					ofs << "\t// Cooling" << std::endl;
					break;
				case cyng::obis::media::MEDIA_HEAT:
					ofs << "\t// Heat" << std::endl;
					break;
				case cyng::obis::media::MEDIA_GAS:
					ofs << "\t// Gas" << std::endl;
					break;
				case cyng::obis::media::MEDIA_WATER_COLD:
					ofs << "\t// Water (cold)" << std::endl;
					break;
				case cyng::obis::media::MEDIA_WATER_HOT:
					ofs << "\t// Water (hot)" << std::endl;
					break;
				case 15: 
					ofs << "\t//  other" << std::endl;
					break;
				case 16: 
					ofs << "\t// Oil" << std::endl;
					break;
				case 17: 
					ofs << "\t// Compressed air" << std::endl;
					break;
				case 18:
					ofs << "\t// Nitrogen" << std::endl;
					break;

				default:
					ofs << "\t// next group" << std::endl;
					break;
				}
				group = next_group;
			}
			
			ofs
				<< "\tOBIS_CODE_DEFINITION("
				<< std::setw(2)
				<< next_group
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_CHANNEL])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_INDICATOR])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_MODE])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_QUANTITY])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_STORAGE])
				<< ", "
				<< ctx.second.get_name()
				<< ");"
				;

			if (ctx.second.get_type() != cyng::TC_NULL) {
				ofs 
					<< "\t// "
					<< cyng::traits::names[ctx.second.get_type()];
			}
			std::string desc = ctx.second.get_desc();
			if (!desc.empty()) {
				if (ctx.second.get_type() != cyng::TC_NULL) {
					ofs << " - ";
				}
				else {
					ofs	<< "\t// ";

				}
				ofs	<< ctx.second.get_desc();
			}
			ofs << std::endl;
		}

	}

#ifdef _DEBUG
	//ofs.close();
	//ofs.open("self.cpp", std::ios::trunc);
	//if (ofs.is_open())	{
	//	unsigned group = std::numeric_limits<unsigned>::max();
	//	for (auto const& ctx : obis_map) {

	//		ofs
	//			<< "\t{ DEFINE_OBIS("
	//			<< std::setw(2)
	//			<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_MEDIUM])
	//			<< ", "
	//			<< std::setw(2)
	//			<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_CHANNEL])
	//			<< ", "
	//			<< std::setw(2)
	//			<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_INDICATOR])
	//			<< ", "
	//			<< std::setw(2)
	//			<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_MODE])
	//			<< ", "
	//			<< std::setw(2)
	//			<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_QUANTITY])
	//			<< ", "
	//			<< std::setw(2)
	//			<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_STORAGE])
	//			<< "), {\""
	//			<< ctx.second.get_name()
	//			<< "\", cyng::"
	//			<< cyng::traits::names[ctx.second.get_type()]
	//			<< ", \""
	//			<< ctx.second.get_desc()
	//			<< "\" }},"
	//			<< std::endl;
	//	}
	//}

#endif

	std::filesystem::path p = std::filesystem::path(__OUTPUT_PATH).replace_filename("db.ipp");	// .replace_extension(".cpp");
	std::ofstream ofs2(p.string(), std::ios::trunc);
	if (ofs2.is_open()) {

		ofs2 << std::hex;
		//ofs2 << "switch (code) {" << std::endl;

		for (auto const& ctx : obis_map) {

			ofs2
				<< "case 0x"
				<< ctx.first.to_uint64()
				<< ": return \""
				<< ctx.second.get_name()
				<< "\";"
				;

			std::string const desc(ctx.second.get_desc());
			if (!desc.empty()) {
				ofs2
					<< "\t// "
					<< desc
					;
			}

			ofs2
				<< std::endl;

		}
		//ofs2 << "default: break;" << std::endl << "}" << std::endl;
		ofs2.close();
	}
	//std::cout << __OUTPUT_PATH << std::endl;
	return EXIT_SUCCESS;
}
