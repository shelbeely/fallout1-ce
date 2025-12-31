#include "game/ai_control_api.h"

#include <stdio.h>
#include <string.h>

#include "game/actions.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/item.h"
#include "game/map.h"
#include "game/object.h"
#include "game/protinst.h"
#include "game/stat.h"
#include "game/tile.h"

namespace fallout {

// Configuration
static bool gAiControlApiEnabled = false;
static const char* kActionFilePath = "ai_action.json";
static const char* kStateFilePath = "ai_state.json";

// Simple JSON writer helpers
class JsonWriter {
public:
    JsonWriter() : buffer_pos(0), first_item(true) {}
    
    void startObject() {
        append("{");
        first_item = true;
    }
    
    void endObject() {
        append("}");
        first_item = false;
    }
    
    void startArray(const char* name) {
        if (!first_item) append(",");
        append("\"");
        append(name);
        append("\":[");
        first_item = true;
    }
    
    void endArray() {
        append("]");
        first_item = false;
    }
    
    void addString(const char* name, const char* value) {
        if (!first_item) append(",");
        append("\"");
        append(name);
        append("\":\"");
        if (value) append(value);
        append("\"");
        first_item = false;
    }
    
    void addInt(const char* name, int value) {
        if (!first_item) append(",");
        char temp[64];
        snprintf(temp, sizeof(temp), "\"%s\":%d", name, value);
        append(temp);
        first_item = false;
    }
    
    void addBool(const char* name, bool value) {
        if (!first_item) append(",");
        append("\"");
        append(name);
        append(value ? "\":true" : "\":false");
        first_item = false;
    }
    
    void addObjectInArray() {
        if (!first_item) append(",");
        append("{");
        first_item = true;
    }
    
    void endObjectInArray() {
        append("}");
        first_item = false;
    }
    
    const char* c_str() const { return buffer; }
    size_t length() const { return buffer_pos; }
    
private:
    static const size_t BUFFER_SIZE = 65536; // 64KB buffer
    char buffer[BUFFER_SIZE];
    size_t buffer_pos;
    bool first_item;
    
    void append(const char* str) {
        size_t len = strlen(str);
        if (buffer_pos + len < BUFFER_SIZE - 1) {
            strcpy(buffer + buffer_pos, str);
            buffer_pos += len;
        }
    }
};

// Write current game state to JSON file
static void writeGameState() {
    if (!obj_dude) {
        return;
    }
    
    JsonWriter json;
    json.startObject();
    
    // Player position
    json.addInt("player_tile", obj_dude->tile);
    json.addInt("player_elevation", obj_dude->elevation);
    json.addInt("player_rotation", obj_dude->rotation);
    
    // Player stats
    json.addInt("hit_points", critter_get_hits(obj_dude));
    json.addInt("max_hit_points", stat_level(obj_dude, STAT_MAXIMUM_HIT_POINTS));
    json.addInt("action_points", obj_dude->data.critter.combat.ap);
    json.addInt("level", stat_pc_get(PC_STAT_LEVEL));
    json.addInt("experience", stat_pc_get(PC_STAT_EXPERIENCE));
    
    // Combat state
    json.addBool("in_combat", isInCombat());
    
    // Primary stats
    json.addInt("strength", stat_level(obj_dude, STAT_STRENGTH));
    json.addInt("perception", stat_level(obj_dude, STAT_PERCEPTION));
    json.addInt("endurance", stat_level(obj_dude, STAT_ENDURANCE));
    json.addInt("charisma", stat_level(obj_dude, STAT_CHARISMA));
    json.addInt("intelligence", stat_level(obj_dude, STAT_INTELLIGENCE));
    json.addInt("agility", stat_level(obj_dude, STAT_AGILITY));
    json.addInt("luck", stat_level(obj_dude, STAT_LUCK));
    
    // Map info
    int mapIndex = map_get_index_number();
    if (mapIndex != -1) {
        char* mapName = map_get_short_name(mapIndex);
        if (mapName) {
            json.addString("map_name", mapName);
        }
    }
    
    // Nearby objects (within reasonable range)
    json.startArray("nearby_objects");
    
    Object* obj = obj_find_first_at(obj_dude->elevation);
    int count = 0;
    while (obj != NULL && count < 100) { // Limit to 100 objects
        if (obj != obj_dude) {
            int distance = obj_dist(obj_dude, obj);
            
            if (distance <= 10) { // Only nearby objects
                json.addObjectInArray();
                json.addInt("tile", obj->tile);
                json.addInt("distance", distance);
                json.addInt("type", FID_TYPE(obj->fid));
                json.addInt("pid", obj->pid);
                
                char* objName = object_name(obj);
                if (objName) {
                    json.addString("name", objName);
                }
                
                // Check if it's a critter
                if (FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER) {
                    json.addBool("is_dead", critter_is_dead(obj));
                    if (!critter_is_dead(obj)) {
                        json.addInt("hp", critter_get_hits(obj));
                    }
                }
                
                json.endObjectInArray();
                count++;
            }
        }
        obj = obj_find_next_at();
    }
    json.endArray();
    
    // Inventory (first 20 items)
    json.startArray("inventory");
    Inventory* inventory = &(obj_dude->data.critter.inventory);
    int invCount = 0;
    for (int i = 0; i < inventory->length && invCount < 20; i++) {
        InventoryItem* item = &(inventory->items[i]);
        if (item->item) {
            json.addObjectInArray();
            json.addInt("pid", item->item->pid);
            json.addInt("quantity", item->quantity);
            
            char* itemName = object_name(item->item);
            if (itemName) {
                json.addString("name", itemName);
            }
            
            json.endObjectInArray();
            invCount++;
        }
    }
    json.endArray();
    
    json.endObject();
    
    // Write to temporary file first, then rename (atomic operation)
    char tempPath[256];
    snprintf(tempPath, sizeof(tempPath), "%s.tmp", kStateFilePath);
    
    FILE* fp = fopen(tempPath, "w");
    if (fp) {
        fwrite(json.c_str(), 1, json.length(), fp);
        fclose(fp);
        
        // Atomic rename
        rename(tempPath, kStateFilePath);
    }
}

// Simple JSON parser for reading action
static bool readAction(char* actionType, int* targetTile, int* targetPid) {
    FILE* fp = fopen(kActionFilePath, "r");
    if (!fp) {
        return false;
    }
    
    char buffer[4096];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);
    
    if (bytesRead == 0) {
        return false;
    }
    buffer[bytesRead] = '\0';
    
    // Simple parsing - look for "action":"value"
    char* actionStart = strstr(buffer, "\"action\"");
    if (actionStart) {
        char* valueStart = strchr(actionStart, ':');
        if (valueStart) {
            valueStart++; // skip :
            while (*valueStart == ' ' || *valueStart == '"') valueStart++;
            
            char* valueEnd = valueStart;
            while (*valueEnd && *valueEnd != '"' && *valueEnd != ',' && *valueEnd != '}') {
                valueEnd++;
            }
            
            size_t len = valueEnd - valueStart;
            if (len > 0 && len < 64) {
                strncpy(actionType, valueStart, len);
                actionType[len] = '\0';
            }
        }
    }
    
    // Look for "target_tile":value
    char* tileStart = strstr(buffer, "\"target_tile\"");
    if (tileStart && targetTile) {
        char* valueStart = strchr(tileStart, ':');
        if (valueStart) {
            *targetTile = atoi(valueStart + 1);
        }
    }
    
    // Look for "target_pid":value
    char* pidStart = strstr(buffer, "\"target_pid\"");
    if (pidStart && targetPid) {
        char* valueStart = strchr(pidStart, ':');
        if (valueStart) {
            *targetPid = atoi(valueStart + 1);
        }
    }
    
    // Delete the action file after reading
    remove(kActionFilePath);
    
    return actionType[0] != '\0';
}

// Execute the action parsed from JSON
static bool executeAction(const char* actionType, int targetTile, int targetPid) {
    if (!obj_dude || !actionType) {
        return false;
    }
    
    // Move to tile
    if (strcmp(actionType, "move") == 0) {
        if (targetTile >= 0 && targetTile < 40000) {
            // Use pathfinding to move
            if (!isInCombat()) {
                // In non-combat mode, directly move
                obj_attempt_placement(obj_dude, targetTile, obj_dude->elevation, 0);
            } else {
                // In combat, use action points
                int distance = tile_dist(obj_dude->tile, targetTile);
                int apCost = item_mp_cost(obj_dude, HIT_MODE_PUNCH, false);
                if (obj_dude->data.critter.combat.ap >= apCost) {
                    obj_attempt_placement(obj_dude, targetTile, obj_dude->elevation, 0);
                    obj_dude->data.critter.combat.ap -= apCost;
                }
            }
        }
        return true;
    }
    
    // Wait/skip turn
    if (strcmp(actionType, "wait") == 0) {
        if (isInCombat()) {
            combat_turn_run();
        }
        return true;
    }
    
    // Use item
    if (strcmp(actionType, "use_item") == 0) {
        // Find item in inventory by PID
        Inventory* inventory = &(obj_dude->data.critter.inventory);
        for (int i = 0; i < inventory->length; i++) {
            InventoryItem* invItem = &(inventory->items[i]);
            if (invItem->item && invItem->item->pid == targetPid) {
                // Use the item
                obj_use_item(obj_dude, invItem->item);
                return true;
            }
        }
    }
    
    // Pickup item
    if (strcmp(actionType, "pickup") == 0) {
        Object* obj = obj_find_first_at(obj_dude->elevation);
        while (obj != NULL) {
            if (obj->tile == targetTile && obj->pid == targetPid) {
                if (FID_TYPE(obj->fid) == OBJ_TYPE_ITEM) {
                    obj_pickup(obj_dude, obj);
                    return true;
                }
            }
            obj = obj_find_next_at();
        }
    }
    
    // Attack target
    if (strcmp(actionType, "attack") == 0) {
        Object* target = NULL;
        Object* obj = obj_find_first_at(obj_dude->elevation);
        while (obj != NULL) {
            if (obj->tile == targetTile && FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER) {
                target = obj;
                break;
            }
            obj = obj_find_next_at();
        }
        
        if (target) {
            combat_attack(obj_dude, target, HIT_MODE_LEFT_WEAPON_PRIMARY, HIT_LOCATION_TORSO);
            return true;
        }
    }
    
    return false;
}

// Initialize AI control API
void ai_control_api_init() {
    int value = 0;
    if (config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, "ai_control_api", &value)) {
        gAiControlApiEnabled = (value != 0);
    }
}

// Cleanup AI control API
void ai_control_api_exit() {
    gAiControlApiEnabled = false;
    
    // Clean up any remaining files
    remove(kActionFilePath);
    remove(kStateFilePath);
}

// Check if API is enabled
bool ai_control_api_enabled() {
    return gAiControlApiEnabled;
}

// Process one AI action and write state
bool ai_control_api_process() {
    if (!gAiControlApiEnabled || !obj_dude) {
        return false;
    }
    
    // Always write current state
    writeGameState();
    
    // Check for action file
    char actionType[64] = {0};
    int targetTile = -1;
    int targetPid = -1;
    
    if (readAction(actionType, &targetTile, &targetPid)) {
        return executeAction(actionType, targetTile, targetPid);
    }
    
    return false;
}

} // namespace fallout
