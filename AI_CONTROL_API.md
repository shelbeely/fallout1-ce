# AI Control API - Enhanced for Twitch Streaming

## Overview

The AI Control API provides a comprehensive JSON-based interface for external AI agents and Twitch chat integration with Fallout 1 Community Edition. It allows reading detailed game state, sending actions, tracking events, and accessing streaming-friendly statistics.

## Configuration

To enable the API, add the following line to your `fallout.cfg` file under the `[preferences]` section:

```ini
ai_control_api=1
```

## How It Works

The API operates through JSON files with enhanced features for streaming:

1. **`ai_state.json`** - Written by the game every frame with comprehensive state
2. **`ai_action.json`** - Read by the game and deleted after processing
3. **Action cooldown** - 100ms between actions to prevent spam
4. **Event tracking** - Recent events logged for stream overlays
5. **Action feedback** - Success/failure results for each action

## Enhanced Game State Format (`ai_state.json`)

The game writes extensive state information every frame:

```json
{
  "player_tile": 20100,
  "player_elevation": 0,
  "player_rotation": 2,
  "hit_points": 45,
  "max_hit_points": 45,
  "action_points": 7,
  "max_action_points": 10,
  "level": 3,
  "experience": 1250,
  "armor_class": 12,
  "sequence": 8,
  "carry_weight": 175,
  "melee_damage": 2,
  "in_combat": false,
  "strength": 5,
  "perception": 6,
  "endurance": 5,
  "charisma": 5,
  "intelligence": 6,
  "agility": 6,
  "luck": 5,
  "skills": [
    {"name": "Small Guns", "value": 45},
    {"name": "Big Guns", "value": 15},
    {"name": "Energy Weapons", "value": 20},
    {"name": "Unarmed", "value": 55},
    {"name": "Melee Weapons", "value": 40},
    {"name": "Throwing", "value": 30},
    {"name": "First Aid", "value": 35},
    {"name": "Doctor", "value": 25},
    {"name": "Sneak", "value": 40},
    {"name": "Lockpick", "value": 30},
    {"name": "Steal", "value": 25},
    {"name": "Traps", "value": 20},
    {"name": "Science", "value": 30},
    {"name": "Repair", "value": 25},
    {"name": "Speech", "value": 50},
    {"name": "Barter", "value": 35},
    {"name": "Gambling", "value": 20},
    {"name": "Outdoorsman", "value": 25}
  ],
  "perks": [
    {"name": "Bonus Move", "level": 1},
    {"name": "More Criticals", "level": 1}
  ],
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
      "quantity": 350,
      "name": "$"
    },
    {
      "pid": 40,
      "quantity": 3,
      "name": "Stimpak"
    }
  ],
  "total_damage_dealt": 145,
  "total_kills": 3,
  "session_time_seconds": 1234,
  "last_action_result": "success: moved to tile 20150",
  "recent_events": [
    {"event": "damage_taken: Took 5 damage (HP: 50->45)"},
    {"event": "pickup: picked up Stimpak"},
    {"event": "attack: attacked Rat"},
    {"event": "move: Player moved"}
  ]
}
```

## Action Format (`ai_action.json`)

Actions now include result feedback and cooldown protection:

### Move Action
```json
{
  "action": "move",
  "target_tile": 20150
}
```
**Results:** `success: moved to tile X` or `error: cannot move to tile X` or `error: not enough AP`

### Wait/Skip Turn
```json
{
  "action": "wait"
}
```
**Results:** `success: turn ended` or `success: waited`

### Use Item
```json
{
  "action": "use_item",
  "target_pid": 40
}
```
**Results:** `success: used [item name]` or `error: item X not found in inventory`

### Pickup Item
```json
{
  "action": "pickup",
  "target_tile": 20105,
  "target_pid": 41
}
```
**Results:** `success: picked up [item name]` or `error: cannot pickup item` or `error: item not found at tile X`

### Attack Target
```json
{
  "action": "attack",
  "target_tile": 20105
}
```
**Results:** `success: attacked [target name]` or `error: no target at tile X`

## Streaming Features

### Event Tracking
The API automatically tracks important events:
- **Damage taken** - Player health changes
- **Level ups** - Character progression
- **Combat actions** - Attacks, kills
- **Item interactions** - Pickups, usage
- **Movement** - Location changes

### Streaming Stats
Perfect for OBS overlays:
- `total_damage_dealt` - Cumulative damage counter
- `total_kills` - Enemy kill counter
- `session_time_seconds` - Time since API started
- `recent_events` - Last 50 events for scrolling display

### Action Cooldown
100ms cooldown between actions prevents spam and ensures stability for Twitch chat integration.

## Twitch Integration Examples

### Python Twitch Bot
```python
import json
import time
import os
from twitchio.ext import commands

class FalloutBot(commands.Bot):
    def __init__(self):
        super().__init__(token='YOUR_TOKEN', prefix='!', initial_channels=['YOUR_CHANNEL'])
        self.action_votes = {}
        self.vote_end_time = 0
    
    def read_state(self):
        if os.path.exists("ai_state.json"):
            with open("ai_state.json", "r") as f:
                return json.load(f)
        return None
    
    def send_action(self, action, **kwargs):
        action_data = {"action": action}
        action_data.update(kwargs)
        with open("ai_action.json", "w") as f:
            json.dump(action_data, f)
    
    @commands.command(name='vote')
    async def vote(self, ctx, action: str):
        """Chat voting: !vote move or !vote attack or !vote wait"""
        if time.time() < self.vote_end_time:
            if action in ['move', 'attack', 'wait', 'heal']:
                self.action_votes[action] = self.action_votes.get(action, 0) + 1
                await ctx.send(f"Vote recorded for {action}!")
    
    @commands.command(name='startvote')
    async def start_vote(self, ctx):
        """Start a 30-second vote"""
        self.action_votes = {}
        self.vote_end_time = time.time() + 30
        await ctx.send("Vote now! Use !vote move, !vote attack, !vote wait, or !vote heal (30 seconds)")
    
    @commands.command(name='endvote')
    async def end_vote(self, ctx):
        """End vote and execute winning action"""
        if not self.action_votes:
            await ctx.send("No votes recorded!")
            return
        
        winner = max(self.action_votes, key=self.action_votes.get)
        await ctx.send(f"Winning action: {winner} with {self.action_votes[winner]} votes!")
        
        state = self.read_state()
        if state:
            if winner == 'move' and state['nearby_objects']:
                # Move to nearest object
                target = state['nearby_objects'][0]
                self.send_action("move", target_tile=target['tile'])
            elif winner == 'attack' and state['nearby_objects']:
                # Attack nearest enemy
                for obj in state['nearby_objects']:
                    if not obj.get('is_dead', True):
                        self.send_action("attack", target_tile=obj['tile'])
                        break
            elif winner == 'heal':
                # Use stimpak (pid 40)
                self.send_action("use_item", target_pid=40)
            elif winner == 'wait':
                self.send_action("wait")
        
        self.action_votes = {}
        self.vote_end_time = 0

bot = FalloutBot()
bot.run()
```

### Stream Overlay (HTML/JavaScript)
```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body { 
            background: transparent; 
            font-family: 'Courier New', monospace;
            color: #00ff00;
        }
        .stats {
            background: rgba(0,0,0,0.8);
            padding: 10px;
            border: 2px solid #00ff00;
        }
    </style>
</head>
<body>
    <div class="stats" id="stats"></div>
    <script>
        async function updateStats() {
            const response = await fetch('ai_state.json');
            const state = await response.json();
            
            document.getElementById('stats').innerHTML = `
                <h2>FALLOUT 1 STATS</h2>
                <p>HP: ${state.hit_points}/${state.max_hit_points}</p>
                <p>Level: ${state.level} (XP: ${state.experience})</p>
                <p>Location: ${state.map_name}</p>
                <p>Session Time: ${Math.floor(state.session_time_seconds / 60)}m</p>
                <p>Kills: ${state.total_kills}</p>
                <p>Damage: ${state.total_damage_dealt}</p>
                <p>Last Action: ${state.last_action_result}</p>
                <h3>Recent Events:</h3>
                ${state.recent_events.map(e => `<p>• ${e.event}</p>`).join('')}
            `;
        }
        
        setInterval(updateStats, 1000); // Update every second
    </script>
</body>
</html>
```

## Implementation Details

- **Deterministic**: Actions are processed exactly once per frame
- **Low-overhead**: Minimal JSON parsing and writing
- **Safe**: Uses atomic file operations (write to temp, then rename)
- **Streaming-friendly**: Continuous state updates with event tracking
- **Cooldown protection**: 100ms minimum between actions
- **Action feedback**: Every action returns success/failure status
- **Event history**: Last 50 events tracked for overlays

## Best Practices for Twitch Streaming

1. **Use voting systems** - Let chat vote on actions every 30-60 seconds
2. **Display overlays** - Show stats, recent events, and action results
3. **Moderate cooldowns** - Don't spam actions too quickly
4. **Track achievements** - Use kill count, damage, and time for milestones
5. **Enable interaction** - Let viewers influence gameplay meaningfully

## Notes

- The API only works when a game is loaded (not in main menu)
- Actions are ignored if the game state doesn't support them
- State file is written using atomic operations to prevent corruption
- Action file is deleted after reading to prevent duplicate execution
- Event log wraps after 50 entries to maintain recent history
- Session stats reset when API is re-initialized

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
