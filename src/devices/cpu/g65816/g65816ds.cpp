// license:BSD-3-Clause
// copyright-holders:Karl Stenerud
/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

G65C816 CPU Emulator V0.90

Copyright Karl Stenerud
All rights reserved.

*/


#include "emu.h"
#include "g65816ds.h"

#ifdef SEC
#undef SEC
#endif

#define ADDRESS_65816(A) ((A)&0xffffff)


namespace {

class g65816_opcode_struct
{
public:
	const char *name() const
	{
			return s_opnames[unsigned(m_name)];
	}

	bool is_call() const
	{
		switch (m_name)
		{
		case op::JSR:
		case op::JSL:
			return true;
		default:
			return false;
		}
	}

	bool is_return() const
	{
		switch (m_name)
		{
		case op::RTI:
		case op::RTL:
		case op::RTS:
			return true;
		default:
			return false;
		}
	}

	static const g65816_opcode_struct &get(unsigned char ins)
	{
		return s_opcodes[ins];
	}

	unsigned char flag;
	unsigned char ea;

protected:
	enum class op : unsigned
	{
		ADC ,  AND ,  ASL ,  BCC ,  BCS ,  BEQ ,  BIT ,  BMI ,  BNE ,  BPL ,  BRA ,
		BRK ,  BRL ,  BVC ,  BVS ,  CLC ,  CLD ,  CLI ,  CLV ,  CMP ,  COP ,  CPX ,
		CPY ,  DEA ,  DEC ,  DEX ,  DEY ,  EOR ,  INA ,  INC ,  INX ,  INY ,  JML ,
		JMP ,  JSL ,  JSR ,  LDA ,  LDX ,  LDY ,  LSR ,  MVN ,  MVP ,  NOP ,  ORA ,
		PEA ,  PEI ,  PER ,  PHA ,  PHB ,  PHD ,  PHK ,  PHP ,  PHX ,  PHY ,  PLA ,
		PLB ,  PLD ,  PLP ,  PLX ,  PLY ,  REP ,  ROL ,  ROR ,  RTI ,  RTL ,  RTS ,
		SBC ,  SEC ,  SED ,  SEI ,  SEP ,  STA ,  STP ,  STX ,  STY ,  STZ ,  TAX ,
		TAY ,  TCS ,  TCD ,  TDC ,  TRB ,  TSB ,  TSC ,  TSX ,  TXA ,  TXS ,  TXY ,
		TYA ,  TYX ,  WAI ,  WDM ,  XBA ,  XCE
	};

	g65816_opcode_struct(op n, unsigned char f, unsigned char e)
		: flag(f)
		, ea(e)
		, m_name(n)
	{
	}

	op m_name;

	static const char *const s_opnames[];
	static const g65816_opcode_struct s_opcodes[256];
};

enum
{
	IMP , ACC , RELB, RELW, IMM , A   , AI  , AL  , ALX , AX  , AXI ,
	AY  , D   , DI  , DIY , DLI , DLIY, DX  , DXI , DY  , S   , SIY ,
	SIG , MVN , MVP , PEA , PEI , PER
};

enum
{
	I, /* ignore */
	M, /* check m bit */
	X  /* check x bit */
};

const char *const g65816_opcode_struct::s_opnames[] =
{
	"ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRA",
	"BRK", "BRL", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "COP", "CPX",
	"CPY", "DEA", "DEC", "DEX", "DEY", "EOR", "INA", "INC", "INX", "INY", "JML",
	"JMP", "JSL", "JSR", "LDA", "LDX", "LDY", "LSR", "MVN", "MVP", "NOP", "ORA",
	"PEA", "PEI", "PER", "PHA", "PHB", "PHD", "PHK", "PHP", "PHX", "PHY", "PLA",
	"PLB", "PLD", "PLP", "PLX", "PLY", "REP", "ROL", "ROR", "RTI", "RTL", "RTS",
	"SBC", "SEC", "SED", "SEI", "SEP", "STA", "STP", "STX", "STY", "STZ", "TAX",
	"TAY", "TCS", "TCD", "TDC", "TRB", "TSB", "TSC", "TSX", "TXA", "TXS", "TXY",
	"TYA", "TYX", "WAI", "WDM", "XBA", "XCE"
};

const g65816_opcode_struct g65816_opcode_struct::s_opcodes[256] =
{
	{op::BRK, I, SIG }, {op::ORA, M, DXI }, {op::COP, I, SIG }, {op::ORA, M, S   },
	{op::TSB, M, D   }, {op::ORA, M, D   }, {op::ASL, M, D   }, {op::ORA, M, DLI },
	{op::PHP, I, IMP }, {op::ORA, M, IMM }, {op::ASL, M, ACC }, {op::PHD, I, IMP },
	{op::TSB, M, A   }, {op::ORA, M, A   }, {op::ASL, M, A   }, {op::ORA, M, AL  },
	{op::BPL, I, RELB}, {op::ORA, M, DIY }, {op::ORA, M, DI  }, {op::ORA, M, SIY },
	{op::TRB, M, D   }, {op::ORA, M, DX  }, {op::ASL, M, DX  }, {op::ORA, M, DLIY},
	{op::CLC, I, IMP }, {op::ORA, M, AY  }, {op::INA, I, IMP }, {op::TCS, I, IMP },
	{op::TRB, M, A   }, {op::ORA, M, AX  }, {op::ASL, M, AX  }, {op::ORA, M, ALX },
	{op::JSR, I, A   }, {op::AND, M, DXI }, {op::JSL, I, AL  }, {op::AND, M, S   },
	{op::BIT, M, D   }, {op::AND, M, D   }, {op::ROL, M, D   }, {op::AND, M, DLI },
	{op::PLP, I, IMP }, {op::AND, M, IMM }, {op::ROL, M, ACC }, {op::PLD, I, IMP },
	{op::BIT, M, A   }, {op::AND, M, A   }, {op::ROL, M, A   }, {op::AND, M, AL  },
	{op::BMI, I, RELB}, {op::AND, M, DIY }, {op::AND, M, DI  }, {op::AND, M, SIY },
	{op::BIT, M, DX  }, {op::AND, M, DX  }, {op::ROL, M, DX  }, {op::AND, M, DLIY},
	{op::SEC, I, IMP }, {op::AND, M, AY  }, {op::DEA, I, IMP }, {op::TSC, I, IMP },
	{op::BIT, M, AX  }, {op::AND, M, AX  }, {op::ROL, M, AX  }, {op::AND, M, ALX },
	{op::RTI, I, IMP }, {op::EOR, M, DXI }, {op::WDM, I, IMP }, {op::EOR, M, S   },
	{op::MVP, I, MVP }, {op::EOR, M, D   }, {op::LSR, M, D   }, {op::EOR, M, DLI },
	{op::PHA, I, IMP }, {op::EOR, M, IMM }, {op::LSR, M, ACC }, {op::PHK, I, IMP },
	{op::JMP, I, A   }, {op::EOR, M, A   }, {op::LSR, M, A   }, {op::EOR, M, AL  },
	{op::BVC, I, RELB}, {op::EOR, M, DIY }, {op::EOR, M, DI  }, {op::EOR, M, SIY },
	{op::MVN, I, MVN }, {op::EOR, M, DX  }, {op::LSR, M, DX  }, {op::EOR, M, DLIY},
	{op::CLI, I, IMP }, {op::EOR, M, AY  }, {op::PHY, I, IMP }, {op::TCD, I, IMP },
	{op::JMP, I, AL  }, {op::EOR, M, AX  }, {op::LSR, M, AX  }, {op::EOR, M, ALX },
	{op::RTS, I, IMP }, {op::ADC, M, DXI }, {op::PER, I, PER }, {op::ADC, M, S   },
	{op::STZ, M, D   }, {op::ADC, M, D   }, {op::ROR, M, D   }, {op::ADC, M, DLI },
	{op::PLA, I, IMP }, {op::ADC, M, IMM }, {op::ROR, M, ACC }, {op::RTL, I, IMP },
	{op::JMP, I, AI  }, {op::ADC, M, A   }, {op::ROR, M, A   }, {op::ADC, M, AL  },
	{op::BVS, I, RELB}, {op::ADC, M, DIY }, {op::ADC, M, DI  }, {op::ADC, M, SIY },
	{op::STZ, M, DX  }, {op::ADC, M, DX  }, {op::ROR, M, DX  }, {op::ADC, M, DLIY},
	{op::SEI, I, IMP }, {op::ADC, M, AY  }, {op::PLY, I, IMP }, {op::TDC, I, IMP },
	{op::JMP, I, AXI }, {op::ADC, M, AX  }, {op::ROR, M, AX  }, {op::ADC, M, ALX },
	{op::BRA, I, RELB}, {op::STA, M, DXI }, {op::BRL, I, RELW}, {op::STA, M, S   },
	{op::STY, X, D   }, {op::STA, M, D   }, {op::STX, X, D   }, {op::STA, M, DLI },
	{op::DEY, I, IMP }, {op::BIT, M, IMM }, {op::TXA, I, IMP }, {op::PHB, I, IMP },
	{op::STY, X, A   }, {op::STA, M, A   }, {op::STX, X, A   }, {op::STA, M, AL  },
	{op::BCC, I, RELB}, {op::STA, M, DIY }, {op::STA, M, DI  }, {op::STA, M, SIY },
	{op::STY, X, DX  }, {op::STA, M, DX  }, {op::STX, X, DY  }, {op::STA, M, DLIY},
	{op::TYA, I, IMP }, {op::STA, M, AY  }, {op::TXS, I, IMP }, {op::TXY, I, IMP },
	{op::STZ, M, A   }, {op::STA, M, AX  }, {op::STZ, M, AX  }, {op::STA, M, ALX },
	{op::LDY, X, IMM }, {op::LDA, M, DXI }, {op::LDX, X, IMM }, {op::LDA, M, S   },
	{op::LDY, X, D   }, {op::LDA, M, D   }, {op::LDX, X, D   }, {op::LDA, M, DLI },
	{op::TAY, I, IMP }, {op::LDA, M, IMM }, {op::TAX, I, IMP }, {op::PLB, I, IMP },
	{op::LDY, X, A   }, {op::LDA, M, A   }, {op::LDX, X, A   }, {op::LDA, M, AL  },
	{op::BCS, I, RELB}, {op::LDA, M, DIY }, {op::LDA, M, DI  }, {op::LDA, M, SIY },
	{op::LDY, X, DX  }, {op::LDA, M, DX  }, {op::LDX, X, DY  }, {op::LDA, M, DLIY},
	{op::CLV, I, IMP }, {op::LDA, M, AY  }, {op::TSX, I, IMP }, {op::TYX, I, IMP },
	{op::LDY, X, AX  }, {op::LDA, M, AX  }, {op::LDX, X, AY  }, {op::LDA, M, ALX },
	{op::CPY, X, IMM }, {op::CMP, M, DXI }, {op::REP, I, IMM }, {op::CMP, M, S   },
	{op::CPY, X, D   }, {op::CMP, M, D   }, {op::DEC, M, D   }, {op::CMP, M, DLI },
	{op::INY, I, IMP }, {op::CMP, M, IMM }, {op::DEX, I, IMP }, {op::WAI, I, IMP },
	{op::CPY, X, A   }, {op::CMP, M, A   }, {op::DEC, M, A   }, {op::CMP, M, AL  },
	{op::BNE, I, RELB}, {op::CMP, M, DIY }, {op::CMP, M, DI  }, {op::CMP, M, SIY },
	{op::PEI, I, PEI }, {op::CMP, M, DX  }, {op::DEC, M, DX  }, {op::CMP, M, DLIY},
	{op::CLD, I, IMP }, {op::CMP, M, AY  }, {op::PHX, I, IMP }, {op::STP, I, IMP },
	{op::JML, I, AI  }, {op::CMP, M, AX  }, {op::DEC, M, AX  }, {op::CMP, M, ALX },
	{op::CPX, X, IMM }, {op::SBC, M, DXI }, {op::SEP, I, IMM }, {op::SBC, M, S   },
	{op::CPX, X, D   }, {op::SBC, M, D   }, {op::INC, M, D   }, {op::SBC, M, DLI },
	{op::INX, M, IMP }, {op::SBC, M, IMM }, {op::NOP, I, IMP }, {op::XBA, I, IMP },
	{op::CPX, X, A   }, {op::SBC, M, A   }, {op::INC, M, A   }, {op::SBC, M, AL  },
	{op::BEQ, I, RELB}, {op::SBC, M, DIY }, {op::SBC, M, DI  }, {op::SBC, M, SIY },
	{op::PEA, I, PEA }, {op::SBC, M, DX  }, {op::INC, M, DX  }, {op::SBC, M, DLIY},
	{op::SED, I, IMP }, {op::SBC, M, AY  }, {op::PLX, I, IMP }, {op::XCE, I, IMP },
	{op::JSR, I, AXI }, {op::SBC, M, AX  }, {op::INC, M, AX  }, {op::SBC, M, ALX }
};

} // anonymous namespace

static const uint8_t *base_oprom;
static uint32_t base_pc;

static inline unsigned int read_8(unsigned int address)
{
	address = ADDRESS_65816(address);
	return base_oprom[address - base_pc];
}

static inline unsigned int read_16(unsigned int address)
{
	unsigned int val = read_8(address);
	return val | (read_8(address+1)<<8);
}

static inline unsigned int read_24(unsigned int address)
{
	unsigned int val = read_8(address);
	val |= (read_8(address+1)<<8);
	return val | (read_8(address+2)<<16);
}

static inline char* int_8_str(unsigned int val)
{
	static char str[20];

	val &= 0xff;

	if(val & 0x80)
		sprintf(str, "-$%x", (0-val) & 0x7f);
	else
		sprintf(str, "$%x", val & 0x7f);

	return str;
}

static inline char* int_16_str(unsigned int val)
{
	static char str[20];

	val &= 0xffff;

	if(val & 0x8000)
		sprintf(str, "-$%x", (0-val) & 0x7fff);
	else
		sprintf(str, "$%x", val & 0x7fff);

	return str;
}


unsigned g65816_disassemble(char* buff, unsigned int pc, unsigned int pb, const uint8_t *oprom, int m_flag, int x_flag)
{
	unsigned int instruction;
	const g65816_opcode_struct* opcode;
	char* ptr;
	int var;
	int length = 1;
	unsigned int address;
	unsigned dasm_flags;

	pb <<= 16;
	address = pc | pb;

	base_oprom = oprom;
	base_pc = address;

	instruction = read_8(address);
	opcode = &g65816_opcode_struct::get(instruction);

	strcpy(buff, opcode->name());
	ptr = buff + strlen(buff);

	if (opcode->is_call())
		dasm_flags = DASMFLAG_STEP_OVER;
	else if (opcode->is_return())
		dasm_flags = DASMFLAG_STEP_OUT;
	else
		dasm_flags = 0;

	switch(opcode->ea)
	{
		case IMP :
			break;
		case ACC :
			sprintf(ptr, "A");
			break;
		case RELB:
			var = (int8_t) read_8(address+1);
			length++;
			sprintf(ptr, " %06x (%s)", pb | ((pc + length + var)&0xffff), int_8_str(var));
			break;
		case RELW:
		case PER :
			var = read_16(address+1);
			length += 2;
			sprintf(ptr, " %06x (%s)", pb | ((pc + length + var)&0xffff), int_16_str(var));
			break;
		case IMM :
			if((opcode->flag == M && !m_flag) || (opcode->flag == X && !x_flag))
			{
				sprintf(ptr, " #$%04x", read_16(address+1));
				length += 2;
			}
			else
			{
				sprintf(ptr, " #$%02x", read_8(address+1));
				length++;
			}
			break;
		case A   :
		case PEA :
			sprintf(ptr, " $%04x", read_16(address+1));
			length += 2;
			break;
		case AI  :
			sprintf(ptr, " ($%04x)", read_16(address+1));
			length += 2;
			break;
		case AL  :
			sprintf(ptr, " $%06x", read_24(address+1));
			length += 3;
			break;
		case ALX :
			sprintf(ptr, " $%06x,X", read_24(address+1));
			length += 3;
			break;
		case AX  :
			sprintf(ptr, " $%04x,X", read_16(address+1));
			length += 2;
			break;
		case AXI :
			sprintf(ptr, " ($%04x,X)", read_16(address+1));
			length += 2;
			break;
		case AY  :
			sprintf(ptr, " $%04x,Y", read_16(address+1));
			length += 2;
			break;
		case D   :
			sprintf(ptr, " $%02x", read_8(address+1));
			length++;
			break;
		case DI  :
		case PEI :
			sprintf(ptr, " ($%02x)", read_8(address+1));
			length++;
			break;
		case DIY :
			sprintf(ptr, " ($%02x),Y", read_8(address+1));
			length++;
			break;
		case DLI :
			sprintf(ptr, " [$%02x]", read_8(address+1));
			length++;
			break;
		case DLIY:
			sprintf(ptr, " [$%02x],Y", read_8(address+1));
			length++;
			break;
		case DX  :
			sprintf(ptr, " $%02x,X", read_8(address+1));
			length++;
			break;
		case DXI :
			sprintf(ptr, " ($%02x,X)", read_8(address+1));
			length++;
			break;
		case DY  :
			sprintf(ptr, " $%02x,Y", read_8(address+1));
			length++;
			break;
		case S   :
			sprintf(ptr, " %s,S", int_8_str(read_8(address+1)));
			length++;
			break;
		case SIY :
			sprintf(ptr, " (%s,S),Y", int_8_str(read_8(address+1)));
			length++;
			break;
		case SIG :
			sprintf(ptr, " #$%02x", read_8(address+1));
			length++;
			break;
		case MVN :
		case MVP :
			sprintf(ptr, " $%02x, $%02x", read_8(address+2), read_8(address+1));
			length += 2;
			break;
	}

	return length | DASMFLAG_SUPPORTED | dasm_flags;
}

CPU_DISASSEMBLE( g65816_generic )
{
	return g65816_disassemble(buffer, (pc & 0x00ffff), (pc & 0xff0000) >> 16, oprom, 0, 0);
}
