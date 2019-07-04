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
#include <intel.hpp>
#include <cstdio>
#include <set>
#pragma warning(pop)
class starting_register
{
public:
	starting_register();
	bool has_register();
	uint16 get_register();

private:
	uint16 reg_num = UINT16_MAX;
};

bool get_reg_highlight(qstring*);
uint16 get_reg_num(qstring, const insn_t*);