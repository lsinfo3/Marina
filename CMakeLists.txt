cmake_minimum_required(VERSION 3.10)
project(controller C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og -ggdb -fno-eliminate-unused-debug-symbols")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto -march=native -fno-semantic-interposition")
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include_directories(controller/tofino_include)

set(controller_source_files
        controller/export/export_features.c
        controller/dataplane/dataplane.c
        controller/dataplane/prepare_dataplane.c
        controller/packet_handler/packet_handler.c
        controller/datastructures/heap.c
        controller/flow_db/flow_db.c
        controller/datastructures/packet_ringbuffer.c
        controller/datastructures/dataplane_ringbuffer.c
        controller/dataplane/dp_api.c
        controller/datastructures/bloom_filter.c
        controller/datastructures/flow_hashtable.c
        controller/datastructures/session_hashtable.c
        controller/datastructures/hashes/MurmurHash3.c
        controller/datastructures/hashes/crc.c
        controller/dataplane/features.c
        controller/dns_db/dns_db.c
        controller/datastructures/dns_ringbuffer.c
        controller/datastructures/dns_hashtable.c
        controller/logging/statistics.c
        controller/runtime_config/runtime_config.h controller/runtime_config/runtime_config.c)


add_library(controller SHARED
        controller/controller.c
        ${controller_source_files}
        )
target_link_libraries(controller pcap uuid)

add_library(benchmark SHARED
        ${controller_source_files}
        controller/benchmark.c
        )
target_link_libraries(benchmark pcap uuid)


add_executable(flowdb_eval
        controller/evaluation/flowdb_eval.c
        controller/tests/mock_switch.c
        ${controller_source_files}
        )
target_link_libraries(flowdb_eval pcap uuid resolv)
target_compile_definitions(flowdb_eval PRIVATE FLOWDB_STATISTICS)

add_executable(flowdb_bloom_eval
        controller/evaluation/flowdb_bloom_eval.c
        controller/tests/mock_switch.c
        ${controller_source_files}
        )
target_link_libraries(flowdb_bloom_eval pcap uuid resolv)
target_compile_definitions(flowdb_bloom_eval PRIVATE FLOWDB_STATISTICS)

add_executable(dnsdb_eval
        controller/evaluation/dnsdb_eval.c
        controller/tests/mock_switch.c
        ${controller_source_files}
        )
target_link_libraries(dnsdb_eval pcap uuid resolv)
target_compile_definitions(dnsdb_eval PRIVATE DNSDB_STATISTICS)

add_executable(export_eval
        controller/evaluation/flowexport_eval.c
        controller/tests/mock_switch.c
        ${controller_source_files}
        )
target_link_libraries(export_eval pcap uuid resolv)
target_compile_definitions(export_eval PRIVATE ENABLE_BENCH)

add_library(packet_handler_eval SHARED
        ${controller_source_files}
        controller/evaluation/packet_handler_eval.c
        )
target_link_libraries(packet_handler_eval pcap uuid)
target_compile_definitions(packet_handler_eval PRIVATE ENABLE_PACKET_HANDLER_STATISTICS)

message("CMAKE_C_FLAGS is ${CMAKE_C_FLAGS}")
message("CMAKE_C_FLAGS_DEBUG is ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE is ${CMAKE_C_FLAGS_RELEASE}")
