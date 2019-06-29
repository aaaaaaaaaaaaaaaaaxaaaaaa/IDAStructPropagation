#include "struct_processor.h"

struct_processor::struct_processor(ea_t starting_addr) : starting_addr(starting_addr) {
	this->target_reg = get_reg_highlight();
	this->struc = choose_struc("Select struct");
	/* debugging: head of initial node will be BLUE */
	auto m = get_bb_head(starting_addr);
	insn_t instr;
	decode_insn(&instr, m);
	set_item_color(m, 0xFF0000);
	/* */
	this->visited.insert(get_bb_head(starting_addr));
	this->func = get_func(starting_addr);
	this->process(starting_addr);
	//this->registers_to_spoil_watch.push_back(this->get_reg_num(this->target_reg));
}

qstring struct_processor::get_reg_highlight() {
	qstring highlight;
	auto widget = get_current_widget();
	uint32 flags;
	get_highlight(&highlight, widget, &flags);
	if ((flags & HIF_REGISTER) == 0) {
		msg("highlight isn't a register name - got: %s - flags: %x\n", highlight.c_str(), flags);
		throw "highlight isn't a register name";
	}

	return highlight;
}

ea_t struct_processor::get_bb_head(ea_t start) {
	/* find head of current basic block */
	insn_t current;
	func_t* func = get_func(start);
	decode_insn(&current, start);
	while (1) {
		ea_t xref = get_first_fcref_to(current.ea);
		if (xref != BADADDR || current.ea == func->start_ea) {
			break;
		}

		decode_prev_insn(&current, current.ea);
	}
	// current contains head of our BB
	msg("found head of BB at %x\n", current.ea);
	return current.ea;
}

bool struct_processor::did_register_spoil(insn_t insn) {
	// Assumptions:
	// 1. A register will only be spoiled happen when that register is the first operand as an o_reg.
	// 2. Instructions that spoil registers almost always have a 2nd operand. 
	if (insn.ops[1].type == o_void) { return false; } // Fails assumption 2
	if (insn.ops[0].type != o_reg) { return false; } // Fails assumption 1
	return insn.ops[0].reg == this->get_reg_num(this->target_reg);
}

uint16 struct_processor::get_reg_num(qstring target_reg) {
	insn_t original;
	decode_insn(&original, this->starting_addr);
	for (int i = 0; i < UA_MAXOP; i++) {
		qstring operand_text;
		op_t op = original.ops[i];
		print_operand(&operand_text, original.ea, i);
		if (regex_match(operand_text.c_str(), target_reg.c_str(), false) != 1) { continue; }
		return op.reg;
	}
	throw "couldnt do struct_processor::get_reg_num()";
}

bool should_branch(insn_t insn) {
	ea_t target = get_first_dref_from(insn.ea);
	uint32 flags = insn.get_canon_feature();
	if (target != BADADDR && (flags & CF_STOP) == 0) {  // 

	}
	return false;
}

void struct_processor::process(ea_t addr) {
	insn_t insn;
	ea_t decoded_addr;
	decoded_addr = decode_insn(&insn, addr);
	while (func_contains(this->func, insn.ea)) {
		/* debug  */
		set_item_color(insn.ea, 0x0000FF);
		/**/
		for (int i = 0; i < UA_MAXOP; i++) {
			op_t op = insn.ops[i];
			if (op.type == o_void) { break; }
			if (op.type == o_displ || op.type == o_phrase) {
				qstring operand_text;
				print_operand(&operand_text, insn.ea, i);
				if (regex_match(operand_text.c_str(), this->target_reg.c_str(), false) != 1) { continue; }
				set_item_color(insn.ea, 0x00FF00);
			}
		}
		if (did_register_spoil(insn)) {
			set_item_color(insn.ea, 0x777777);
			return;
		}
		decoded_addr = decode_insn(&insn, insn.ea + insn.size);
	}
}
