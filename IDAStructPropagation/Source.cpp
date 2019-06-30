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

int idaapi init(void)
{
	if (stricmp(inf.procname, "metapc") != 0) {
		msg("skipped idastructpropagation");
		return PLUGIN_SKIP;
	}
	msg("running idastructpropagation");
	return PLUGIN_OK;
}

void idaapi term(void)
{
}


bool idaapi run(size_t arg)
{
	struct_processor processor(get_screen_ea());
	return true;
}	

//--------------------------------------------------------------------------
static const char comment[] = "Auto struct propagate";
static const char help[] = "";
static const char wanted_name[] = "Auto struct propagate";
static const char wanted_hotkey[] = "shift+t";
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_UNL,           // plugin flags
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
