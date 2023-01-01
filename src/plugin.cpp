#include <iostream>

// clang-format off
#include <gcc-plugin.h>
#include <context.h>
#include <plugin-version.h>
#include <tree.h>
#include <gimple.h>
#include <tree-pass.h>
#include <gimple-iterator.h>
#include <stringpool.h>
#include <attribs.h>
#include <gimple-pretty-print.h>
#include <tree-pretty-print.h>
#include <plugin.h>
#include <cp/cp-tree.h>
#include <diagnostic-core.h>
#include <print-tree.h>
#include "tree-core.h"

// clang-format on

namespace
{

// -----------------------------------------------------------------------------
// GCC PLUGIN SETUP (BASIC INFO / LICENSE / REQUIRED VERSION)
// -----------------------------------------------------------------------------

/**
 * When 1 enables verbose printing
 */
constexpr auto DEBUG = 1;

/**
 * Version of this plugin
 */
constexpr auto PLUGIN_VERSION = "0.1";

/**
 * Help/usage string for the plugin
 */
constexpr auto PLUGIN_HELP = "This plugin instruments functions with the invariant attribute";

/**
 * Name of the attribute used to instrument a function
 */
constexpr auto ATTRIBUTE_NAME = "invariant";

/**
 * Name of this plugin
 */
constexpr auto PLUGIN_NAME = "invariant_plugin";

/**
 * GCC version we need to use this plugin
 */
constexpr auto PLUGIN_GCC_BASEV = "13.0.0";

/**
 * Additional information about the plugin. Used by --help and --version
 */
const struct plugin_info invariant_plugin_info = {
  .version = PLUGIN_VERSION,
  .help    = PLUGIN_HELP,
};

/**
 * Represents the gcc version we need. Used to void using an incompatible plugin
 */
const struct plugin_gcc_version invariant_plugin_version = {
  .basever = PLUGIN_GCC_BASEV,
};

// -----------------------------------------------------------------------------
// GCC EXTERNAL DECLARATION
// -----------------------------------------------------------------------------

/**
 * Takes a tree node and returns the identifier string
 * @see https://gcc.gnu.org/onlinedocs/gccint/Identifiers.html
 */
constexpr auto FN_NAME = [](tree tree_fun) { return IDENTIFIER_POINTER(DECL_NAME(tree_fun)); };

// -----------------------------------------------------------------------------
// GCC ATTRIBUTES MANAGEMENT (REGISTERING / CALLBACKS)
// -----------------------------------------------------------------------------

/**
 * Attribute handler callback
 * @note NODE points to the node to which the attribute is to be applied. NAME
 * is the name of the attribute. ARGS is the TREE_LIST of arguments (may be
 * NULL). FLAGS gives information about the context of the attribute.
 * Afterwards, the attributes will be added unless *NO_ADD_ATTRS is set to true
 * (which should be done on error). Depending on FLAGS, any attributes to be
 * applied to another type or DECL later may be returned; otherwise the return
 * value should be NULL_TREE. This pointer may be NULL if no special handling is
 * required
 * @see Declared in tree-core.h
 */
tree
handle_invariant_attribute(tree* node, tree name, tree args, int flags, bool* no_add_attrs)
{
  if constexpr (DEBUG == 1)
  {
    fprintf(stderr, "> Found attribute\n");

    fprintf(stderr, "\tnode = ");
    print_generic_stmt(stderr, *node, TDF_NONE);

    fprintf(stderr, "\tname = ");
    print_generic_stmt(stderr, name, TDF_NONE);
  }
  return NULL_TREE;
}

/**
 * Structure describing an attribute and a function to handle it
 * @see Declared in tree-core.h
 * @note Refer to tree-core for docs about
 */

/* Attribute definition */
const struct attribute_spec invariant_attribute = {
  // [[demo::invariant]]
  .name                   = "invariant",
  .min_length             = 0,
  .max_length             = 0,
  .decl_required          = true,
  .type_required          = false,
  .function_type_required = false,
  .affects_type_identity  = false,
  .handler                = handle_invariant_attribute,
  .exclude                = nullptr,
};

// The array of attribute specs passed to register_scoped_attributes must be NULL terminated
const attribute_spec scoped_attributes[] = {
  invariant_attribute,
  { NULL, 0, 0, false, false, false, false, NULL, NULL }
};

/**
 * Plugin callback called during attribute registration
 */
void
register_attributes(void* event_data, void* data)
{
  warning(0, "Callback to register attributes");
  register_scoped_attributes(scoped_attributes, "demo");
}

// -----------------------------------------------------------------------------
// PLUGIN INSTRUMENTATION LOGICS
// -----------------------------------------------------------------------------

/**
 * For each function lookup attributes and attach profiling function
 */
unsigned int
instrument_invariants_plugin_exec(void)
{
  // get the FUNCTION_DECL of the function whose body we are reading
  tree fndecl = current_function_decl;

  // print the function name
  fprintf(stderr, "> Inspecting function '%s'\n", FN_NAME(fndecl));

  // If the method is not a member of a struct/class, then skip it.
  if (TREE_CODE(DECL_CONTEXT(fndecl)) != RECORD_TYPE)
    return 0;

  // Try to locate the [[invariant]] function
  tree invariant_fn = NULL_TREE;
  // Iterate over every member function of the class
  for (tree f = TYPE_FIELDS(DECL_FIELD_CONTEXT(fndecl)); f != NULL_TREE; f = DECL_CHAIN(f))
  {
    if (TREE_CODE(f) == FUNCTION_DECL)
    {
      // Check if the function has an [[invariant]] attribute
      tree attrs = DECL_ATTRIBUTES(f);
      for (tree attr = attrs; attr != nullptr; attr = TREE_CHAIN(attr))
      {
        // Check if attribute name is "invariant"
        if (get_attribute_name(attr) == get_identifier("invariant"))
        {
          invariant_fn = f;
        }
      }
    }
  }

  if (invariant_fn == NULL_TREE)
    return 0;

  // If the method name is the same as the [[invariant]] attribute function, then skip it.
  if (DECL_NAME(fndecl) == DECL_NAME(invariant_fn))
    return 0;

  // attribute was in the list
  fprintf(stderr, "\t attribute %s found! \n", ATTRIBUTE_NAME);

  // get function entry block
  basic_block entry = ENTRY_BLOCK_PTR_FOR_FN(cfun)->next_bb;

  auto insert_invariant_calls_intelligently = [&] {
    // get the first statement
    gimple* first_stmt = gsi_stmt(gsi_start_bb(entry));

    // Skip if this is a constructor, because fields will not be initialized yet
    if (DECL_CONSTRUCTOR_P(fndecl))
    {
      fprintf(stderr, "\t skipping constructor start invariant call\n");
      return;
    }

    // warn the user we are adding an invariant function
    fprintf(stderr, "\t adding function call before ");
    print_gimple_stmt(stderr, first_stmt, 0, TDF_NONE);

    // Insert the invariant call before the current statement
    gimple_stmt_iterator gsi = gsi_for_stmt(first_stmt);
    gsi_insert_before(&gsi, gimple_build_call(invariant_fn, 0), GSI_SAME_STMT);

    // Skip if destructor, because fields will have been destroyed already
    if (DECL_DESTRUCTOR_P(fndecl))
    {
      fprintf(stderr, "\t skipping destructor end invariant call\n");
      return;
    }

    // Insert the invariant call at the end of the function, or before the return statement (if
    // any)
    gimple_stmt_iterator gsi2      = gsi_last_bb(ENTRY_BLOCK_PTR_FOR_FN(cfun)->next_bb);
    gimple*              last_stmt = gsi_stmt(gsi2);

    // Double check to ensure that the last_stmt != first_stmt
    if (first_stmt == last_stmt)
    {
      fprintf(stderr, "\t first and last statement are the same, skipping last statement\n");
      return;
    }

    // If the last statement is a return statement, then insert before it
    if (gimple_code(last_stmt) == GIMPLE_RETURN)
    {
      fprintf(stderr, "\t adding function call before ");
      print_gimple_stmt(stderr, last_stmt, 0, TDF_NONE);
      gsi_insert_before(&gsi2, gimple_build_call(invariant_fn, 0), GSI_SAME_STMT);
    }
    else
    {
      fprintf(stderr, "\t adding function call after ");
      print_gimple_stmt(stderr, last_stmt, 0, TDF_NONE);
      gsi_insert_after(&gsi2, gimple_build_call(invariant_fn, 0), GSI_SAME_STMT);
    }
  };

  // Insert the invariant function calls at the beginning/end of fn bodies based on
  // certain conditions (whether it's a ctor/dtor, etc.)
  insert_invariant_calls_intelligently();

  // done!
  return 0;
}

/**
 * Metadata for a pass, non-varying across all instances of a pass
 * @see Declared in tree-pass.h
 * @note Refer to tree-pass for docs about
 */
const struct pass_data invariant_pass_data = {
  .type                 = GIMPLE_PASS,     // type of pass
  .name                 = PLUGIN_NAME,     // name of plugin
  .optinfo_flags        = OPTGROUP_NONE,   // no opt dump
  .tv_id                = TV_NONE,         // no timevar (see timevar.h)
  .properties_required  = PROP_gimple_any, // entire gimple grammar as input
  .properties_provided  = 0,               // no prop in output
  .properties_destroyed = 0,               // no prop removed
  .todo_flags_start     = 0,               // need nothing before
  .todo_flags_finish =
    TODO_update_ssa | TODO_cleanup_cfg     // need to update SSA repr after and repair cfg
};

/**
 * Definition of our invariant GIMPLE pass
 * @note Extends gimple_opt_pass class
 * @see Declared in tree-pass.h
 */
class invariant_gimple_pass : public gimple_opt_pass
{
public:
  /**
   * Constructor
   */
  invariant_gimple_pass(const pass_data& data, gcc::context* ctxt)
    : gimple_opt_pass(data, ctxt)
  {
  }

  /**
   * This is the code to run when pass is executed
   * @note Defined in opt_pass father class
   * @see Defined in tree-pass.h
   */
  unsigned int execute(function* exec_fun) { return instrument_invariants_plugin_exec(); }
};

}; // namespace

// -----------------------------------------------------------------------------
// PLUGIN INITIALIZATION
// -----------------------------------------------------------------------------

int plugin_is_GPL_compatible;

/**
 * Initializes the plugin. Returns 0 if initialization finishes successfully.
 */
int
plugin_init(struct plugin_name_args* plugin_info, struct plugin_gcc_version* version)
{
  // this plugin is compatible only with specified base ver
  if (!plugin_default_version_check(version, &gcc_version))
    return 1;

  register_callback(plugin_info->base_name, PLUGIN_INFO, NULL, (void*) &invariant_plugin_info);

  // warn the user about the presence of this plugin
  printf("> plugin '%s @ %s' was loaded onto GCC\n", PLUGIN_NAME, PLUGIN_VERSION);

  // insert inst pass into the struct used to register the pass
  register_pass_info invariant_pass = {
    .pass                     = new invariant_gimple_pass(invariant_pass_data, g),
    .reference_pass_name      = "ssa", // get called after GCC has produced SSA representation
    .ref_pass_instance_number = 1,     // after first opt pass to be sure opt will not throw away
    .pos_op                   = PASS_POS_INSERT_AFTER,
  };

  // add our pass hooking into pass manager
  register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &invariant_pass);

  // get called at attribute registration
  register_callback(plugin_info->base_name, PLUGIN_ATTRIBUTES, register_attributes, NULL);

  // everthing has worked
  return 0;
}