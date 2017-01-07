#ifndef PACKET_H_
#define PACKET_H_

#include "network.h"

#define STATE_HANDSHAKE 0
#define STATE_PLAY 3
#define STATE_STATUS 1
#define STATE_LOGIN 2

#define PKT_HANDSHAKE_SERVER_HANDSHAKE 0

struct pkt_handshake_server_handshake {
		int32_t protocol_version;
		char* server_address;
		uint16_t server_port;
		int32_t next_state;
};

#define PKT_PLAY_CLIENT_SPAWNOBJECT 0

struct pkt_play_client_spawnobject {
		int32_t entity_id;
		struct uuid object_uuid;
		int8_t type;
		double x;
		double y;
		double z;
		uint8_t pitch;
		uint8_t yaw;
		int32_t data;
		int16_t velocity_x;
		int16_t velocity_y;
		int16_t velocity_z;
};

#define PKT_PLAY_CLIENT_SPAWNEXPERIENCEORB 1

struct pkt_play_client_spawnexperienceorb {
		int32_t entity_id;
		double x;
		double y;
		double z;
		int16_t count;
};

#define PKT_PLAY_CLIENT_SPAWNGLOBALENTITY 2

struct pkt_play_client_spawnglobalentity {
		int32_t entity_id;
		int8_t type;
		double x;
		double y;
		double z;
};

#define PKT_PLAY_CLIENT_SPAWNMOB 3

struct pkt_play_client_spawnmob {
		int32_t entity_id;
		struct uuid entity_uuid;
		int32_t type;
		double x;
		double y;
		double z;
		uint8_t yaw;
		uint8_t pitch;
		uint8_t head_pitch;
		int16_t velocity_x;
		int16_t velocity_y;
		int16_t velocity_z;
		struct entity_metadata metadata;
};

#define PKT_PLAY_CLIENT_SPAWNPAINTING 4

struct pkt_play_client_spawnpainting {
		int32_t entity_id;
		struct uuid entity_uuid;
		char* title;
		struct encpos location;
		int8_t direction;
};

#define PKT_PLAY_CLIENT_SPAWNPLAYER 5

struct pkt_play_client_spawnplayer {
		int32_t entity_id;
		struct uuid player_uuid;
		double x;
		double y;
		double z;
		uint8_t yaw;
		uint8_t pitch;
		struct entity_metadata metadata;
};

#define PKT_PLAY_CLIENT_ANIMATION 6

struct pkt_play_client_animation {
		int32_t entity_id;
		uint8_t animation;
};

#define PKT_PLAY_CLIENT_STATISTICS 7

struct statistic {
		char* name;
		int32_t value;
};

struct pkt_play_client_statistics {
		int32_t count;
		struct statistic* statistics;
};

#define PKT_PLAY_CLIENT_BLOCKBREAKANIMATION 8

struct pkt_play_client_blockbreakanimation {
		int32_t entity_id;
		struct encpos location;
		int8_t destroy_stage;
};

#define PKT_PLAY_CLIENT_UPDATEBLOCKENTITY 9

struct pkt_play_client_updateblockentity {
		struct encpos location;
		uint8_t action;
		struct nbt_tag* nbt_data;
};

#define PKT_PLAY_CLIENT_BLOCKACTION 10

struct pkt_play_client_blockaction {
		struct encpos location;
		uint8_t action_id;
		uint8_t action_param;
		int32_t block_type;
};

#define PKT_PLAY_CLIENT_BLOCKCHANGE 11

struct pkt_play_client_blockchange {
		struct encpos location;
		int32_t block_id;
};

#define PKT_PLAY_CLIENT_BOSSBAR 12

struct bossbar_add {
		char* title;
		float health;
		int32_t color;
		int32_t division;
		uint8_t flags;
};

struct bossbar_updatestyle {
		int32_t color;
		int32_t division;
};

union bossbar_actions {
		struct bossbar_add add;
		float updatehealth_health;
		char* updatetitle_title;
		struct bossbar_updatestyle updatestyle;
		uint8_t updateflags_flags;
};

struct pkt_play_client_bossbar {
		struct uuid uuid;
		int32_t actionid;
		union bossbar_actions action;
};

#define PKT_PLAY_CLIENT_SERVERDIFFICULTY 13

struct pkt_play_client_serverdifficulty {
		uint8_t difficulty;
};

#define PKT_PLAY_CLIENT_TABCOMPLETE 14

struct pkt_play_client_tabcomplete {
		int32_t count;
		char** matches;
};

#define PKT_PLAY_CLIENT_CHATMESSAGE 15

struct pkt_play_client_chatmessage {
		char* json_data;
		int8_t position;
};

#define PKT_PLAY_CLIENT_MULTIBLOCKCHANGE 16

struct __attribute__((__packed__)) mbc_record {
		uint8_t x :4;
		uint8_t z :4;
		uint8_t y;
		int32_t block_id;
};

struct pkt_play_client_multiblockchange {
		int32_t chunk_x;
		int32_t chunk_z;
		int32_t record_count;
		struct mbc_record* records;
};

#define PKT_PLAY_CLIENT_CONFIRMTRANSACTION 17

struct pkt_play_client_confirmtransaction {
		int8_t window_id;
		int16_t action_number;
		uint8_t accepted;
};

#define PKT_PLAY_CLIENT_CLOSEWINDOW 18

struct pkt_play_client_closewindow {
		uint8_t window_id;
};

#define PKT_PLAY_CLIENT_OPENWINDOW 19

struct pkt_play_client_openwindow {
		uint8_t window_id;
		char* window_type;
		char* window_title;
		uint8_t number_of_slots;
		int32_t entity_id; // optional
};

#define PKT_PLAY_CLIENT_WINDOWITEMS 20

struct pkt_play_client_windowitems {
		uint8_t window_id;
		int16_t count;
		struct slot* slot_data;
};

#define PKT_PLAY_CLIENT_WINDOWPROPERTY 21

struct pkt_play_client_windowproperty {
		uint8_t window_id;
		int16_t property;
		int16_t value;
};

#define PKT_PLAY_CLIENT_SETSLOT 22

struct pkt_play_client_setslot {
		int8_t window_id;
		int16_t slot;
		struct slot slot_data;
};

#define PKT_PLAY_CLIENT_SETCOOLDOWN 23

struct pkt_play_client_setcooldown {
		int32_t item_id;
		int32_t cooldown_ticks;
};

#define PKT_PLAY_CLIENT_PLUGINMESSAGE 24

struct pkt_play_client_pluginmessage {
		char* channel;
		size_t data_size;
		int8_t* data;
};

#define PKT_PLAY_CLIENT_NAMEDSOUNDEFFECT 25

struct pkt_play_client_namedsoundeffect {
		char* sound_name;
		int32_t sound_category;
		int32_t effect_position_x;
		int32_t effect_position_y;
		int32_t effect_position_z;
		float volume;
		float pitch;
};

#define PKT_PLAY_CLIENT_DISCONNECT 26

struct pkt_play_client_disconnect {
		char* reason;
};

#define PKT_PLAY_CLIENT_ENTITYSTATUS 27

struct pkt_play_client_entitystatus {
		int32_t entity_id;
		int8_t entity_status;
};

#define PKT_PLAY_CLIENT_EXPLOSION 28

struct pkt_play_client_explosion {
		float x;
		float y;
		float z;
		float radius;
		int32_t record_count;
		int8_t* records; // size is record_count * 3
		float player_motion_x;
		float player_motion_y;
		float player_motion_z;
};

#define PKT_PLAY_CLIENT_UNLOADCHUNK 29

struct pkt_play_client_unloadchunk {
		int32_t chunk_x;
		int32_t chunk_z;
		struct chunk* ch;
};

#define PKT_PLAY_CLIENT_CHANGEGAMESTATE 30

struct pkt_play_client_changegamestate {
		uint8_t reason;
		float value;
};

#define PKT_PLAY_CLIENT_KEEPALIVE 31

struct pkt_play_client_keepalive {
		int32_t keep_alive_id;
};

#define PKT_PLAY_CLIENT_CHUNKDATA 32

struct pkt_play_client_chunkdata {
		uint8_t ground_up_continuous;
		int16_t cx;
		int16_t cz;
		struct chunk* data;
		int32_t number_of_block_entities;
		struct nbt_tag** block_entities;
};

#define PKT_PLAY_CLIENT_EFFECT 33

struct pkt_play_client_effect {
		int32_t effect_id;
		struct encpos location;
		int32_t data;
		uint8_t disable_relative_volume;
};

#define PKT_PLAY_CLIENT_PARTICLE 34

struct pkt_play_client_particle {
		int32_t particle_id;
		uint8_t long_distance;
		float x;
		float y;
		float z;
		float offset_x;
		float offset_y;
		float offset_z;
		float particle_data;
		int32_t particle_count;
		int32_t* data;
};

#define PKT_PLAY_CLIENT_JOINGAME 35

struct pkt_play_client_joingame {
		int32_t entity_id;
		uint8_t gamemode;
		int32_t dimension;
		uint8_t difficulty;
		uint8_t max_players;
		char* level_type;
		uint8_t reduced_debug_info;
};

#define PKT_PLAY_CLIENT_MAP 36

struct map_icon {
		int8_t direction :4;
		int8_t type :4;
		int8_t x;
		int8_t z;
};

struct pkt_play_client_map {
		int32_t item_damage;
		int8_t scale;
		int8_t tracking_position;
		int32_t icon_count;
		struct icon* icons;
		int8_t columns;
		int8_t rows; // optional
		int8_t x; // optional
		int8_t z; // optional
		int32_t length; // optional
		uint8_t* data; // optional
};

#define PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE 37

struct pkt_play_client_entityrelativemove {
		int32_t entity_id;
		int16_t delta_x;
		int16_t delta_y;
		int16_t delta_z;
		uint8_t on_ground;
};

#define PKT_PLAY_CLIENT_ENTITYLOOKANDRELATIVEMOVE 38

struct pkt_play_client_entitylookandrelativemove {
		int32_t entity_id;
		int16_t delta_x;
		int16_t delta_y;
		int16_t delta_z;
		uint8_t yaw;
		uint8_t pitch;
		uint8_t on_ground;
};

#define PKT_PLAY_CLIENT_ENTITYLOOK 39

struct pkt_play_client_entitylook {
		int32_t entity_id;
		uint8_t yaw;
		uint8_t pitch;
		uint8_t on_ground;
};

#define PKT_PLAY_CLIENT_ENTITY 40

struct pkt_play_client_entity {
		int32_t entity_id;
};

#define PKT_PLAY_CLIENT_VEHICLEMOVE 41

struct pkt_play_client_vehiclemove {
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
};

#define PKT_PLAY_CLIENT_OPENSIGNEDITOR 42

struct pkt_play_client_opensigneditor {
		struct encpos location;
};

#define PKT_PLAY_CLIENT_PLAYERABILITIES 43

struct pkt_play_client_playerabilities {
		int8_t flags;
		float flying_speed;
		float field_of_view_modifier;
};

#define PKT_PLAY_CLIENT_COMBATEVENT 44

struct combatevent_endcombat {
		int32_t duration;
		int32_t entity_id;
};

struct combatevent_entitydead {
		int32_t player_id;
		int32_t entity_id;
		char* message;
};

union combatevent_event {
		struct combatevent_endcombat endcombat;
		struct combatevent_entitydead entitydead;
};

struct pkt_play_client_combatevent {
		int32_t event_id;
		union combatevent_event event;
};

#define PKT_PLAY_CLIENT_PLAYERLISTITEM 45

struct listitem_addplayer_property {
		char* name;
		char* value;
		int8_t isSigned; // optional
		char* signature; // optional
};

struct listitem_addplayer {
		char* name;
		int32_t number_of_properties;
		struct listitem_addplayer_property* properties;
		int32_t gamemode;
		int32_t ping;
		int8_t has_display_name;
		char* display_name; //optional
};

struct listitem_updatedisplayname {
		int8_t has_display_name;
		char* display_name; //optional
};

union listitem_action {
		struct listitem_addplayer addplayer;
		int32_t updategamemode_gamemode;
		int32_t updatelatency_ping;
		struct listitem_updatedisplayname updatedisplayname;
};

struct listitem_player {
		struct uuid uuid;
		union listitem_action action;
};

struct pkt_play_client_playerlistitem {
		int32_t action_id;
		int32_t number_of_players;
		struct listitem_player* players;
};

#define PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK 46

struct pkt_play_client_playerpositionandlook {
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
		int8_t flags;
		int32_t teleport_id;
};

#define PKT_PLAY_CLIENT_USEBED 47

struct pkt_play_client_usebed {
		int32_t entity_id;
		struct encpos location;
};

#define PKT_PLAY_CLIENT_DESTROYENTITIES 48

struct pkt_play_client_destroyentities {
		int32_t count;
		int32_t* entity_ids;
};

#define PKT_PLAY_CLIENT_REMOVEENTITYEFFECT 49

struct pkt_play_client_removeentityeffect {
		int32_t entity_id;
		int8_t effect_id;
};

#define PKT_PLAY_CLIENT_RESOURCEPACKSEND 50

struct pkt_play_client_resourcepacksend {
		char* url;
		char* hash;
};

#define PKT_PLAY_CLIENT_RESPAWN 51

struct pkt_play_client_respawn {
		int32_t dimension;
		uint8_t difficulty;
		uint8_t gamemode;
		char* level_type;
};

#define PKT_PLAY_CLIENT_ENTITYHEADLOOK 52

struct pkt_play_client_entityheadlook {
		int32_t entity_id;
		uint8_t head_yaw;
};

#define PKT_PLAY_CLIENT_WORLDBORDER 53

struct worldborder_lerpsize {
		double old_diameter;
		double new_diameter;
		int64_t speed;
};

struct worldborder_setcenter {
		double x;
		double z;
};

struct worldborder_initialize {
		double x;
		double z;
		double old_diameter;
		double new_diameter;
		int64_t speed;
		int32_t portal_teleport_boundary;
		int32_t warning_time;
		int32_t warning_blocks;
};

union worldborder_action {
		double setsize_diameter;
		struct worldborder_lerpsize lerpsize;
		struct worldborder_setcenter setcenter;
		struct worldborder_initialize initialize;
		int32_t setwarningtime_warning_time;
		int32_t setwarningblocks_warning_blocks;
};

struct pkt_play_client_worldborder {
		int32_t action_id;
		union worldborder_action action;
};

#define PKT_PLAY_CLIENT_CAMERA 54

struct pkt_play_client_camera {
		int32_t camera_id;
};

#define PKT_PLAY_CLIENT_HELDITEMCHANGE 55

struct pkt_play_client_helditemchange {
		int8_t slot;
};

#define PKT_PLAY_CLIENT_DISPLAYSCOREBOARD 56

struct pkt_play_client_displayscoreboard {
		int8_t position;
		char* score_name;
};

#define PKT_PLAY_CLIENT_ENTITYMETADATA 57

struct pkt_play_client_entitymetadata {
		int32_t entity_id;
		struct entity_metadata metadata;
};

#define PKT_PLAY_CLIENT_ATTACHENTITY 58

struct pkt_play_client_attachentity {
		int32_t attached_entity_id;
		int32_t holding_entity_id;
};

#define PKT_PLAY_CLIENT_ENTITYVELOCITY 59

struct pkt_play_client_entityvelocity {
		int32_t entity_id;
		int16_t velocity_x;
		int16_t velocity_y;
		int16_t velocity_z;
};

#define PKT_PLAY_CLIENT_ENTITYEQUIPMENT 60

struct pkt_play_client_entityequipment {
		int32_t entity_id;
		int32_t slot;
		struct slot item;
};

#define PKT_PLAY_CLIENT_SETEXPERIENCE 61

struct pkt_play_client_setexperience {
		float experience_bar;
		int32_t level;
		int32_t total_experience;
};

#define PKT_PLAY_CLIENT_UPDATEHEALTH 62

struct pkt_play_client_updatehealth {
		float health;
		int32_t food;
		float food_saturation;
};

#define PKT_PLAY_CLIENT_SCOREBOARDOBJECTIVE 63

struct pkt_play_client_scoreboardobjective {
		char* objective_name;
		int8_t mode;
		char* objective_value; // optional
		char* type; // optional
};

#define PKT_PLAY_CLIENT_SETPASSENGERS 64

struct pkt_play_client_setpassengers {
		int32_t entity_id;
		int32_t passenger_count;
		int32_t* passengers;
};

#define PKT_PLAY_CLIENT_TEAMS 65

struct pkt_play_client_teams {
		//TODO: Manual Implementation
};

#define PKT_PLAY_CLIENT_UPDATESCORE 66

struct pkt_play_client_updatescore {
		char* score_name;
		int8_t action;
		char* objective_name;
		int32_t value; // optional
};

#define PKT_PLAY_CLIENT_SPAWNPOSITION 67

struct pkt_play_client_spawnposition {
		struct encpos location;
};

#define PKT_PLAY_CLIENT_TIMEUPDATE 68

struct pkt_play_client_timeupdate {
		int64_t world_age;
		int64_t time_of_day;
};

#define PKT_PLAY_CLIENT_TITLE 69

struct pkt_play_client_title {
		//TODO: Manual Implementation
};

#define PKT_PLAY_CLIENT_SOUNDEFFECT 70

struct pkt_play_client_soundeffect {
		int32_t sound_id;
		int32_t sound_category;
		int32_t effect_position_x;
		int32_t effect_position_y;
		int32_t effect_position_z;
		float volume;
		float pitch;
};

#define PKT_PLAY_CLIENT_PLAYERLISTHEADERANDFOOTER 71

struct pkt_play_client_playerlistheaderandfooter {
		char* header;
		char* footer;
};

#define PKT_PLAY_CLIENT_COLLECTITEM 72

struct pkt_play_client_collectitem {
		int32_t collected_entity_id;
		int32_t collector_entity_id;
		int32_t pickup_item_count;
};

#define PKT_PLAY_CLIENT_ENTITYTELEPORT 73

struct pkt_play_client_entityteleport {
		int32_t entity_id;
		double x;
		double y;
		double z;
		uint8_t yaw;
		uint8_t pitch;
		uint8_t on_ground;
};

#define PKT_PLAY_CLIENT_ENTITYPROPERTIES 74

struct entity_property_modifier {
		struct uuid uuid;
		double amount;
		int8_t operation;
};

struct entity_property {
		char* key;
		double value;
		int32_t number_of_modifiers;
		struct entity_property_modifier* modifiers;
};

struct pkt_play_client_entityproperties {
		int32_t entity_id;
		int32_t number_of_properties;
		struct entity_property* properties;
};

#define PKT_PLAY_CLIENT_ENTITYEFFECT 75

struct pkt_play_client_entityeffect {
		int32_t entity_id;
		int8_t effect_id;
		int8_t amplifier;
		int32_t duration;
		int8_t flags;
};

#define PKT_PLAY_SERVER_TELEPORTCONFIRM 0

struct pkt_play_server_teleportconfirm {
		int32_t teleport_id;
};

#define PKT_PLAY_SERVER_TABCOMPLETE 1

struct pkt_play_server_tabcomplete {
		char* text;
		uint8_t assume_command;
		uint8_t has_position;
		struct encpos looked_at_block; // optional
};

#define PKT_PLAY_SERVER_CHATMESSAGE 2

struct pkt_play_server_chatmessage {
		char* message;
};

#define PKT_PLAY_SERVER_CLIENTSTATUS 3

struct pkt_play_server_clientstatus {
		int32_t action_id;
};

#define PKT_PLAY_SERVER_CLIENTSETTINGS 4

struct pkt_play_server_clientsettings {
		char* locale;
		int8_t view_distance;
		int32_t chat_mode;
		uint8_t chat_colors;
		uint8_t displayed_skin_parts;
		int32_t main_hand;
};

#define PKT_PLAY_SERVER_CONFIRMTRANSACTION 5

struct pkt_play_server_confirmtransaction {
		int8_t window_id;
		int16_t action_number;
		uint8_t accepted;
};

#define PKT_PLAY_SERVER_ENCHANTITEM 6

struct pkt_play_server_enchantitem {
		int8_t window_id;
		int8_t enchantment;
};

#define PKT_PLAY_SERVER_CLICKWINDOW 7

struct pkt_play_server_clickwindow {
		uint8_t window_id;
		int16_t slot;
		int8_t button;
		int16_t action_number;
		int32_t mode;
		struct slot clicked_item;
};

#define PKT_PLAY_SERVER_CLOSEWINDOW 8

struct pkt_play_server_closewindow {
		uint8_t window_id;
};

#define PKT_PLAY_SERVER_PLUGINMESSAGE 9

struct pkt_play_server_pluginmessage {
		char* channel;
		int8_t* data;
};

#define PKT_PLAY_SERVER_USEENTITY 10

struct pkt_play_server_useentity {
		int32_t target;
		int32_t type;
		float target_x; // optional
		float target_y; // optional
		float target_z; // optional
		int32_t hand; // optional
};

#define PKT_PLAY_SERVER_KEEPALIVE 11

struct pkt_play_server_keepalive {
		int32_t keep_alive_id;
};

#define PKT_PLAY_SERVER_PLAYERPOSITION 12

struct pkt_play_server_playerposition {
		double x;
		double feet_y;
		double z;
		uint8_t on_ground;
};

#define PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK 13

struct pkt_play_server_playerpositionandlook {
		double x;
		double feet_y;
		double z;
		float yaw;
		float pitch;
		uint8_t on_ground;
};

#define PKT_PLAY_SERVER_PLAYERLOOK 14

struct pkt_play_server_playerlook {
		float yaw;
		float pitch;
		uint8_t on_ground;
};

#define PKT_PLAY_SERVER_PLAYER 15

struct pkt_play_server_player {
		uint8_t on_ground;
};

#define PKT_PLAY_SERVER_VEHICLEMOVE 16

struct pkt_play_server_vehiclemove {
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
};

#define PKT_PLAY_SERVER_STEERBOAT 17

struct pkt_play_server_steerboat {
		uint8_t right_paddle_turning;
		uint8_t left_paddle_turning;
};

#define PKT_PLAY_SERVER_PLAYERABILITIES 18

struct pkt_play_server_playerabilities {
		int8_t flags;
		float flying_speed;
		float walking_speed;
};

#define PKT_PLAY_SERVER_PLAYERDIGGING 19

struct pkt_play_server_playerdigging {
		int32_t status;
		struct encpos location;
		int8_t face;
};

#define PKT_PLAY_SERVER_ENTITYACTION 20

struct pkt_play_server_entityaction {
		int32_t entity_id;
		int32_t action_id;
		int32_t jump_boost;
};

#define PKT_PLAY_SERVER_STEERVEHICLE 21

struct pkt_play_server_steervehicle {
		float sideways;
		float forward;
		uint8_t flags;
};

#define PKT_PLAY_SERVER_RESOURCEPACKSTATUS 22

struct pkt_play_server_resourcepackstatus {
		int32_t result;
};

#define PKT_PLAY_SERVER_HELDITEMCHANGE 23

struct pkt_play_server_helditemchange {
		int16_t slot;
};

#define PKT_PLAY_SERVER_CREATIVEINVENTORYACTION 24

struct pkt_play_server_creativeinventoryaction {
		int16_t slot;
		struct slot clicked_item;
};

#define PKT_PLAY_SERVER_UPDATESIGN 25

struct pkt_play_server_updatesign {
		struct encpos location;
		char* line_1;
		char* line_2;
		char* line_3;
		char* line_4;
};

#define PKT_PLAY_SERVER_ANIMATION 26

struct pkt_play_server_animation {
		int32_t hand;
};

#define PKT_PLAY_SERVER_SPECTATE 27

struct pkt_play_server_spectate {
		struct uuid target_player;
};

#define PKT_PLAY_SERVER_PLAYERBLOCKPLACEMENT 28

struct pkt_play_server_playerblockplacement {
		struct encpos location;
		int32_t face;
		int32_t hand;
		float cursor_position_x;
		float cursor_position_y;
		float cursor_position_z;
};

#define PKT_PLAY_SERVER_USEITEM 29

struct pkt_play_server_useitem {
		int32_t hand;
};

#define PKT_STATUS_CLIENT_RESPONSE 0

struct pkt_status_client_response {
		char* json_response;
};

#define PKT_STATUS_CLIENT_PONG 1

struct pkt_status_client_pong {
		int64_t payload;
};

#define PKT_STATUS_SERVER_REQUEST 0

struct pkt_status_server_request {

};

#define PKT_STATUS_SERVER_PING 1

struct pkt_status_server_ping {
		int64_t payload;
};

#define PKT_LOGIN_CLIENT_DISCONNECT 0

struct pkt_login_client_disconnect {
		char* reason;
};

#define PKT_LOGIN_CLIENT_ENCRYPTIONREQUEST 1

struct pkt_login_client_encryptionrequest {
		char* server_id;
		int32_t public_key_length;
		uint8_t* public_key;
		int32_t verify_token_length;
		uint8_t* verify_token;
};

#define PKT_LOGIN_CLIENT_LOGINSUCCESS 2

struct pkt_login_client_loginsuccess {
		char* uuid;
		char* username;
};

#define PKT_LOGIN_CLIENT_SETCOMPRESSION 3

struct pkt_login_client_setcompression {
		int32_t threshold;
};

#define PKT_LOGIN_SERVER_LOGINSTART 0

struct pkt_login_server_loginstart {
		char* name;
};

#define PKT_LOGIN_SERVER_ENCRYPTIONRESPONSE 1

struct pkt_login_server_encryptionresponse {
		int32_t shared_secret_length;
		uint8_t* shared_secret;
		int32_t verify_token_length;
		uint8_t* verify_token;
};

union pkt_handshake_client {
};

union pkt_handshake_server {
		struct pkt_handshake_server_handshake handshake;
};

union pkt_play_client {
		struct pkt_play_client_spawnobject spawnobject;
		struct pkt_play_client_spawnexperienceorb spawnexperienceorb;
		struct pkt_play_client_spawnglobalentity spawnglobalentity;
		struct pkt_play_client_spawnmob spawnmob;
		struct pkt_play_client_spawnpainting spawnpainting;
		struct pkt_play_client_spawnplayer spawnplayer;
		struct pkt_play_client_animation animation;
		struct pkt_play_client_statistics statistics;
		struct pkt_play_client_blockbreakanimation blockbreakanimation;
		struct pkt_play_client_updateblockentity updateblockentity;
		struct pkt_play_client_blockaction blockaction;
		struct pkt_play_client_blockchange blockchange;
		struct pkt_play_client_bossbar bossbar;
		struct pkt_play_client_serverdifficulty serverdifficulty;
		struct pkt_play_client_tabcomplete tabcomplete;
		struct pkt_play_client_chatmessage chatmessage;
		struct pkt_play_client_multiblockchange multiblockchange;
		struct pkt_play_client_confirmtransaction confirmtransaction;
		struct pkt_play_client_closewindow closewindow;
		struct pkt_play_client_openwindow openwindow;
		struct pkt_play_client_windowitems windowitems;
		struct pkt_play_client_windowproperty windowproperty;
		struct pkt_play_client_setslot setslot;
		struct pkt_play_client_setcooldown setcooldown;
		struct pkt_play_client_pluginmessage pluginmessage;
		struct pkt_play_client_namedsoundeffect namedsoundeffect;
		struct pkt_play_client_disconnect disconnect;
		struct pkt_play_client_entitystatus entitystatus;
		struct pkt_play_client_explosion explosion;
		struct pkt_play_client_unloadchunk unloadchunk;
		struct pkt_play_client_changegamestate changegamestate;
		struct pkt_play_client_keepalive keepalive;
		struct pkt_play_client_chunkdata chunkdata;
		struct pkt_play_client_effect effect;
		struct pkt_play_client_particle particle;
		struct pkt_play_client_joingame joingame;
		struct pkt_play_client_map map;
		struct pkt_play_client_entityrelativemove entityrelativemove;
		struct pkt_play_client_entitylookandrelativemove entitylookandrelativemove;
		struct pkt_play_client_entitylook entitylook;
		struct pkt_play_client_entity entity;
		struct pkt_play_client_vehiclemove vehiclemove;
		struct pkt_play_client_opensigneditor opensigneditor;
		struct pkt_play_client_playerabilities playerabilities;
		struct pkt_play_client_combatevent combatevent;
		struct pkt_play_client_playerlistitem playerlistitem;
		struct pkt_play_client_playerpositionandlook playerpositionandlook;
		struct pkt_play_client_usebed usebed;
		struct pkt_play_client_destroyentities destroyentities;
		struct pkt_play_client_removeentityeffect removeentityeffect;
		struct pkt_play_client_resourcepacksend resourcepacksend;
		struct pkt_play_client_respawn respawn;
		struct pkt_play_client_entityheadlook entityheadlook;
		struct pkt_play_client_worldborder worldborder;
		struct pkt_play_client_camera camera;
		struct pkt_play_client_helditemchange helditemchange;
		struct pkt_play_client_displayscoreboard displayscoreboard;
		struct pkt_play_client_entitymetadata entitymetadata;
		struct pkt_play_client_attachentity attachentity;
		struct pkt_play_client_entityvelocity entityvelocity;
		struct pkt_play_client_entityequipment entityequipment;
		struct pkt_play_client_setexperience setexperience;
		struct pkt_play_client_updatehealth updatehealth;
		struct pkt_play_client_scoreboardobjective scoreboardobjective;
		struct pkt_play_client_setpassengers setpassengers;
		struct pkt_play_client_teams teams;
		struct pkt_play_client_updatescore updatescore;
		struct pkt_play_client_spawnposition spawnposition;
		struct pkt_play_client_timeupdate timeupdate;
		struct pkt_play_client_title title;
		struct pkt_play_client_soundeffect soundeffect;
		struct pkt_play_client_playerlistheaderandfooter playerlistheaderandfooter;
		struct pkt_play_client_collectitem collectitem;
		struct pkt_play_client_entityteleport entityteleport;
		struct pkt_play_client_entityproperties entityproperties;
		struct pkt_play_client_entityeffect entityeffect;
};

union pkt_play_server {
		struct pkt_play_server_teleportconfirm teleportconfirm;
		struct pkt_play_server_tabcomplete tabcomplete;
		struct pkt_play_server_chatmessage chatmessage;
		struct pkt_play_server_clientstatus clientstatus;
		struct pkt_play_server_clientsettings clientsettings;
		struct pkt_play_server_confirmtransaction confirmtransaction;
		struct pkt_play_server_enchantitem enchantitem;
		struct pkt_play_server_clickwindow clickwindow;
		struct pkt_play_server_closewindow closewindow;
		struct pkt_play_server_pluginmessage pluginmessage;
		struct pkt_play_server_useentity useentity;
		struct pkt_play_server_keepalive keepalive;
		struct pkt_play_server_playerposition playerposition;
		struct pkt_play_server_playerpositionandlook playerpositionandlook;
		struct pkt_play_server_playerlook playerlook;
		struct pkt_play_server_player player;
		struct pkt_play_server_vehiclemove vehiclemove;
		struct pkt_play_server_steerboat steerboat;
		struct pkt_play_server_playerabilities playerabilities;
		struct pkt_play_server_playerdigging playerdigging;
		struct pkt_play_server_entityaction entityaction;
		struct pkt_play_server_steervehicle steervehicle;
		struct pkt_play_server_resourcepackstatus resourcepackstatus;
		struct pkt_play_server_helditemchange helditemchange;
		struct pkt_play_server_creativeinventoryaction creativeinventoryaction;
		struct pkt_play_server_updatesign updatesign;
		struct pkt_play_server_animation animation;
		struct pkt_play_server_spectate spectate;
		struct pkt_play_server_playerblockplacement playerblockplacement;
		struct pkt_play_server_useitem useitem;
};

union pkt_status_client {
		struct pkt_status_client_response response;
		struct pkt_status_client_pong pong;
};

union pkt_status_server {
		struct pkt_status_server_request request;
		struct pkt_status_server_ping ping;
};

union pkt_login_client {
		struct pkt_login_client_disconnect disconnect;
		struct pkt_login_client_encryptionrequest encryptionrequest;
		struct pkt_login_client_loginsuccess loginsuccess;
		struct pkt_login_client_setcompression setcompression;
};

union pkt_login_server {
		struct pkt_login_server_loginstart loginstart;
		struct pkt_login_server_encryptionresponse encryptionresponse;
};

union pkts {
		union pkt_handshake_client handshake_client;
		union pkt_handshake_server handshake_server;
		union pkt_play_client play_client;
		union pkt_play_server play_server;
		union pkt_status_client status_client;
		union pkt_status_server status_server;
		union pkt_login_client login_client;
		union pkt_login_server login_server;
};

struct packet {
		int32_t id;
		union pkts data;
};

ssize_t readPacket(struct conn* conn, unsigned char* buf, size_t buflen, struct packet* packet);

ssize_t writePacket(struct conn* conn, struct packet* packet);

void freePacket(int state, int dir, struct packet* packet);

#endif
