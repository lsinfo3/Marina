#ifndef CONTROLLER_CHECK_CONFIG_H
#define CONTROLLER_CHECK_CONFIG_H

/*
 * Helper file for early configuration checks
 */

#ifndef ENABLE_IP4
#ifndef ENABLE_IP6
#error "Must enable either ENABLE_IP4 or ENABLE_IP6"
#endif
#endif

#ifndef ENABLE_TCP
#ifndef ENABLE_UDP
#error "Must enable either ENABLE_TCP or ENABLE_UDP"
#endif
#endif

#ifdef IP6_ONLY_MATCH_CRC
#error "IP6_ONLY_MATCH_CRC currently not implemented on the controller"
#ifndef ENABLE_IP6
#error "IP6_ONLY_MATCH_CRC only available with ENABLE_IP6"
#endif
#endif

#endif //CONTROLLER_CHECK_CONFIG_H
