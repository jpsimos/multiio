# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: /home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_system/_ide/scripts/systemdebugger_multiio_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source /home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_system/_ide/scripts/systemdebugger_multiio_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-HS2 210241257964" && level==0 && jtag_device_ctx=="jsn-JTAG-HS2-210241257964-13722093-0"}
fpga -file /home/jacobpsimos/MultiIO/Vitis/Workspace/core0/_ide/bitstream/multiio_design_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw /home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/export/multiio_platform/hw/multiio_design_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
targets -set -nocase -filter {name =~ "*A9*#0"}
rst -processor
dow /home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/export/multiio_platform/sw/multiio_platform/boot/fsbl.elf
set bp_11_24_fsbl_bp [bpadd -addr &FsblHandoffJtagExit]
con -block -timeout 60
bpremove $bp_11_24_fsbl_bp
targets -set -nocase -filter {name =~ "*A9*#0"}
dow /home/jacobpsimos/MultiIO/Vitis/Workspace/core0/Debug/core0.elf
targets -set -nocase -filter {name =~ "*A9*#1"}
dow /home/jacobpsimos/MultiIO/Vitis/Workspace/core1/Debug/core1.elf
configparams force-mem-access 0
bpadd -addr &main
