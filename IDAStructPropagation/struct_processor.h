#pragma once
#pragma warning(push, 0)        
#include <ida.hpp>
#include <idp.hpp>
#include <expr.hpp>
#include <bytes.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <frame.hpp>
#include <struct.hpp>
#include <name.hpp>
#include <cstdio>
#include <set>
#pragma warning(pop)
class struct_processor
{
public:
	struct_processor(ea_t);
private:
	ea_t starting_addr;
	qstring target_reg;
	std::set<ea_t> visited;
	struc_t* struc;
	func_t* func;
	qvector<uint32> registers_to_spoil_watch;

	qstring get_reg_highlight();
	ea_t get_bb_head(ea_t start);
	void process(ea_t addr);
	bool did_register_spoil(insn_t);
	uint16 get_reg_num(qstring);
};

