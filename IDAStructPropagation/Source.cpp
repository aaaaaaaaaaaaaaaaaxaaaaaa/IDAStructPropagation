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
#include "struct_processor.h"

class process_action : public action_handler_t {
	int (*func)(action_activation_ctx_t* ctx);
public:

	process_action(int (*func)(action_activation_ctx_t*)) : func(func) {

	}

	int idaapi activate(action_activation_ctx_t* ctx) {
		return func(ctx);
	}

	action_state_t idaapi update(action_update_ctx_t* ctx) {
		return AST_ENABLE_ALWAYS;
	}
};

// Shift + T action
struc_t* selectedStruct = 0;
void propagate_callback(ea_t ea, uint8 op) {
	insn_t insn;
	decode_insn(&insn, ea);
	op_stroff(insn, op, &selectedStruct->id, 1, 0);
}
int propagate_action(action_activation_ctx_t* ctx) {
	msg("propagate_action\n");
	struc_t* struc = choose_struc("Select struct");
	if (struc == NULL) { return true; }
	selectedStruct = struc;
	struct_processor processor(get_screen_ea(), &propagate_callback);
	selectedStruct = 0;
	return 1;
}
process_action propagate_action_handler = process_action(&propagate_action);
action_desc_t propagate_action_desc = ACTION_DESC_LITERAL("lyxica:propagateoffsets", "Auto struct propagate", &propagate_action_handler, "shift+t", NULL, NULL);

// Shift + Y action
void clear_callback(ea_t ea, uint8 op) {
	clr_op_type(ea, op);
}
int clear_action(action_activation_ctx_t* ctx) {
	struct_processor processor(get_screen_ea(), &clear_callback);
	return 1;
}
process_action propagate_action_handler2 = process_action(&clear_action);
action_desc_t propagate_action2_desc = ACTION_DESC_LITERAL("lyxica:propagateoffsetclear", "Clear offsets from register", &propagate_action_handler2, "shift+y", NULL, NULL);

int idaapi init(void)
{
	if (stricmp(inf.procname, "metapc") != 0) {
		return PLUGIN_SKIP;
	}

	register_action(propagate_action_desc);
	register_action(propagate_action2_desc);
	return PLUGIN_KEEP;
}
void idaapi term(void)
{
	unregister_action("lyxica:propagateoffsets");
	unregister_action("lyxica:propagateoffsetclear");
}
bool idaapi run(size_t arg)
{
	return true;
}	
//--------------------------------------------------------------------------
static const char comment[] = "";
static const char help[] = "";
static const char wanted_name[] = "";
static const char wanted_hotkey[] = "";
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_PROC | PLUGIN_HIDE, // plugin flags
  init,                 // initialize

  term,                 // terminate. this pointer may be NULL.

  run,                  // invoke plugin

  comment,              // long comment about the plugin
						// it could appear in the status line
						// or as a hint

  help,                 // multiline help about the plugin

  wanted_name,          // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};
