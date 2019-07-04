#include "starting_register.h"
uint16 get_reg_num(qstring target_reg, const insn_t* insn) {
	for (int i = 0; i < UA_MAXOP; i++) {
		qstring operand_text;
		op_t op = insn->ops[i];
		print_operand(&operand_text, insn->ea, i);
		if (regex_match(operand_text.c_str(), target_reg.c_str(), false) != 1) { continue; }
		return op.reg;
	}
	throw "couldnt do struct_processor::get_reg_num()";
}

bool get_reg_highlight(qstring *out) {
	auto widget = get_current_widget();
	uint32 flags;
	get_highlight(out, widget, &flags);
	return (flags & HIF_REGISTER) != 0;
}

starting_register::starting_register() {
	insn_t insn;
	qstring highlighted;

	auto ea = get_screen_ea();
	decode_insn(&insn, ea);
	if (get_reg_highlight(&highlighted)) {
		this->reg_num = get_reg_num(highlighted, &insn);
	}
}

bool starting_register::has_register() {
	return this->reg_num != UINT16_MAX;
}
uint16 starting_register::get_register() {
	return this->reg_num;
}