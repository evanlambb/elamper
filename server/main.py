import json
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from typing import Dict, List

app = FastAPI()

DEFAULT_STATE = {"power": False, "color": "red"}


@app.get("/health")
async def health_check():
    return {"status": "alive", "message": "Server is awake"}


class ConnectionManager:
    def __init__(self):
        self.rooms: Dict[str, List[WebSocket]] = {}
        self.state: Dict[str, dict] = {}

    async def connect(self, websocket: WebSocket, room_id: str):
        await websocket.accept()
        if room_id not in self.rooms:
            self.rooms[room_id] = []
        self.rooms[room_id].append(websocket)

        current = self.state.get(room_id, DEFAULT_STATE.copy())
        await websocket.send_text(json.dumps(current))

    def disconnect(self, websocket: WebSocket, room_id: str):
        if room_id in self.rooms and websocket in self.rooms[room_id]:
            self.rooms[room_id].remove(websocket)
            if not self.rooms[room_id]:
                del self.rooms[room_id]

    async def handle_message(self, message: str, sender: WebSocket, room_id: str):
        try:
            data = json.loads(message)
        except json.JSONDecodeError:
            return

        if "power" not in data or "color" not in data:
            return

        self.state[room_id] = {"power": data["power"], "color": data["color"]}

        if room_id in self.rooms:
            for connection in self.rooms[room_id]:
                if connection != sender:
                    await connection.send_text(json.dumps(self.state[room_id]))


manager = ConnectionManager()


@app.websocket("/ws/lamps/{room_id}")
async def websocket_endpoint(websocket: WebSocket, room_id: str):
    await manager.connect(websocket, room_id)
    try:
        while True:
            data = await websocket.receive_text()
            await manager.handle_message(data, sender=websocket, room_id=room_id)
    except WebSocketDisconnect:
        manager.disconnect(websocket, room_id)
