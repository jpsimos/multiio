# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {multiio_platform}\
-hw {/home/jacobpsimos/MultiIO/Vivado/multiio_design_wrapper.xsa}\
-out {/home/jacobpsimos/MultiIO/Vitis/Workspace}

platform write
domain create -name {cortexa9_0_domain} -display-name {cortexa9_0_domain} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {empty_application}
platform generate -domains 
platform write
domain active {zynq_fsbl}
domain active {cortexa9_0_domain}
platform generate -quick
platform active {multiio_platform}
domain create -name {cortexa9_1_domain} -display-name {cortexa9_1_domain} -os {standalone} -proc {ps7_cortexa9_1} -runtime {cpp} -arch {32-bit} -support-app {empty_application}
platform generate -domains 
platform write
domain active {zynq_fsbl}
domain active {cortexa9_0_domain}
domain active {cortexa9_1_domain}
platform generate -quick
domain active {zynq_fsbl}
domain active {cortexa9_0_domain}
domain active {cortexa9_1_domain}
platform-generate
bsp reload
bsp config tcp_snd_buf "32768"
bsp config memp_n_tcp_seg "1024"
bsp write
bsp reload
catch {bsp regenerate}
platform generate
platform active {multiio_platform}
domain active {cortexa9_0_domain}
bsp reload
domain active {zynq_fsbl}
bsp reload
domain active {cortexa9_1_domain}
bsp reload
bsp reload
domain active {cortexa9_0_domain}
domain config -qemu-data {/home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/resources/multiio_platform/cortexa9_0_domain/qemu}
platform write
domain config -qemu-args {/home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/resources/multiio_platform/cortexa9_0_domain/qemu_args.txt}
platform write
domain active {cortexa9_1_domain}
domain config -qemu-data {/home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/resources/multiio_platform/cortexa9_1_domain/qemu}
platform write
domain config -qemu-args {/home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/resources/multiio_platform/cortexa9_1_domain/qemu_args.txt}
platform write
platform generate -domains 
platform clean
platform generate
platform active {multiio_platform}
domain active {cortexa9_0_domain}
bsp reload
platform active {multiio_platform}
platform generate
bsp reload
bsp reload
