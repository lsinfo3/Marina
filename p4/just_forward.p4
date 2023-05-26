#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
//#include <tofino/stateful_alu_blackbox.p4>
#else
#error This program is intended to compile for Tofino P4 architecture only
#endif

/*
 * The simplest possible p4 program that actually does something useful
 * Just forward based on ingress port or drop, when there are no rules
 */

parser start {
    return ingress;
}

action forward_hit(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}
action forward_miss() {
    drop();
}

table forward {
    reads { ig_intr_md.ingress_port: exact; }
    actions {
        forward_hit;
        forward_miss;
    }
    default_action: forward_miss();
}

control ingress {
    apply(forward);
}
