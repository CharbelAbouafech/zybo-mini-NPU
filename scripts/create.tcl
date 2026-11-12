# create.tcl - rebuild the mini_npu Vivado project into proj/
# Usage (from the repo root): vivado -mode batch -source scripts/create.tcl

set proj_name mini_npu
set current_dir [file normalize [file dirname [info script]]]
set repo_dir [file dirname $current_dir]
set proj_dir [file join $repo_dir proj]

if {[file exists $proj_dir]} {
    error "proj/ already exists at $proj_dir. Remove it before re-running this script."
}

# The block design needs the Zybo Z7-10 board preset (DDR, MIO, clocks), so the board files are required.
# Checked before create_project so a failure doesn't leave a half-made proj/ behind.
# Boards installed from the Vivado Store are only found automatically in the GUI, so batch mode
# needs the per-user board store folder added to board.repoPaths.
set vivado_ver [version -short]
if {[info exists ::env(APPDATA)]} {
    set board_store [file join $::env(APPDATA) Xilinx Vivado $vivado_ver xhub board_store \
        xilinx_board_store XilinxBoardStore Vivado $vivado_ver boards]
    if {[file isdirectory $board_store]} {
        set_param board.repoPaths [concat [get_param board.repoPaths] [list $board_store]]
    }
}
set board_part [lindex [lsort [get_board_parts -quiet "digilentinc.com:zybo-z7-10:part0:*"]] end]
if {$board_part eq ""} {
    error "Zybo Z7-10 board files not found. Install Digilent's vivado-boards (board_files/zybo-z7-10) into <Vivado>/data/boards/board_files and restart Vivado."
}

create_project $proj_name $proj_dir -part xc7z010clg400-1
set_property board_part $board_part [current_project]
set_property target_language Verilog [current_project]

# RTL sources (.v and .sv) - must be added before the block design, because the mini-NPU is a module reference in it
set src_files [concat [glob -nocomplain [file join $repo_dir src *.v]] [glob -nocomplain [file join $repo_dir src *.sv]]]
if {[llength $src_files] > 0} {
    add_files -norecurse $src_files
}

# Constraints
set xdc_files [glob -nocomplain [file join $repo_dir src *.xdc]]
if {[llength $xdc_files] > 0} {
    add_files -fileset constrs_1 -norecurse $xdc_files
}

# Testbenches and test vectors (.mem files in sim_1 are copied into the xsim run folder by Vivado)
set sim_files [concat [glob -nocomplain [file join $repo_dir sim *.v]] [glob -nocomplain [file join $repo_dir sim *.sv]]]
if {[llength $sim_files] > 0} {
    add_files -fileset sim_1 -norecurse $sim_files
}
set mem_files [glob -nocomplain [file join $repo_dir sim *.mem]]
if {[llength $mem_files] > 0} {
    add_files -fileset sim_1 -norecurse $mem_files
}

# Block design: scripts/bd.tcl is exported from the GUI with write_bd_tcl
set bd_script [file join $current_dir bd.tcl]
if {[file exists $bd_script]} {
    source $bd_script
    set bd_file [lindex [get_files -filter {FILE_TYPE == "Block Designs"}] 0]
    set bd_name [file rootname [file tail $bd_file]]
    make_wrapper -files $bd_file -top -import
    set_property top "${bd_name}_wrapper" [current_fileset]
} else {
    puts "INFO: scripts/bd.tcl not found - project created without a block design."
}

update_compile_order -fileset sources_1
update_compile_order -fileset sim_1
