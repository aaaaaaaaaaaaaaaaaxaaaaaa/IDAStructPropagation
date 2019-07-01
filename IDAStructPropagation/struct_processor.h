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
	struct_processor(ea_t, struc_t*);

private:
	ea_t starting_addr;
	qstring target_reg;
	std::set<ea_t> visited;
	struc_t* struc;
	func_t* func;
	uint processed_lines;

	qstring get_reg_highlight();
	void process(ea_t addr, std::set<uint16>);
	uint16 did_register_spoil(insn_t, std::set<uint16>);
	uint16 get_reg_num(qstring, insn_t);
	ea_t branch_target(insn_t);
};

