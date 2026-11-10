

proc generate {drv_handle} {
	xdefine_include_file $drv_handle "xparameters.h" "gmii_to_mii" "NUM_INSTANCES" "DEVICE_ID"  "C_S_AXILITE_BASEADDR" "C_S_AXILITE_HIGHADDR"
}
