#include "struct_processor.h"

/// Check an instruction and see if it causes a monitored register to be spoiled
uint16 did_register_spoil(insn_t insn, std::set<uint16> registers) {
	// Assumptions:
	// 1. A register will only be spoiled happen when that register is the first operand as an o_reg.
	if (insn.ops[0].type != o_reg) { return UINT16_MAX; } // Fails assumption 1
	if (registers.find(insn.ops[0].reg) == registers.end()) { return UINT16_MAX; }
	
	auto flags = insn.get_canon_feature();
	if (flags & CF_CHG1) {
		return insn.ops[0].reg;
	}
	return UINT16_MAX;
}

/// Check if an instruction is moving a structure pointer into another register. 
/// Return the new register if it is, UINT16_MAX otherwise.
uint16 check_for_struc_transfer(insn_t insn, std::set<uint16> set) {
	// Assumptions:
	// 1. ops[0] is destination, ops[1] is the source
	// 2. Horizontal transfer of a structure pointer can only happen if both operands are o_reg
	// 3. An instruction that performs o_reg, o_reg where both o_reg's are the same register is not meaningful (likely xor or nop)
	if (insn.ops[0].type != o_reg || insn.ops[1].type != o_reg) { return UINT16_MAX; }
	if (set.find(insn.ops[1].reg) == set.end()) { return UINT16_MAX; }
	if (insn.ops[0].reg == insn.ops[1].reg) { return UINT16_MAX; }

	return insn.ops[0].reg;
}


bool check_for_add(insn_t insn, std::set<uint16> set) {
	qstring mnem;
	print_insn_mnem(&mnem, insn.ea);
	if (mnem != "add") { return false; }
	if (set.find(insn.ops[0].reg) == set.end()) { return false; }

	return true;
}

struct_processor::struct_processor(ea_t starting_addr, void (*callback)(ea_t, uint8), uint16 starting_register) : callback(callback) {
	this->func = get_func(starting_addr);
	this->processed_lines = 0;

	insn_t starting_insn;
	decode_insn(&starting_insn, starting_addr);
	std::set<uint16> monitored_registers;
	std::set<ea_t> visited;
	monitored_registers.insert(starting_register);
	this->process(starting_addr, monitored_registers);
}
qvector<ea_t> struct_processor::branch_target(insn_t insn) {
	qvector<ea_t> branch_targets;
	for (auto target = get_first_fcref_from(insn.ea); target != BADADDR; target = get_next_fcref_from(insn.ea, target)) {
		branch_targets.push_back(target);
	}
	return branch_targets;
}

void struct_processor::process(ea_t addr, std::set<uint16> monitored_registers) {
	insn_t insn;
	ea_t decoded_addr;
	decoded_addr = decode_insn(&insn, addr);
	while (func_contains(this->func, insn.ea) && monitored_registers.size() > 0) {
		if (visited.find(insn.ea) != visited.end()) { return; }
		visited.insert(insn.ea);
		this->processed_lines++;
		bool bypass_spoil = false;

		// Detect if a monitored register has copied its value into another register, and add that new register to the monitor list
		uint16 new_register = check_for_struc_transfer(insn, monitored_registers);
		if (new_register != UINT16_MAX) {
			monitored_registers.insert(new_register);
			bypass_spoil = true;
		}

		// Detect any jump targets and recursively run them
		auto targets = branch_target(insn);
		if (!targets.empty()) {
			for (int i = 0; i < targets.size(); i++) {
				this->process(targets[i], monitored_registers); // Insn is a Jcc type, process TRUE branch
				decoded_addr = decode_insn(&insn, insn.ea + insn.size);
				this->process(insn.ea, monitored_registers); // process FALSE branch
			}
		}

		// Apply structure offsets to any monitored registers
		for (int i = 0; i < UA_MAXOP; i++) {
			op_t op = insn.ops[i];
			if (op.type == o_void) { break; }
			if (op.type == o_displ || op.type == o_phrase) {
				uint16 reg = op.specflag2 ? sib_base(insn, op) : op.reg;
				if (monitored_registers.find(reg) != monitored_registers.end()) {
					this->callback(insn.ea, i);
				}
			}
		}

		// Handle structure offsets that are set using the add op
		if (check_for_add(insn, monitored_registers)) {
			this->callback(insn.ea, 1);
		}

		// Check if a monitored register has had an outside value moved into it
		if (!bypass_spoil) {
			if (insn.get_canon_feature() & CF_STOP) {
				auto targets = branch_target(insn);
				for (int i = 0; i < targets.size(); i++) {
					this->process(targets[i], monitored_registers);
				}
				return;
			}
			uint16 spoiled_register = did_register_spoil(insn, monitored_registers);
			if ((spoiled_register != UINT16_MAX) && this->processed_lines > 1) {
				monitored_registers.erase(spoiled_register);
			}
		}
		decoded_addr = decode_insn(&insn, insn.ea + insn.size);
	}
}
