ucli

pm port-add 2/0 100G RS
pm port-add 4/0 100G RS
pm port-enb -/-

exit

pd-just-forward

pd forward add_entry forward_hit ig_intr_md_ingress_port 52 action_port 440
pd forward add_entry forward_hit ig_intr_md_ingress_port 440 action_port 52

exit
