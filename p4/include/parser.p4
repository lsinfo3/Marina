/*
 * Parser with support for
 * - Ethernet
 * - IPv4 (with options)
 * - IPv6 (without extensions)
 * - TCP
 * - UDP
 *
 * As everywhere, all protocols can be en-/disabled with preprocessor options
 */

header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        etherType : 16;
    }
}
header ethernet_t ethernet;

header_type ipv4_t {
    fields {
        version        : 4;
        ihl            : 4;
        diffserv       : 8;
        totalLen       : 16;
        identification : 16;
        flags          : 3;
        fragOffset     : 13;
        ttl            : 8;
        protocol       : 8;
        hdrChecksum    : 16;
        srcAddr        : 32;
        dstAddr        : 32;
    }
}

// Do not overlay ipv4 addresses with other header fields.
// Without, enabling IPv4 and v6 would fail in the bloom filter, as
// v4 and v6 header are mutually exclusive, and apparently share memory,
// but the bloom filter has both in the hash field list. The compiler does
// not support this constellation.
@pragma pa_no_overlay ingress ipv4.srcAddr
@pragma pa_no_overlay ingress ipv4.dstAddr
header ipv4_t     ipv4;


header_type ipv6_t {
    fields {
        version      : 4;
        trafficClass : 8;
        flowLabel    : 20;
        payloadLen   : 16;
        nextHdr      : 8;
        hopLimit     : 8;
        srcAddr      : 128;
        dstAddr      : 128;
    }
}
header ipv6_t  ipv6;

header_type tcp_t {
    fields {
        srcPort     : 16;
        dstPort     : 16;
        seqNo       : 32;
        ackNo       : 32;
        dataOffset  : 4;
        res         : 4;
        CWR         : 1;
        ECE         : 1;
        URG         : 1;
        ACK         : 1;
        PSH         : 1;
        RST         : 1;
        SYN         : 1;
        FIN         : 1;
        window      : 16;
        checksum    : 16;
        urgentPtr   : 16;
    }
}
header tcp_t tcp;

header_type udp_t {
    fields {
        srcPort     : 16;
        dstPort     : 16;
        length_     : 16;  /* length is a reserved word */
        checksum    : 16;
    }
}
header udp_t udp;

header_type dns_t {
    fields {
        qr     : 1;
        opcode : 4;
        aa     : 1;
        tc     : 1;
        rd     : 1;
        ra     : 1;
        z      : 3;
        rcode  : 4;
    }
}
header dns_t dns;


/*************************************************************************
 ***********************  M E T A D A T A  *******************************
 *************************************************************************/
header_type l4_lookup_t {
    fields {
        src      : 16;
        dst      : 16;
        tcp_flags: 8;
    }
}
metadata l4_lookup_t l4_lookup;

header_type l3_lookup_t {
    fields {
        len: 16;
    }
}
metadata l3_lookup_t l3_lookup;

/*************************************************************************
 ***********************  P A R S E R  ***********************************
 *************************************************************************/

/*
 * Ethernet parser with support for etherType IPv4 and IPv6
 */
parser start {
    extract(ethernet);
    return select(ethernet.etherType) {
        0x0800 : parse_ipv4;
        0x86DD : parse_ipv6;
        default: ingress;
    }
}

/*
 * Essentially, this is a macro that, for every options length in IPv4,
 * creates a parser to parse this specific options length
 */

#define IPV4_OPTIONS(bits)                                 \
  header_type ipv4_options_##bits##b_t {                   \
      fields { options : bits; }                           \
  }                                                        \
                                                           \
  header ipv4_options_##bits##b_t ipv4_options_##bits##b;  \
                                                           \
  parser parse_ipv4_options_##bits##b {                    \
      extract(ipv4_options_##bits##b);                     \
      set_metadata(l4_lookup.src, current(0, 16));      \
      set_metadata(l4_lookup.dst, current(16, 16));     \
                                                           \
      return select(ipv4.fragOffset, ipv4.protocol) {      \
          0x000006 : parse_tcp;                            \
          0x000011 : parse_udp;                            \
          default  : ingress;                              \
      }                                                    \
  }

IPV4_OPTIONS( 32)  // ihl=0x6  4 Bytes
IPV4_OPTIONS( 64)  // ihl=0x7  8 Bytes
IPV4_OPTIONS( 96)  // ihl=0x8 12 Bytes
IPV4_OPTIONS(128)  // ihl=0x9 16 Bytes
IPV4_OPTIONS(160)  // ihl=0xA 20 Bytes
IPV4_OPTIONS(192)  // ihl=0xB 24 Bytes
IPV4_OPTIONS(224)  // ihl=0xC 28 Bytes
IPV4_OPTIONS(256)  // ihl=0xD 32 Bytes
IPV4_OPTIONS(288)  // ihl=0xE 36 Bytes
IPV4_OPTIONS(320)  // ihl=0xF 40 Bytes

/*
 * The IPv4 entry parser, extracts the normal ipv4 header, then offloads the
 * rest to the options parser above, if there are options
 */
parser parse_ipv4 {
    extract(ipv4);
    set_metadata(l3_lookup.len, ipv4.totalLen);

    return select(ipv4.ihl) {
        0x5 : parse_ipv4_no_options;
        0x6 : parse_ipv4_options_32b;
        0x7 : parse_ipv4_options_64b;
        0x8 : parse_ipv4_options_96b;
        0x9 : parse_ipv4_options_128b;
        0xA : parse_ipv4_options_160b;
        0xB : parse_ipv4_options_192b;
        0xC : parse_ipv4_options_224b;
        0xD : parse_ipv4_options_256b;
        0xE : parse_ipv4_options_288b;
        0xF : parse_ipv4_options_320b;
        default  : ingress; 
    }
}

/*
 * Mostly the same as the options parser above, but missing the extract() step
 * as there are no options.
 * Note the set_metadata calls, to extract the L4 source and destination ports
 * into layer4 independent metadata fields
 */
parser parse_ipv4_no_options {
    set_metadata(l4_lookup.src, current(0, 16));
    set_metadata(l4_lookup.dst, current(16, 16));

    return select(ipv4.fragOffset, ipv4.protocol) {
        0x000006 : parse_tcp;
        0x000011 : parse_udp;
        default  : ingress;
    }
}

parser parse_ipv6 {
    extract(ipv6);

    set_metadata(l4_lookup.src, current(0, 16));
    set_metadata(l4_lookup.dst, current(16, 16));

    set_metadata(l3_lookup.len, ipv6.payloadLen);

    return select(ipv6.nextHdr) {
        0x06 : parse_tcp;
        0x11 : parse_udp;
        default: ingress;
    }
}

parser parse_tcp {
    set_metadata(l4_lookup.tcp_flags, current(104, 8));
    extract(tcp);
    return ingress;
}

parser parse_udp {
    extract(udp);
    return select(udp.srcPort) {
        53: parse_dns;
        default: ingress;
    }
}

parser parse_dns {
    extract(dns);
    return ingress;
}
