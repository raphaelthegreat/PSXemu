#include "cdrom.h"

#define BIND(x) std::bind(&CDRom::x, this)

void CDRom::register_commands()
{
	command_lookup[0x1] = BIND(get_stat_cmd);
	//command_lookup[0x2] = BIND(set_loc_cmd);
	//command_lookup[0x3] = BIND(play_cmd);
	//command_lookup[0x4] = BIND(forward_cmd);
	//command_lookup[0x5] = BIND(backward_cmd);
	//command_lookup[0x6] = BIND(readn_cmd);
	//command_lookup[0x7] = BIND(motor_on_cmd);
	//command_lookup[0x8] = BIND(stop_cmd);
	//command_lookup[0x9] = BIND(pause_cmd);
	//command_lookup[0xA] = BIND(init_cmd);
	//command_lookup[0xB] = BIND(mute_cmd);
	//command_lookup[0xC] = BIND(demute_cmd);
	//command_lookup[0xD] = BIND(set_filter_cmd);
	//command_lookup[0xE] = BIND(set_mode_cmd);
	//command_lookup[0xF] = BIND(get_param_cmd);
	//command_lookup[0x10] = BIND(get_locl_cmd);
	//command_lookup[0x11] = BIND(get_locp_cmd);
	//command_lookup[0x12] = BIND(set_session_cmd);
	//command_lookup[0x13] = BIND(get_tn_cmd);
	//command_lookup[0x14] = BIND(get_td_cmd);
	//command_lookup[0x15] = BIND(seekl_cmd);
	//command_lookup[0x16] = BIND(seekp_cmd);
	command_lookup[0x19] = BIND(test_cmd);
	command_lookup[0x1A] = BIND(get_id_cmd);
	//command_lookup[0x1B] = BIND(reads_cmd);
	//command_lookup[0x1C] = BIND(reset_cmd);
	//command_lookup[0x1D] = BIND(getq_cmd);
	command_lookup[0x1E] = BIND(read_toc_cmd);
	//command_lookup[0x1F] = BIND(video_cd_cmd);
}