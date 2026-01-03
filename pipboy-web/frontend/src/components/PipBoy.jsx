import React, { useState, useEffect } from 'react'
import StatusTab from './StatusTab'
import StatsTab from './StatsTab'
import InventoryTab from './InventoryTab'
import DataTab from './DataTab'
import io from 'socket.io-client'

function PipBoy() {
  const [activeTab, setActiveTab] = useState('status')
  const [gameData, setGameData] = useState(null)
  const [socket, setSocket] = useState(null)

  useEffect(() => {
    // Connect to WebSocket for real-time updates
    const newSocket = io('http://localhost:5001')
    
    newSocket.on('connect', () => {
      console.log('Connected to Pip-Boy server')
      newSocket.emit('subscribe_updates')
    })

    newSocket.on('game_state_update', (data) => {
      setGameData(data)
    })

    setSocket(newSocket)

    return () => {
      newSocket.close()
    }
  }, [])

  // Also fetch initial data via REST API
  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await fetch('/api/status')
        if (response.ok) {
          const data = await response.json()
          setGameData(data)
        }
      } catch (error) {
        console.error('Error fetching game data:', error)
      }
    }

    fetchData()
  }, [])

  return (
    <div className="pipboy">
      <div className="pipboy-screen">
        <div className="pipboy-header">
          <h1>PIP-BOY 2000</h1>
          <div className="pipboy-tabs">
            <button 
              className={activeTab === 'status' ? 'active' : ''} 
              onClick={() => setActiveTab('status')}
            >
              STAT
            </button>
            <button 
              className={activeTab === 'stats' ? 'active' : ''} 
              onClick={() => setActiveTab('stats')}
            >
              SPECIAL
            </button>
            <button 
              className={activeTab === 'inventory' ? 'active' : ''} 
              onClick={() => setActiveTab('inventory')}
            >
              INV
            </button>
            <button 
              className={activeTab === 'data' ? 'active' : ''} 
              onClick={() => setActiveTab('data')}
            >
              DATA
            </button>
          </div>
        </div>

        <div className="pipboy-content">
          {activeTab === 'status' && <StatusTab gameData={gameData} />}
          {activeTab === 'stats' && <StatsTab />}
          {activeTab === 'inventory' && <InventoryTab />}
          {activeTab === 'data' && <DataTab />}
        </div>
      </div>
    </div>
  )
}

export default PipBoy
