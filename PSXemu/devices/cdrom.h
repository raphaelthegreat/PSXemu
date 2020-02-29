#pragma once
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <queue>

struct CDStatus {
	uint32_t irq_enable;
	uint32_t irq_request;
	uint32_t index;
	uint32_t command;
	bool has_command;
};

typedef std::function<void()> CDCommand;

/* A class that emulates the CD Rom drive */
/* and is responsible of sending game data to the CPU. */
class Bus;
class CDRom {
public:
	CDRom(Bus* _bus);
	~CDRom() = default;

	/* Bus read/write. */
	void write(uint32_t offset, uint32_t data);
	uint32_t read(uint32_t offset);

	void register_commands();

	uint8_t get_data();
	uint8_t get_arg();
	uint8_t get_response();

	void tick();

	/* CDROM commands. */
	void get_stat_cmd();
	void set_loc_cmd();
	void play_cmd();
	void forward_cmd();
	void backward_cmd();
	void readn_cmd();
	void motor_on_cmd();
	void stop_cmd();
	void pause_cmd();
	void init_cmd();
	void mute_cmd();
	void demute_cmd();
	void set_filter_cmd();
	void set_mode_cmd();
	void get_param_cmd();
	void get_locl_cmd();
	void get_locp_cmd();
	void set_session_cmd();
	void get_tn_cmd();
	void get_td_cmd();
	void seekl_cmd();
	void seekp_cmd();
	void test_cmd();
	void get_id_cmd();
	void reads_cmd();
	void reset_cmd();
	void getq_cmd();
	void read_toc_cmd();
	void video_cd_cmd();

public:
	CDStatus status;
	CDCommand second_response;

	/* CD queues. */
	std::queue<uint8_t> parameter_fifo;
	std::queue<uint8_t> response_fifo;
	std::queue<uint8_t> data_fifo;

	std::unordered_map<uint32_t, CDCommand> command_lookup;

	Bus* bus;
};