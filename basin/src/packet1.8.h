/*
 * packet1.9.h
 *
 *  Created on: Jun 23, 2016
 *      Author: root
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "network.h"

#define PKT_HANDSHAKE_CLIENT_HANDSHAKE 0x00
#define PKT_LOGIN_CLIENT_LOGINSTART 0x00
#define PKT_LOGIN_CLIENT_ENCRYPTIONRESPONSE 0x01
#define PKT_LOGIN_SERVER_DISCONNECT 0x00
#define PKT_LOGIN_SERVER_ENCRYPTIONREQUEST 0x01
#define PKT_LOGIN_SERVER_LOGINSUCCESS 0x02
#define PKT_LOGIN_SERVER_SETCOMPRESSION 0x03
#define PKT_PLAY_CLIENT_KEEPALIVE 0x00
#define PKT_PLAY_CLIENT_CHATMESSAGE 0x01
#define PKT_PLAY_CLIENT_USEENTITY 0x02
#define PKT_PLAY_CLIENT_PLAYER 0x03
#define PKT_PLAY_CLIENT_PLAYERPOSITION 0x04
#define PKT_PLAY_CLIENT_PLAYERLOOK 0x05
#define PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK 0x06
#define PKT_PLAY_CLIENT_PLAYERDIG 0x07
#define PKT_PLAY_CLIENT_PLAYERPLACE 0x08
#define PKT_PLAY_CLIENT_HELDITEMCHANGE 0x09
#define PKT_PLAY_CLIENT_ANIMATION 0x0A
#define PKT_PLAY_CLIENT_ENTITYACTION 0x0B
#define PKT_PLAY_CLIENT_STEERVEHICLE 0x0C
#define PKT_PLAY_CLIENT_CLOSEWINDOW 0x0D
#define PKT_PLAY_CLIENT_CLICKWINDOW 0x0E
#define PKT_PLAY_CLIENT_CONFIRMTRANSACTION 0x0F
#define PKT_PLAY_CLIENT_CREATIVEINVENTORYACTION 0x10
#define PKT_PLAY_CLIENT_ENCHANTITEM 0x11
#define PKT_PLAY_CLIENT_UPDATESIGN 0x12
#define PKT_PLAY_CLIENT_PLAYERABILITIES 0x13
#define PKT_PLAY_CLIENT_TABCOMPLETE 0x14
#define PKT_PLAY_CLIENT_CLIENTSETTINGS 0x15
#define PKT_PLAY_CLIENT_CLIENTSTATUS 0x16
#define PKT_PLAY_CLIENT_PLUGINMESSAGE 0x17
#define PKT_PLAY_CLIENT_SPECTATE 0x18
#define PKT_PLAY_CLIENT_RESOURCEPACKSTATUS 0x19
#define PKT_PLAY_SERVER_KEEPALIVE 0x00
#define PKT_PLAY_SERVER_JOINGAME 0x01
#define PKT_PLAY_SERVER_CHATMESSAGE 0x02
#define PKT_PLAY_SERVER_TIMEUPDATE 0x03
#define PKT_PLAY_SERVER_ENTITYEQUIPMENT 0x04
#define PKT_PLAY_SERVER_SPAWNPOSITION 0x05
#define PKT_PLAY_SERVER_UPDATEHEALTH 0x06
#define PKT_PLAY_SERVER_RESPAWN 0x07
#define PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK 0x08
#define PKT_PLAY_SERVER_HELDITEMCHANGE 0x09
#define PKT_PLAY_SERVER_USEBED 0x0A
#define PKT_PLAY_SERVER_ANIMATION 0x0B
#define PKT_PLAY_SERVER_SPAWNPLAYER 0x0C
#define PKT_PLAY_SERVER_COLLECTITEM 0x0D
#define PKT_PLAY_SERVER_SPAWNOBJECT 0x0E
#define PKT_PLAY_SERVER_SPAWNMOB 0x0F
#define PKT_PLAY_SERVER_SPAWNPAINTING 0x10
#define PKT_PLAY_SERVER_SPAWNEXPERIENCEORB 0x11
#define PKT_PLAY_SERVER_ENTITYVELOCITY 0x12
#define PKT_PLAY_SERVER_DESTROYENTITIES 0x13
#define PKT_PLAY_SERVER_ENTITY 0x14
#define PKT_PLAY_SERVER_ENTITYRELMOVE 0x15
#define PKT_PLAY_SERVER_ENTITYLOOK 0x16
#define PKT_PLAY_SERVER_ENTITYLOOKANDRELMOVE 0x17
#define PKT_PLAY_SERVER_ENTITYTELEPORT 0x18
#define PKT_PLAY_SERVER_ENTITYHEADLOOK 0x19
#define PKT_PLAY_SERVER_ENTITYSTATUS 0x1A
#define PKT_PLAY_SERVER_ATTACHENTITY 0x1B
#define PKT_PLAY_SERVER_ENTITYMETADATA 0x1C
#define PKT_PLAY_SERVER_ENTITYEFFECT 0x1D
#define PKT_PLAY_SERVER_REMOVEENTITYEFFECT 0x1E
#define PKT_PLAY_SERVER_SETEXPERIENCE 0x1F
#define PKT_PLAY_SERVER_ENTITYPROPERTIES 0x20
#define PKT_PLAY_SERVER_CHUNKDATA 0x21
#define PKT_PLAY_SERVER_MULTIBLOCKCHANGE 0x22
#define PKT_PLAY_SERVER_BLOCKCHANGE 0x23
#define PKT_PLAY_SERVER_BLOCKACTION 0x24
#define PKT_PLAY_SERVER_BLOCKBREAKANIMATION 0x25
#define PKT_PLAY_SERVER_MAPCHUNKBULK 0x26
#define PKT_PLAY_SERVER_EXPLOSION 0x27
#define PKT_PLAY_SERVER_EFFECT 0x28
#define PKT_PLAY_SERVER_SOUNDEFFECT 0x29
#define PKT_PLAY_SERVER_PARTICLE 0x2A
#define PKT_PLAY_SERVER_CHANGEGAMESTATE 0x2B
#define PKT_PLAY_SERVER_SPAWNGLOBALENTITY 0x2C
#define PKT_PLAY_SERVER_OPENWINDOW 0x2D
#define PKT_PLAY_SERVER_CLOSEWINDOW 0x2E
#define PKT_PLAY_SERVER_SETSLOT 0x2F
#define PKT_PLAY_SERVER_WINDOWITEMS 0x30
#define PKT_PLAY_SERVER_WINDOWPROPERTY 0x31
#define PKT_PLAY_SERVER_CONFIRMTRANSACTION 0x32
#define PKT_PLAY_SERVER_UPDATESIGN 0x33
#define PKT_PLAY_SERVER_MAP 0x34
#define PKT_PLAY_SERVER_UPDATEBLOCKENTITY 0x35
#define PKT_PLAY_SERVER_OPENSIGNEDITOR 0x36
#define PKT_PLAY_SERVER_STATISTICS 0x37
#define PKT_PLAY_SERVER_PLAYERLISTITEM 0x38
#define PKT_PLAY_SERVER_PLAYERABILITIES 0x39
#define PKT_PLAY_SERVER_TABCOMPLETE 0x3A
#define PKT_PLAY_SERVER_SCOREBOARDOBJECTIVE 0x3B
#define PKT_PLAY_SERVER_UPDATESCORE 0x3C
#define PKT_PLAY_SERVER_DISPLAYSCOREBOARD 0x3D
#define PKT_PLAY_SERVER_TEAMS 0x3E
#define PKT_PLAY_SERVER_PLUGINMESSAGE 0x3F
#define PKT_PLAY_SERVER_DISCONNECT 0x40
#define PKT_PLAY_SERVER_SERVERDIFFICULTY 0x41
#define PKT_PLAY_SERVER_COMBATEVENT 0x42
#define PKT_PLAY_SERVER_CAMERA 0x43
#define PKT_PLAY_SERVER_WORLDBORDER 0x44
#define PKT_PLAY_SERVER_TITLE 0x45
#define PKT_PLAY_SERVER_SETCOMPRESSION 0x46
#define PKT_PLAY_SERVER_PLAYERLISTHEADERFOOTER 0x47
#define PKT_PLAY_SERVER_RESOURCEPACKSEND 0x48
#define PKT_PLAY_SERVER_UPDATEENTITYNBT 0x49
#define PKT_STATUS_CLIENT_REQUEST 0x00
#define PKT_STATUS_CLIENT_PING 0x01
#define PKT_STATUS_SERVER_RESPONSE 0x00
#define PKT_STATUS_SERVER_PONG 0x01

struct pkt_handshake_client_handshake {
		int32_t protocol_version;
		char* ip;
		uint16_t port;
		int32_t state;
};

struct pkt_login_client_loginstart {
		char* name;
};

struct pkt_login_client_encryptionresponse {
		int32_t sharedSecret_size;
		unsigned char* sharedSecret;
		int32_t verifyToken_size;
		unsigned char* verifyToken;
};

struct pkt_login_server_disconnect {
		char* reason;
};

struct pkt_login_server_encryptionrequest {
		char* serverID;
		int32_t publicKey_size;
		unsigned char* publicKey;
		int32_t verifyToken_size;
		unsigned char* verifyToken;
};

struct pkt_login_server_loginsuccess {
		char* UUID;
		char* username;
};

struct pkt_login_server_setcompression {
		size_t threshold;
};

struct pkt_play_client_keepalive {
		int32_t key;
};

struct pkt_play_client_chatmessage {
		char* message;
};

struct pkt_play_client_useentity {
		int32_t target;
		int32_t type;
		float targetX;
		float targetY;
		float targetZ;
};

struct pkt_play_client_player {
		mcbool onGround;
};

struct pkt_play_client_playerpos {
		double x;
		double y;
		double z;
		mcbool onGround;
};

struct pkt_play_client_playerlook {
		float yaw;
		float pitch;
		mcbool onGround;
};

struct pkt_play_client_playerposlook {
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
		mcbool onGround;
};

struct pkt_play_client_playerdig {
		unsigned char status;
		struct encpos pos;
		unsigned char face;
};

struct pkt_play_client_playerplace {
		struct encpos loc;
		int32_t face;
		struct slot slot;
		signed char curPosX;
		signed char curPosY;
		signed char curPosZ;
};

struct pkt_play_client_helditemchange {
		int16_t slot;
};

struct pkt_play_client_animation {
};

struct pkt_play_client_entityaction {
		int32_t entityID;
		int32_t actionID;
		int32_t actionParameter;
};

struct pkt_play_client_steervehicle {
		float sideways;
		float forward;
		unsigned char flags;
};

struct pkt_play_client_closewindow {
		unsigned char windowID;
};

struct pkt_play_client_clickwindow {
		unsigned char windowID;
		int16_t slot;
		signed char button;
		uint16_t actionNumber;
		signed char mode;
		struct slot clicked;
};

struct pkt_play_client_confirmtransaction {
		unsigned char windowID;
		uint16_t actionNumber;
		mcbool accepted;
};

struct pkt_play_client_creativeinventoryaction {
		int16_t slot;
		struct slot clicked;
};

struct pkt_play_client_enchantitem {
		unsigned char windowID;
		signed char enchantment;
};

struct pkt_play_client_updatesign {
		struct encpos pos;
		char* line[4];
};

struct pkt_play_client_playerabilities {
		unsigned char flags;
		float flyingSpeed;
		float walkingSpeed;
};

struct pkt_play_client_tabcomplete {
		char* text;
		mcbool haspos;
		struct encpos pos;
};

struct pkt_play_client_clientsettings {
		char* locale;
		unsigned char viewDistance;
		unsigned char chatMode;
		mcbool chatColors;
		unsigned char displayedSkin;
};

struct pkt_play_client_clientstatus {
		int32_t actionID;
};

struct pkt_play_client_pluginmessage {
		char* channel;
		size_t data_size;
		unsigned char* data;
};

struct pkt_play_client_spectate {
		struct uuid uuid;
};

struct pkt_play_client_resourcepackstatus {
		char* hash;
		int32_t result;
};

struct pkt_play_server_keepalive {
		int32_t key;
};

struct pkt_play_server_joingame {
		int32_t eid;
		unsigned char gamemode;
		signed char dimension;
		unsigned char difficulty;
		unsigned char maxPlayers;
		char* levelType;
		mcbool reducedDebugInfo;
};

struct pkt_play_server_chatmessage {
		char* json;
		signed char pos;
};

struct pkt_play_server_timeupdate {
		int64_t worldAge;
		int64_t time;
};

struct pkt_play_server_entityequipment {
		int32_t entityID;
		int16_t slot;
		struct slot slotitem;
};

struct pkt_play_server_spawnposition {
		struct encpos pos;
};

struct pkt_play_server_updatehealth {
		float health;
		int32_t food;
		float saturation;
};

struct pkt_play_server_respawn {
		int32_t dimension;
		unsigned char difficulty;
		unsigned char gamemode;
		char* levelType;
};

struct pkt_play_server_playerposlook {
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
		unsigned char flags;
};

struct pkt_play_server_helditemchange {
		signed char slot;
};

struct pkt_play_server_usebed {
		int32_t entityID;
		struct encpos pos;
};

struct pkt_play_server_animation {
		int32_t entityID;
		unsigned char anim;
};

struct pkt_play_server_spawnplayer {
		int32_t entityID;
		struct uuid uuid;
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
		unsigned char* metadata;
		size_t metadata_size;
};

struct pkt_play_server_collectitem {
		int32_t collectedEntityID;
		int32_t collectorEntityID;
};

struct pkt_play_server_spawnobject {
		int32_t entityID;
		struct uuid uuid;
		unsigned char type;
		double x;
		double y;
		double z;
		float pitch;
		float yaw;
		int32_t data;
		float velX;
		float velY;
		float velZ;
};

struct pkt_play_server_spawnmob {
		int32_t entityID;
		struct uuid uuid;
		unsigned char type;
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
		float headPitch;
		float velX;
		float velY;
		float velZ;
		unsigned char* metadata;
		size_t metadata_size;
};

struct pkt_play_server_spawnpainting {
		int32_t entityID;
		char* title;
		struct encpos loc;
		unsigned char direction;
};

struct pkt_play_server_spawnexperienceorb {
		int32_t entityID;
		double x;
		double y;
		double z;
		int16_t count;
};

struct pkt_play_server_entityvelocity {
		int32_t entityID;
		float velX;
		float velY;
		float velZ;
};

struct pkt_play_server_destroyentities {
		int32_t count;
		int32_t* entityIDs;
};

struct pkt_play_server_entity {
		int32_t entityID;
};

struct pkt_play_server_entityrelmove {
		int32_t entityID;
		float dx;
		float dy;
		float dz;
		mcbool onGround;
};

struct pkt_play_server_entitylook {
		int32_t entityID;
		float yaw;
		float pitch;
		mcbool onGround;
};

struct pkt_play_server_entitylookrelmove {
		int32_t entityID;
		float dx;
		float dy;
		float dz;
		float yaw;
		float pitch;
		mcbool onGround;
};

struct pkt_play_server_entityteleport {
		int32_t entityID;
		double x;
		double y;
		double z;
		float yaw;
		float pitch;
		mcbool onGround;
};

struct pkt_play_server_entityheadlook {
		int32_t entityID;
		float headYaw;
};

struct pkt_play_server_entitystatus {
		int32_t entityID;
		signed char status;
};

struct pkt_play_server_attachentity {
		int32_t entityID;
		int32_t vehicleID;
};

struct pkt_play_server_entitymetadata {
		int32_t entityID;
		unsigned char* metadata;
		size_t metadata_size;
};

struct pkt_play_server_entityeffect {
		int32_t entityID;
		signed char effectID;
		signed char amplifier;
		int32_t duration;
		unsigned char hideParticles;
};

struct pkt_play_server_removeentityeffect {
		int32_t entityID;
		signed char effectID;
};

struct pkt_play_server_setexperience {
		float bar;
		int32_t level;
		int32_t total;
};

struct entity_properties {
		char* key;
		double value;
		int32_t mod_count;
		struct entity_mod {
				struct uuid uuid;
				double amount;
				signed char op;
		}* mods;
};

struct pkt_play_server_entityproperties {
		int32_t entityID;
		int32_t properties_count;
		struct entity_properties* properties;
};

struct pkt_play_server_chunkdata {
		int32_t x;
		int32_t z;
		mcbool continuous;
		int32_t bitMask;
		int32_t size;
		unsigned char* data;
		unsigned char biomes[256];
		int32_t nbtc;
		struct nbt_tag** nbts;
};

struct pkt_play_server_multiblockchange {
		int32_t x;
		int32_t z;
		int32_t record_count;
		struct __attribute__((__packed__)) mbc_record {
				unsigned char x :4;
				unsigned char z :4;
				unsigned char y;
				int32_t blockID;
		}* records;
};

struct pkt_play_server_blockchange {
		struct encpos pos;
		int32_t blockID;
};

struct pkt_play_server_blockaction {
		struct encpos pos;
		unsigned char byte1;
		unsigned char byte2;
		int32_t type;
};

struct pkt_play_server_blockbreakanimation {
		int32_t entityID;
		struct encpos pos;
		signed char stage;
};

struct pkt_play_server_mapchunkbulk {
		int32_t x;
		int32_t z;
		mcbool continuous;
		int32_t bitMask;
		int32_t size;
		unsigned char* data;
		unsigned char biomes[256];
		int32_t nbtc;
		struct nbt_tag** nbts;
};

struct pkt_play_server_explosion {
		float x;
		float y;
		float z;
		float radius;
		int32_t record_count;
		struct __attribute__((__packed__)) exp_record {
				int8_t x;
				int8_t y;
				int8_t z;
		}* records;
		float velX;
		float velY;
		float velZ;
};

struct pkt_play_server_effect {
		int32_t effectID;
		struct encpos pos;
		int32_t data;
		mcbool disableRelVolume;
};

struct pkt_play_server_soundeffect {
		int32_t id;
		int32_t category;
		int32_t x;
		int32_t y;
		int32_t z;
		float volume;
		float pitch;
};

struct pkt_play_server_particles {
		int32_t particleID;
		mcbool longDistance;
		float x;
		float y;
		float z;
		float offX;
		float offY;
		float offZ;
		float data;
		int32_t count;
		int32_t data1;
		int32_t data2;
};

struct pkt_play_server_changegamestate {
		unsigned char reason;
		float value;
};

struct pkt_play_server_spawnglobalentity {
		int32_t entityID;
		signed char type;
		double x;
		double y;
		double z;
};

struct pkt_play_server_openwindow {
		unsigned char windowID;
		char* type;
		char* title;
		unsigned char slot_count;
		int32_t entityID;
};

struct pkt_play_server_closewindow {
		unsigned char windowID;
};

struct pkt_play_server_setslot {
		unsigned char windowID;
		int16_t slot;
		struct slot data;
};

struct pkt_play_server_windowitems {
		unsigned char windowID;
		int16_t count;
		struct slot* slots;
};

struct pkt_play_server_windowproperty {
		unsigned char windowID;
		int16_t property;
		int16_t value;
};

struct pkt_play_server_confirmtransaction {
		unsigned char windowID;
		uint16_t actionID;
		mcbool accepted;
};

struct pkt_play_server_updatesign {
		struct encpos pos;
		char* lines[4];
};

struct pkt_play_server_map {
		int32_t damage;
		signed char scale;
		mcbool showIcons;
		int32_t icon_count;
		struct __attribute__((__packed__)) map_icon {
				uint8_t direction :4;
				uint8_t type :4;
				uint8_t x;
				uint8_t z;
		}* icons;
		signed char columns;
		signed char rows;
		signed char x;
		signed char z;
		int32_t data_size;
		void* data;
};

struct pkt_play_server_updateblockentity {
		struct encpos pos;
		unsigned char action;
		struct nbt_tag* nbt;
};

struct pkt_play_server_opensigneditor {
		struct encpos pos;
};

struct statistic {
		char* name;
		int32_t value;
};

struct pkt_play_server_statistics {
		int32_t stat_count;
		struct statistic* stats;
};

struct li_property {
		char* name;
		char* value;
		char* signature;
};

struct li_add {
		char* name;
		int32_t prop_count;
		struct li_property* props;
		int32_t gamemode;
		int32_t ping;
		char* displayName;
};

union li_types {
		struct li_add add;
		int32_t gamemode;
		int32_t latency;
		char* displayName;
};

struct li_player {
		struct uuid uuid;
		union li_types type;
};

struct pkt_play_server_playerlistitem {
		int32_t action;
		int32_t player_count;
		struct li_player* players;
};

struct pkt_play_server_playerabilities {
		signed char flags;
		float flyingSpeed;
		float fov;
};

struct pkt_play_server_tabcomplete {
		int32_t count;
		char** matches;
};

struct pkt_play_server_scoreboardobjective {
		char* name;
		signed char mode;
		char* value;
		char* type;
};

struct pkt_play_server_updatescore {
		char* scorename;
		signed char action;
		char* objname;
		int32_t value;
};

struct pkt_play_server_displayscoreboard {
		signed char pos;
		char* name;
};

struct pkt_play_server_teams {
		char* name;
		signed char mode;
		char* displayName;
		char* prefix;
		char* suffix;
		signed char friendlyFire;
		char* nameTagVisibility;
		char* collisionRule;
		signed char color;
		int32_t player_count;
		char** players;
};

struct pkt_play_server_pluginmessage {
		char* channel;
		size_t data_size;
		void* data;
};

struct pkt_play_server_disconnect {
		char* reason;
};

struct pkt_play_server_serverdifficulty {
		unsigned char difficulty;
};

struct pkt_play_server_combatevent {
		int32_t event;
		int32_t duration;
		int32_t playerId;
		int32_t entityID;
		char* message;
};

struct pkt_play_server_camera {
		int32_t cameraID;
};

struct wb_lerpsize {
		double oldradius;
		double newradius;
		int64_t speed;
};

struct wp_setcenter {
		double x;
		double z;
};

struct wp_initialize {
		double x;
		double z;
		double oldradius;
		double newradius;
		int64_t speed;
		int32_t portalBoundary;
		int32_t warningTime;
		int32_t warningBlocks;
};

union wb_action {
		double setsize_radius;
		struct wb_lerpsize lerpsize;
		struct wp_setcenter setcenter;
		struct wp_initialize initialize;
		int32_t warningTime;
		int32_t warningBlocks;
};

struct pkt_play_server_worldborder {
		int32_t action;
		union wb_action wb_action;
};

struct title_set {
		int32_t fadein;
		int32_t stay;
		int32_t fadeout;
};

union title_action {
		char* title;
		char* subtitle;
		struct title_set set;
};

struct pkt_play_server_title {
		int32_t action;
		union title_action act;
};

struct pkt_play_server_setcompression {
		int32_t threshold;
};

struct pkt_play_server_playerlistheaderfooter {
		char* header;
		char* footer;
};

struct pkt_play_server_resourcepacksend {
		char* url;
		char* hash;
};

struct pkt_play_server_updateentitynbt {
		int32_t id;
		struct nbt_tag* nbt;
};

union pkt_state_handshake_client {
		struct pkt_handshake_client_handshake handshake;
};

union pkt_state_login_client {
		struct pkt_login_client_loginstart loginstart;
		struct pkt_login_client_encryptionresponse encryptionresponse;
};

union pkt_state_login_server {
		struct pkt_login_server_disconnect disconnect;
		struct pkt_login_server_encryptionrequest encryptionrequest;
		struct pkt_login_server_loginsuccess loginsuccess;
		struct pkt_login_server_setcompression setcompression;
};

struct pkt_state_status {

};

union pkt_state_play_client {
		struct pkt_play_client_keepalive keepalive;
		struct pkt_play_client_chatmessage chatmessage;
		struct pkt_play_client_useentity useentity;
		struct pkt_play_client_player player;
		struct pkt_play_client_playerpos playerpos;
		struct pkt_play_client_playerlook playerlook;
		struct pkt_play_client_playerposlook playerposlook;
		struct pkt_play_client_playerdig playerdig;
		struct pkt_play_client_playerplace playerplace;
		struct pkt_play_client_helditemchange helditemchange;
		struct pkt_play_client_animation animation;
		struct pkt_play_client_entityaction entityaction;
		struct pkt_play_client_steervehicle steervehicle;
		struct pkt_play_client_closewindow closewindow;
		struct pkt_play_client_clickwindow clickwindow;
		struct pkt_play_client_confirmtransaction confirmtransaction;
		struct pkt_play_client_creativeinventoryaction creativeinventoryaction;
		struct pkt_play_client_enchantitem enchantitem;
		struct pkt_play_client_updatesign updatesign;
		struct pkt_play_client_playerabilities playerabilities;
		struct pkt_play_client_tabcomplete tabcomplete;
		struct pkt_play_client_clientsettings clientsettings;
		struct pkt_play_client_clientstatus clientstatus;
		struct pkt_play_client_pluginmessage pluginmessage;
		struct pkt_play_client_spectate spectate;
		struct pkt_play_client_resourcepackstatus resourcepackstatus;
};

union pkt_state_play_server {
		struct pkt_play_server_keepalive keepalive;
		struct pkt_play_server_joingame joingame;
		struct pkt_play_server_chatmessage chatmessage;
		struct pkt_play_server_timeupdate timeupdate;
		struct pkt_play_server_entityequipment entityequipment;
		struct pkt_play_server_spawnposition spawnposition;
		struct pkt_play_server_updatehealth updatehealth;
		struct pkt_play_server_respawn respawn;
		struct pkt_play_server_playerposlook playerposlook;
		struct pkt_play_server_helditemchange helditemchange;
		struct pkt_play_server_usebed usebed;
		struct pkt_play_server_animation animation;
		struct pkt_play_server_spawnplayer spawnplayer;
		struct pkt_play_server_collectitem collectitem;
		struct pkt_play_server_spawnobject spawnobject;
		struct pkt_play_server_spawnmob spawnmob;
		struct pkt_play_server_spawnpainting spawnpainting;
		struct pkt_play_server_spawnexperienceorb spawnexperienceorb;
		struct pkt_play_server_entityvelocity entityvelocity;
		struct pkt_play_server_destroyentities destroyentities;
		struct pkt_play_server_entity entity;
		struct pkt_play_server_entityrelmove entityrelmove;
		struct pkt_play_server_entitylook entitylook;
		struct pkt_play_server_entitylookrelmove entitylookrelmove;
		struct pkt_play_server_entityteleport entityteleport;
		struct pkt_play_server_entityheadlook entityheadlook;
		struct pkt_play_server_entitystatus entitystatus;
		struct pkt_play_server_attachentity attachentity;
		struct pkt_play_server_entitymetadata entitymetadata;
		struct pkt_play_server_entityeffect entityeffect;
		struct pkt_play_server_removeentityeffect removeentityeffect;
		struct pkt_play_server_setexperience setexperience;
		struct pkt_play_server_entityproperties entityproperties;
		struct pkt_play_server_chunkdata chunkdata;
		struct pkt_play_server_multiblockchange multiblockchange;
		struct pkt_play_server_blockchange blockchange;
		struct pkt_play_server_blockaction blockaction;
		struct pkt_play_server_blockbreakanimation blockbreakanimation;
		struct pkt_play_server_mapchunkbulk mapchunkbulk;
		struct pkt_play_server_explosion explosion;
		struct pkt_play_server_effect effect;
		struct pkt_play_server_soundeffect soundeffect;
		struct pkt_play_server_particles particles;
		struct pkt_play_server_changegamestate changegamestate;
		struct pkt_play_server_spawnglobalentity spawnglobalentity;
		struct pkt_play_server_openwindow openwindow;
		struct pkt_play_server_closewindow closewindow;
		struct pkt_play_server_setslot setslot;
		struct pkt_play_server_windowitems windowitems;
		struct pkt_play_server_windowproperty windowproperty;
		struct pkt_play_server_confirmtransaction confirmtransaction;
		struct pkt_play_server_updatesign updatesign;
		struct pkt_play_server_map map;
		struct pkt_play_server_updateblockentity updateblockentity;
		struct pkt_play_server_opensigneditor opensigneditor;
		struct pkt_play_server_statistics statistics;
		struct pkt_play_server_playerlistitem playerlistitem;
		struct pkt_play_server_playerabilities playerabilities;
		struct pkt_play_server_tabcomplete tabcomplete;
		struct pkt_play_server_scoreboardobjective scoreboardobjective;
		struct pkt_play_server_updatescore updatescore;
		struct pkt_play_server_displayscoreboard displayscoreboard;
		struct pkt_play_server_teams teams;
		struct pkt_play_server_pluginmessage pluginmessage;
		struct pkt_play_server_disconnect disconnect;
		struct pkt_play_server_serverdifficulty serverdifficulty;
		struct pkt_play_server_combatevent combatevent;
		struct pkt_play_server_camera camera;
		struct pkt_play_server_worldborder worldborder;
		struct pkt_play_server_title title;
		struct pkt_play_server_setcompression setcompression;
		struct pkt_play_server_playerlistheaderfooter playerlistheaderfooter;
		struct pkt_play_server_resourcepacksend resourcepacksend;
		struct pkt_play_server_updateentitynbt updateentitynbt;
};

union pkts {
		union pkt_state_handshake_client handshake_client;
		union pkt_state_login_client login_client;
		union pkt_state_login_server login_server;
		union pkt_state_play_client play_client;
		union pkt_state_play_server play_server;
		struct pkt_state_status status;
};

struct packet {
		int32_t id;
		union pkts data;
};

int readPacket(struct conn* conn, struct packet* packet);

int writePacket(struct conn* conn, struct packet* packet);

#endif /* PACKET1_9_H_ */
