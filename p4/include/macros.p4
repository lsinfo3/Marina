/*
 * Helper macros
 * In the P4 control block, one can either call another control block
 * or apply a table. There is no way to directly apply an action.
 * A workaround is to apply a dummy table with size 1 and the desired action
 * as default action.
 * A similar thing is true for the stateful alu blackboxes, which require a
 * verbose call, but are only ever called with m.flow_id as index, therefore
 * it is easier to encapsulate the call in a macro.
 *
 * - BLACKBOX2ACTION
 *   prepare a action for a stateful_alu_blackbox with flow_id as index
 *
 * - ACTION2TABLE
 *   prepare a simple table with the action as default_action and size one
 *
 * - BLACKBOX2TABLE
 *   combination of the first two macros, create a table for the blackbox

 */

/*
 * Assumes there is a blackbox called NAME_alu
 * creates a table called NAME_action
 */
#define BLACKBOX2ACTION(NAME)                           \
action NAME##_action() {                                \
    NAME##_alu.execute_stateful_alu(m.flow_id);         \
}

/*
 * assumes there is an action called NAME_action
 * creates a table called NAME_table which executes this action
 */
#define ACTION2TABLE(NAME)                              \
table NAME##_table {                                    \
    actions { NAME##_action; }                          \
    default_action: NAME##_action();                    \
    size: 1;                                            \
}

/*
 * Assumes there is a blackbox called NAME_alu
 * creates a table called NAME_table which executes this blackbox
 */
#define BLACKBOX2TABLE(NAME)                            \
BLACKBOX2ACTION(NAME)                                   \
ACTION2TABLE(NAME)
