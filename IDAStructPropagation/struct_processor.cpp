#include "struct_processor.h"

struct_processor::struct_processor(ea_t starting_addr, struc_t* struc) : starting_addr(starting_addr), struc(struc) {
	this->target_reg = get_reg_highlight();
	this->func = get_func(starting_addr);
	this->processed_lines = 0;

	insn_t starting_insn;
	decode_insn(&starting_insn, starting_addr);
	std::set<uint16> monitored_registers;
	monitored_registers.insert(this->get_reg_num(this->target_reg, starting_insn));
	this->process(starting_addr, monitored_registers);
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

/// Check an instruction and see if it causes a monitored register to be spoiled
uint16 struct_processor::did_register_spoil(insn_t insn, std::set<uint16> monitored_registers) {
	// Assumptions:
	// 1. A register will only be spoiled happen when that register is the first operand as an o_reg.
	// 2. Instructions that spoil registers almost always have a 2nd operand. 
	if (insn.ops[1].type == o_void) { return UINT16_MAX; } // Fails assumption 2
	if (insn.ops[0].type != o_reg) { return UINT16_MAX; } // Fails assumption 1
	if (monitored_registers.find(insn.ops[0].reg) != monitored_registers.end()) {
		return insn.ops[0].reg;
	}
	else {
		return UINT16_MAX;
	}
}

uint16 struct_processor::get_reg_num(qstring target_reg, insn_t insn) {
	for (int i = 0; i < UA_MAXOP; i++) {
		qstring operand_text;
		op_t op = insn.ops[i];
		print_operand(&operand_text, insn.ea, i);
		if (regex_match(operand_text.c_str(), target_reg.c_str(), false) != 1) { continue; }
		return op.reg;
	}
	throw "couldnt do struct_processor::get_reg_num()";
}

ea_t struct_processor::branch_target(insn_t insn) {
	ea_t branch_target = get_first_fcref_from(insn.ea);
	return this->func->contains(branch_target) ? branch_target : BADADDR;
}

/// Check if an instruction is moving a structure pointer into another register. 
/// Return the new register if it is, UINT16_MAX otherwise.
uint16 struct_processor::check_for_struc_transfer(insn_t insn, std::set<uint16> set) {
	// Assumptions:
	// 1. ops[0] is destination, ops[1] is the source
	// 2. Horizontal transfer of a structure pointer can only happen if both operands are o_reg
	// 3. An instruction that performs o_reg, o_reg where both o_reg's are the same register is not meaningful (likely xor or nop)
	if (insn.ops[0].type != o_reg || insn.ops[1].type != o_reg) { return UINT16_MAX; }
	if (set.find(insn.ops[1].reg) == set.end()) { return UINT16_MAX; }
	if (insn.ops[0].reg == insn.ops[1].reg) { return UINT16_MAX; }

	return insn.ops[0].reg;
}

bool struct_processor::check_for_add(insn_t insn, std::set<uint16> set) {
	qstring mnem;
	print_insn_mnem(&mnem, insn.ea);
	if (mnem != "add") { return false; }
	if (set.find(insn.ops[0].reg) == set.end()) { return false; }

	return true;
}

void struct_processor::process(ea_t addr, std::set<uint16> monitored_registers) {
	insn_t insn;
	ea_t decoded_addr;
	decoded_addr = decode_insn(&insn, addr);

	while (func_contains(this->func, insn.ea) && monitored_registers.size() > 0) {
		if (this->visited.find(insn.ea) != this->visited.end()) { return; }
		this->visited.insert(insn.ea);
		this->processed_lines++;
		bool bypass_spoil = false;

		uint16 new_register = check_for_struc_transfer(insn, monitored_registers);
		if (new_register != UINT16_MAX) {
			monitored_registers.insert(new_register);
			bypass_spoil = true;
		}

		if (branch_target(insn) != BADADDR) {
			this->process(branch_target(insn), monitored_registers); // Insn is a Jcc type, process TRUE branch
			decoded_addr = decode_insn(&insn, insn.ea + insn.size);
			this->process(insn.ea, monitored_registers); // process FALSE branch
			return;
		}

		for (int i = 0; i < UA_MAXOP; i++) {
			op_t op = insn.ops[i];
			if (op.type == o_void) { break; }
			if (op.type == o_displ || op.type == o_phrase) {
				uint16 reg = op.specflag2 ? sib_base(insn, op) : op.reg;
				if (monitored_registers.find(reg) != monitored_registers.end()) {
					op_stroff(insn, i, &this->struc->id, 1, 0);
				}
			}
		}

		if (check_for_add(insn, monitored_registers)) {
			op_stroff(insn, 1, &this->struc->id, 1, 0);
		}

		if (!bypass_spoil) {
			if (insn.get_canon_feature() & CF_STOP) {
				if (branch_target(insn) != BADADDR) {
					this->process(branch_target(insn), monitored_registers);
				}
				return;
			}
			uint16 spoiled_register = this->did_register_spoil(insn, monitored_registers);
			if ((spoiled_register != UINT16_MAX) && this->processed_lines > 1) {
				monitored_registers.erase(spoiled_register);
			}
		}
		decoded_addr = decode_insn(&insn, insn.ea + insn.size);
	}
}
