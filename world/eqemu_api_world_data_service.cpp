#include <fmt/format.h>
#include "clientlist.h"
#include "cliententry.h"
#include "eqemu_api_world_data_service.h"
#include "zoneserver.h"
#include "zonelist.h"
#include "../common/database_schema.h"
#include "../common/zone_store.h"
#include "worlddb.h"
#include "wguild_mgr.h"
#include "world_config.h"
#include "ucs.h"
#include "queryserv.h"

extern ZSList            zoneserver_list;
extern ClientList        client_list;
extern WorldGuildManager guild_mgr;
extern UCSConnection UCSLink;
extern QueryServConnection QSLink;

void callGetZoneList(Json::Value &response)
{
	for (auto &zone: zoneserver_list.getZoneServerList()) {
		Json::Value row;

		if (!zone->IsConnected()) {
			continue;
		}

		row["booting_up"]           = zone->IsBootingUp();
		row["client_address"]       = zone->GetCAddress();
		row["client_local_address"] = zone->GetCLocalAddress();
		row["client_port"]          = zone->GetCPort();
		row["compile_time"]         = zone->GetCompileTime();
		row["id"]                   = zone->GetID();
		row["instance_id"]          = zone->GetInstanceID();
		row["ip"]                   = zone->GetIP();
		row["is_static_zone"]       = zone->IsStaticZone();
		row["launch_name"]          = zone->GetLaunchName();
		row["launched_name"]        = zone->GetLaunchedName();
		row["number_players"]       = zone->NumPlayers();
		row["port"]                 = zone->GetPort();
		row["previous_zone_id"]     = zone->GetPrevZoneID();
		row["uuid"]                 = zone->GetUUID();
		row["zone_id"]              = zone->GetZoneID();
		row["zone_long_name"]       = zone->GetZoneLongName();
		row["zone_name"]            = zone->GetZoneName();
		row["zone_os_pid"]          = zone->GetZoneOSProcessID();

		response.append(row);
	}
}

void callGetDatabaseSchema(Json::Value &response)
{
	Json::Value              player_tables_json;
	std::vector<std::string> player_tables = DatabaseSchema::GetPlayerTables();
	for (const auto          &table: player_tables) {
		player_tables_json.append(table);
	}

	Json::Value              content_tables_json;
	std::vector<std::string> content_tables = DatabaseSchema::GetContentTables();
	for (const auto          &table: content_tables) {
		content_tables_json.append(table);
	}

	Json::Value              server_tables_json;
	std::vector<std::string> server_tables = DatabaseSchema::GetServerTables();
	for (const auto          &table: server_tables) {
		server_tables_json.append(table);
	}

	Json::Value              login_tables_json;
	std::vector<std::string> login_tables = DatabaseSchema::GetLoginTables();
	for (const auto          &table: login_tables) {
		login_tables_json.append(table);
	}

	Json::Value              state_tables_json;
	std::vector<std::string> state_tables = DatabaseSchema::GetStateTables();
	for (const auto          &table: state_tables) {
		state_tables_json.append(table);
	}

	Json::Value              version_tables_json;
	std::vector<std::string> version_tables = DatabaseSchema::GetVersionTables();
	for (const auto          &table: version_tables) {
		version_tables_json.append(table);
	}

	Json::Value                        character_table_columns_json;
	std::map<std::string, std::string> character_table = DatabaseSchema::GetCharacterTables();
	for (const auto                    &ctc: character_table) {
		character_table_columns_json[ctc.first] = ctc.second;
	}

	Json::Value schema;

	schema["character_table_columns"] = character_table_columns_json;
	schema["content_tables"]          = content_tables_json;
	schema["login_tables"]            = login_tables_json;
	schema["player_tables"]           = player_tables_json;
	schema["server_tables"]           = server_tables_json;
	schema["state_tables"]            = state_tables_json;
	schema["version_tables"]          = version_tables_json;

	response.append(schema);
}

void callGetClientList(Json::Value &response)
{
	client_list.GetClientList(response);
}


struct Reload {
	std::string command{};
	uint16      opcode;
	std::string desc{};
};

std::vector<Reload> reload_types = {
	Reload{.command = "aa", .opcode = ServerOP_ReloadAAData, .desc = "Alternate Advancement"},
	Reload{.command = "alternate_currencies", .opcode = ServerOP_ReloadAlternateCurrencies, .desc = "Alternate Currencies"},
	Reload{.command = "base_data", .opcode = ServerOP_ReloadBaseData, .desc = "Base Data"},
	Reload{.command = "blocked_spells", .opcode = ServerOP_ReloadBlockedSpells, .desc = "Blocked Spells"},
	Reload{.command = "commands", .opcode = ServerOP_ReloadCommands, .desc = "Commands"},
	Reload{.command = "content_flags", .opcode = ServerOP_ReloadContentFlags, .desc = "Content Flags"},
	Reload{.command = "data_buckets_cache", .opcode = ServerOP_ReloadDataBucketsCache, .desc = "Data Buckets Cache"},
	Reload{.command = "doors", .opcode = ServerOP_ReloadDoors, .desc = "Doors"},
	Reload{.command = "dztemplates", .opcode = ServerOP_ReloadDzTemplates, .desc = "Dynamic Zone Templates"},
	Reload{.command = "ground_spawns", .opcode = ServerOP_ReloadGroundSpawns, .desc = "Ground Spawns"},
	Reload{.command = "level_mods", .opcode = ServerOP_ReloadLevelEXPMods, .desc = "Level Mods"},
	Reload{.command = "logs", .opcode = ServerOP_ReloadLogs, .desc = "Log Settings"},
	Reload{.command = "loot", .opcode = ServerOP_ReloadLoot, .desc = "Loot"},
	Reload{.command = "merchants", .opcode = ServerOP_ReloadMerchants, .desc = "Merchants"},
	Reload{.command = "npc_emotes", .opcode = ServerOP_ReloadNPCEmotes, .desc = "NPC Emotes"},
	Reload{.command = "npc_spells", .opcode = ServerOP_ReloadNPCSpells, .desc = "NPC Spells"},
	Reload{.command = "objects", .opcode = ServerOP_ReloadObjects, .desc = "Objects"},
	Reload{.command = "opcodes", .opcode = ServerOP_ReloadOpcodes, .desc = "Opcodes"},
	Reload{.command = "perl_export", .opcode = ServerOP_ReloadPerlExportSettings, .desc = "Perl Event Export Settings"},
	Reload{.command = "rules", .opcode = ServerOP_ReloadRules, .desc = "Rules"},
	Reload{.command = "skill_caps", .opcode = ServerOP_ReloadSkillCaps, .desc = "Skill Caps"},
	Reload{.command = "static", .opcode = ServerOP_ReloadStaticZoneData, .desc = "Static Zone Data"},
	Reload{.command = "tasks", .opcode = ServerOP_ReloadTasks, .desc = "Tasks"},
	Reload{.command = "titles", .opcode = ServerOP_ReloadTitles, .desc = "Titles"},
	Reload{.command = "traps", .opcode = ServerOP_ReloadTraps, .desc = "Traps"},
	Reload{.command = "variables", .opcode = ServerOP_ReloadVariables, .desc = "Variables"},
	Reload{.command = "veteran_rewards", .opcode = ServerOP_ReloadVeteranRewards, .desc = "Veteran Rewards"},
	Reload{.command = "world", .opcode = ServerOP_ReloadWorld, .desc = "World"},
	Reload{.command = "zone_points", .opcode = ServerOP_ReloadZonePoints, .desc = "Zone Points"},
};

void getReloadTypes(Json::Value &response)
{
	for (auto &c: reload_types) {
		Json::Value v;

		v["command"]     = c.command;
		v["opcode"]      = c.opcode;
		v["description"] = c.desc;
		response.append(v);
	}
}


void EQEmuApiWorldDataService::reload(Json::Value &r, const std::vector<std::string> &args)
{
	std::vector<std::string> commands{};
	commands.reserve(reload_types.size());
	for (auto &c: reload_types) {
		commands.emplace_back(c.command);
	}

	std::string command = !args[1].empty() ? args[1] : "";
	if (command.empty()) {
		message(r, fmt::format("Need to provide a type to reload. Example(s) [{}]", Strings::Implode("|", commands)));
		return;
	}

	ServerPacket *pack = nullptr;

	bool      found_command = false;
	for (auto &c: reload_types) {
		if (command == c.command) {
			if (c.command == "world") {
				uint8 global_repop = ReloadWorld::NoRepop;

				if (Strings::IsNumber(args[2])) {
					global_repop = static_cast<uint8>(Strings::ToUnsignedInt(args[2]));

					if (global_repop > ReloadWorld::ForceRepop) {
						global_repop = ReloadWorld::ForceRepop;
					}
				}

				message(
					r,
					fmt::format(
						"Attempting to reload Quests {}worldwide.",
						(
							global_repop ?
								(
									global_repop == ReloadWorld::Repop ?
										"and repop NPCs " :
										"and forcefully repop NPCs "
								) :
								""
						)
					)
				);

				pack = new ServerPacket(ServerOP_ReloadWorld, sizeof(ReloadWorld_Struct));
				auto RW = (ReloadWorld_Struct *) pack->pBuffer;
				RW->global_repop = global_repop;
			}
			else {
				pack = new ServerPacket(c.opcode, 0);
				message(r, fmt::format("Reloading [{}] globally", c.desc));

				if (c.opcode == ServerOP_ReloadLogs) {
					LogSys.LoadLogDatabaseSettings();
					QSLink.SendPacket(pack);
					UCSLink.SendPacket(pack);
				}
				else if (c.opcode == ServerOP_ReloadRules) {
					RuleManager::Instance()->LoadRules(&database, RuleManager::Instance()->GetActiveRuleset(), true);
				}
			}

			found_command = true;
		}
	}

	if (!found_command) {
		message(r, fmt::format("Need to provide a type to reload. Example(s) [{}]", Strings::Implode("|", commands)));
		return;
	}

	if (pack) {
		zoneserver_list.SendPacket(pack);
	}

	safe_delete(pack);
}

void EQEmuApiWorldDataService::message(Json::Value &r, const std::string &message)
{
	r["message"] = message;
}

void EQEmuApiWorldDataService::get(Json::Value &r, const std::vector<std::string> &args)
{
	const std::string &m = args[0];
	if (m == "get_zone_list") {
		callGetZoneList(r);
	}
	if (m == "get_database_schema") {
		callGetDatabaseSchema(r);
	}
	if (m == "get_client_list") {
		callGetClientList(r);
	}
	if (m == "get_reload_types") {
		getReloadTypes(r);
	}
	if (m == "reload") {
		reload(r, args);
	}
	if (m == "get_guild_details") {
		callGetGuildDetails(r, args);
	}
	if (m == "lock_status") {
		r["locked"] = WorldConfig::get()->Locked;
	}
}

void EQEmuApiWorldDataService::callGetGuildDetails(Json::Value &response, const std::vector<std::string> &args)
{

	std::string command = !args[1].empty() ? args[1] : "";
	if (command.empty()) {
		return;
	}
	Json::Value row;

	auto guild_id = Strings::ToUnsignedInt(command);
	if (!guild_id) {
		row = "useage is: api get_guild_details ### where ### is a valid guild id.";
		return;
	}
	auto guild = guild_mgr.GetGuildByGuildID(guild_id);
	if (!guild) {
		row = fmt::format("Could not find guild id {}", guild_id);
		return;
	}

	row["guild_id"]    = command;
	row["guild_name"]  = guild->name;
	row["leader_id"]   = guild->leader;
	row["min_status"]  = guild->minstatus;
	row["motd"]        = guild->motd;
	row["motd_setter"] = guild->motd_setter;
	row["url"]         = guild->url;
	row["channel"]     = guild->channel;

	for (int i = GUILD_LEADER; i <= GUILD_RECRUIT; i++) {
		row["Ranks"][i] = guild->rank_names[i].c_str();
	}

	for (int i = 1; i <= GUILD_MAX_FUNCTIONS; i++) {
		row["functions"][i]["db_id"]      = guild->functions[i].id;
		row["functions"][i]["perm_id"]    = guild->functions[i].perm_id;
		row["functions"][i]["guild_id"]   = guild->functions[i].guild_id;
		row["functions"][i]["perm_value"] = guild->functions[i].perm_value;
	}

	row["tribute"]["favor"]          = guild->tribute.favor;
	row["tribute"]["id1"]            = guild->tribute.id_1;
	row["tribute"]["id1_tier"]       = guild->tribute.id_1_tier;
	row["tribute"]["id2"]            = guild->tribute.id_2;
	row["tribute"]["id2_tier"]       = guild->tribute.id_2_tier;
	row["tribute"]["time_remaining"] = guild->tribute.time_remaining;
	row["tribute"]["enabled"]        = guild->tribute.enabled;

	client_list.GetGuildClientList(response, guild_id);

	response.append(row);
}
