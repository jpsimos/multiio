domain active {zynq_fsbl}
bsp config extra_compiler_flags "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -nostartfiles -g -Wall -Wextra -fno-tree-loop-distribute-patterns -fstack-protector-strong"
bsp config dependency_flags "-MMD -MP"
bsp config compiler "arm-none-eabi-gcc"
bsp write
bsp reload
catch {bsp regenerate}
domain active {cortexa9_0_domain}
bsp config extra_compiler_flags "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -nostartfiles -g -Wall -Wextra -fno-tree-loop-distribute-patterns -fstack-protector-strong"
bsp write
bsp reload
catch {bsp regenerate}
domain active {cortexa9_1_domain}
bsp config extra_compiler_flags "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -nostartfiles -g -Wall -Wextra -fno-tree-loop-distribute-patterns -fstack-protector-strong"
bsp setlib -name lwip220 -ver 1.1
bsp config stdin "ps7_coresight_comp_0"
bsp config stdout "ps7_coresight_comp_0"
bsp config use_emaclite_on_zynq "0"
bsp config lwip_tcp_keepalive "true"
bsp config arp_table_size "15"
bsp config lwip_dhcp "true"
bsp config ip_options "1"
bsp config mem_size "524288"
bsp config phy_link_speed "CONFIG_LINKSPEED100"
bsp setlib -name xilffs -ver 5.3
bsp config use_lfn "1"
bsp config use_mkfs "false"
bsp config use_strfunc "1"
bsp config use_chmod "true"
bsp config set_fs_rpath "1"
bsp write
bsp reload
catch {bsp regenerate}
