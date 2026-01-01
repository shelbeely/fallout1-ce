# Website Backend README

## Data Collector + API Server + Extended Data Generator

The website backend consists of three components:

1. **Data Collector** (`data_collector.py`) - Background service that polls game JSON files
2. **API Server** (`api_server.py`) - REST API that serves data to frontend
3. **Extended Data Generator** (`character_data_generator.py`) - Generates comprehensive character data for terminal UI

## Installation

```bash
pip install -r requirements.txt
```

## Running

### Start Data Collector (background)
```bash
python data_collector.py --game-dir ../.. --interval 1.0
```

### Start API Server
```bash
python api_server.py --port 5000
```

## API Endpoints

### Base Endpoints (from database)
- `GET /api/current-state` - Current game state with inventory
- `GET /api/stats-history?hours=1` - HP/XP history over time
- `GET /api/skills-current` - Current skill levels
- `GET /api/milestones` - All milestones achieved
- `GET /api/items-collected?limit=100` - Items collected history
- `GET /api/decisions?limit=50` - AI decision history
- `GET /api/session-stats` - Aggregated session statistics
- `GET /api/location-history` - Visited locations
- `GET /api/combat-stats` - Combat statistics
- `GET /api/health` - Health check

### Extended Endpoints (for terminal UI)
- `GET /api/character-extended` - Complete extended character data (all fields below)
- `GET /api/timeline` - Chronological event timeline
- `GET /api/quests` - Quest log with status and outcomes
- `GET /api/locations-extended` - Detailed location dossiers and map data
- `GET /api/journal` - In-character journal entries

### Extended Data Features

The extended endpoints support the terminal UI with:
- **Character Identity**: Name, age, pronouns, origin, background
- **Visual Assets**: Portrait and sprite URLs
- **Interactive Map**: Locations with coordinates, visit status, travel routes
- **Timeline**: Chronological "Previously On..." feed (QUICK/FULL/ARC modes)
- **Quest Log**: Active/completed/failed quests with outcomes
- **Location Archive**: Detailed dossiers with events, NPCs, consequences
- **Journal**: In-character diary entries
- **Faction Relations**: Karma and reputation tracking
- **Stream Highlights**: Quick recap bullets for overlays

See [EXTENDED_SCHEMA.md](./EXTENDED_SCHEMA.md) for complete JSON schema documentation.

## Generating Extended Data

The extended data generator creates comprehensive character information for the terminal UI:

```bash
python character_data_generator.py --game-dir ../.. --output ../../character_extended.json
```

This reads base game data (`ai_state.json`, `ai_memory.json`, etc.) and generates:
- Timeline from memory events
- Quest log from actions
- Location tracking from visited maps
- Journal entries based on progression

The API server automatically generates this data when requested, with 10-second caching.

## Database

SQLite database stored at `./database/game_data.db`

Tables:
- `game_states` - Game state snapshots
- `inventory_snapshots` - Inventory over time
- `events` - Game events
- `milestones` - Achievements
- `decisions` - AI decisions
- `skills` - Skill progression
- `stats` - SPECIAL stats
- `session_stats` - Session statistics
- `items_collected` - Item collection history
