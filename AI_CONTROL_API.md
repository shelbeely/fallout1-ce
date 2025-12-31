# AI Control API

## Overview

The AI Control API provides a JSON-based interface for external AI agents to interact with Fallout 1 Community Edition. It allows reading the game state and sending actions through simple JSON files.

## Configuration

To enable the API, add the following line to your `fallout.cfg` file under the `[preferences]` section:

```ini
ai_control_api=1
```

## How It Works

The API operates through two JSON files:

1. **`ai_state.json`** - Written by the game every frame when API is enabled
2. **`ai_action.json`** - Read by the game and deleted after processing

### Game State Format (`ai_state.json`)

The game writes the current state every frame:

```json
{
  "player_tile": 20100,
  "player_elevation": 0,
  "player_rotation": 2,
  "hit_points": 45,
  "max_hit_points": 45,
  "action_points": 7,
  "level": 1,
  "experience": 0,
  "in_combat": false,
  "strength": 5,
  "perception": 6,
  "endurance": 5,
  "charisma": 5,
  "intelligence": 6,
  "agility": 6,
  "luck": 5,
  "map_name": "V13Ent",
  "nearby_objects": [
    {
      "tile": 20105,
      "distance": 2,
      "type": 1,
      "pid": 16777216,
      "name": "Vault Dweller",
      "is_dead": false,
      "hp": 30
    }
  ],
  "inventory": [
    {
      "pid": 41,
      "quantity": 1,
      "name": "$"
    }
  ]
}
```

### Action Format (`ai_action.json`)

To perform an action, write a JSON file with the following format:

#### Move Action
```json
{
  "action": "move",
  "target_tile": 20150
}
```

#### Wait/Skip Turn
```json
{
  "action": "wait"
}
```

#### Use Item
```json
{
  "action": "use_item",
  "target_pid": 40
}
```

#### Pickup Item
```json
{
  "action": "pickup",
  "target_tile": 20105,
  "target_pid": 41
}
```

#### Attack Target
```json
{
  "action": "attack",
  "target_tile": 20105
}
```

## Implementation Details

- **Deterministic**: Actions are processed exactly once per frame
- **Low-overhead**: Minimal JSON parsing and writing
- **Safe**: Uses atomic file operations (write to temp, then rename)
- **Streaming-friendly**: Continuous state updates every frame

## Example Usage

Here's a simple Python example to interact with the API:

```python
import json
import time
import os

def read_state():
    if os.path.exists("ai_state.json"):
        with open("ai_state.json", "r") as f:
            return json.load(f)
    return None

def send_action(action, **kwargs):
    action_data = {"action": action}
    action_data.update(kwargs)
    with open("ai_action.json", "w") as f:
        json.dump(action_data, f)

# Main loop
while True:
    state = read_state()
    if state:
        print(f"Player at tile {state['player_tile']}, HP: {state['hit_points']}/{state['max_hit_points']}")
        
        # Example: Move to a nearby tile
        if not state["in_combat"]:
            send_action("move", target_tile=state["player_tile"] + 1)
    
    time.sleep(0.1)  # Check state 10 times per second
```

## Notes

- The API only works when a game is loaded (not in main menu)
- Actions are ignored if the game state doesn't support them (e.g., move during dialog)
- The state file is written using atomic operations to prevent corruption
- The action file is deleted after reading to prevent duplicate execution
