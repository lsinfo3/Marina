/*
 * This file contains all forwarding tables and actions
 *
 * to_controller: applied when the packet should be copied to the controller
 *                sets the copy_to_cpu flag, which causes the scheduler to create a
 *                copy which is targeted to the cpu
 * save_ingress_port_to_ethernet_src: Applied on egress. If the packet is on its way to the CPU,
 *                                    saves the original ingress port in the Ethernet
 *                                    source port, because the controller requires the ingress
 *                                    port for correct flow id mapping and register allocation
 * forward_table: contains ingress_port -> egress_port mappings for simple
 *                forwarding.
 */


action to_controller_action() {
    modify_field(ig_intr_md_for_tm.copy_to_cpu, 1);
}
ACTION2TABLE(to_controller)

action forward_action(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}
action forward_miss() {
    drop();
}
@pragma phase0 1
table forward_table {
    reads {
        ig_intr_md.ingress_port: exact;
    }
    actions {
        forward_action;
    }
	default_action: forward_action;
    size: 64;
}

control forward {
	if (ig_intr_md.resubmit_flag == 0) {
		apply(forward_table);
	}
}

/*
 * On Egress:
 */
action save_ingress_port_to_ethernet_src_action() {
    modify_field(ethernet.srcAddr, ig_intr_md.ingress_port);
    modify_field(ethernet.dstAddr, ig_intr_md.ingress_mac_tstamp);
}
ACTION2TABLE(save_ingress_port_to_ethernet_src)
