# Website Backend README

## Data Collector + API Server

The website backend consists of two components:

1. **Data Collector** (`data_collector.py`) - Background service that polls game JSON files
2. **API Server** (`api_server.py`) - REST API that serves data to frontend

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
