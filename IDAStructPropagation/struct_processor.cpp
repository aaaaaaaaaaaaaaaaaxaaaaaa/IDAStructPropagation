#include "struct_processor.h"

struct_processor::struct_processor(ea_t starting_addr) : starting_addr(starting_addr) {
	this->target_reg = get_reg_highlight();
	this->struc = choose_struc("Select struct");
	this->func = get_func(starting_addr);
	this->process(starting_addr);
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

ea_t struct_processor::branch_target(insn_t insn) {
	ea_t branch_target = get_first_fcref_from(insn.ea);
	return this->func->contains(branch_target) ? branch_target : BADADDR;
}

void cmt(insn_t insn, qstring text) {
	qstring cmt;
	get_cmt(&cmt, insn.ea, false);
	char new_cmt[256];
	qsnprintf(new_cmt, 256, "%s - %s", cmt.c_str(), text.c_str());
	set_cmt(insn.ea, new_cmt, false);
}

void struct_processor::process(ea_t addr) {
	insn_t insn;
	ea_t decoded_addr;
	decoded_addr = decode_insn(&insn, addr);
	cmt(insn, "[ENTRY]");
	while (func_contains(this->func, insn.ea)) {
		if (this->visited.find(insn.ea) != this->visited.end()) { return; }
		this->visited.insert(insn.ea);

		if (branch_target(insn) != BADADDR) {
			this->process(branch_target(insn));
			cmt(insn, " - branched from here");
			decoded_addr = decode_insn(&insn, insn.ea + insn.size);
			continue;
		}
		cmt(insn, "processed line");

		for (int i = 0; i < UA_MAXOP; i++) {
			op_t op = insn.ops[i];
			if (op.type == o_void) { break; }
			if (op.type == o_displ || op.type == o_phrase) {
				qstring operand_text;
				print_operand(&operand_text, insn.ea, i);
				if (regex_match(operand_text.c_str(), this->target_reg.c_str(), false) != 1) { continue; }
				cmt(insn, "found matching");
				op_stroff(insn, i, &this->struc->id, 1, 0);
			}
		}
		if (insn.get_canon_feature() & CF_STOP) {
			if (branch_target(insn) != BADADDR) {
				this->process(branch_target(insn));
				cmt(insn, " branched from here");
			}
			cmt(insn, "CF_STOP");
			return;
		}
		if (did_register_spoil(insn)) {
			cmt(insn, "detected spoil");
			return;
		}
		decoded_addr = decode_insn(&insn, insn.ea + insn.size);
	}
}
