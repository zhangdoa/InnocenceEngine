import asyncio
import websockets
import json
import subprocess
import time
import os
import sys
import threading
from datetime import datetime

# Configuration
WS_URI = "ws://127.0.0.1:8081"
ENGINE_EXE = "Bin/RelWithDebInfo/Main.exe"
CONNECT_TIMEOUT = 60
OP_TIMEOUT = 10

def log(msg, level="INFO"):
    timestamp = datetime.now().strftime("%H:%M:%S")
    print(f"[{timestamp}][{level}] {msg}")

def read_stream(stream, prefix):
    try:
        for line in iter(stream.readline, b''):
            decoded = line.decode().strip()
            if decoded:
                log(f"{prefix}: {decoded}", level="ENGINE")
    except Exception as e:
        log(f"Stream reader error: {e}", level="ERROR")

class EditorTestClient:
    def __init__(self):
        self.websocket = None
        self.process = None
        self.shared_handle = None
        self.entities = []

    async def connect(self):
        log(f"Connecting to {WS_URI}...")
        start_time = time.time()
        while time.time() - start_time < CONNECT_TIMEOUT:
            try:
                self.websocket = await websockets.connect(WS_URI)
                log("Connected to Engine WebSocket")
                return True
            except Exception:
                await asyncio.sleep(1)
        log("Failed to connect to Engine WebSocket within timeout", level="ERROR")
        return False

    async def send_and_wait(self, msg_type, payload=None, expected_reply=None):
        if not self.websocket:
            return None
        
        msg = {"type": msg_type}
        if payload:
            msg.update(payload)
        
        log(f"Sending: {msg_type}")
        await self.websocket.send(json.dumps(msg))
        
        if not expected_reply:
            return True

        try:
            while True:
                response = await asyncio.wait_for(self.websocket.recv(), timeout=OP_TIMEOUT)
                data = json.loads(response)
                log(f"Received: {data['type']}")
                
                # Check for VIEWPORT_READY (can be async or the expected reply)
                if data["type"] == "VIEWPORT_READY":
                    self.shared_handle = data.get("sharedHandle")
                    log(f"Viewport Ready! Handle: {self.shared_handle} (type: {type(self.shared_handle)})")
                    if expected_reply == "VIEWPORT_READY":
                        return data
                
                # Check for the specific reply we want
                if data["type"] == expected_reply:
                    return data
                
        except asyncio.TimeoutError:
            log(f"Timeout waiting for {expected_reply}", level="ERROR")
            return None

    def validate_handle(self, handle):
        if handle is None: return False
        if isinstance(handle, int):
            return handle != 0
        if isinstance(handle, str):
            try:
                # Handle hex string if it comes as "0x..." or just "ABC..."
                val = int(handle, 16)
                return val != 0
            except ValueError:
                return False
        return False

    async def run_full_suite(self):
        # 1. HELO Handshake
        reply = await self.send_and_wait("HELO", expected_reply="HELLO_REPLY")
        if not reply: return False
        log("[PASS] Handshake successful")
        
        # 2. Wait for Viewport (if not already received)
        if not self.validate_handle(self.shared_handle):
            log("Waiting for VIEWPORT_READY...")
            # If we didn't get it yet, wait for it by waiting on another message
            reply = await self.send_and_wait("GET_SCENE", expected_reply="VIEWPORT_READY") 
            if not self.validate_handle(self.shared_handle):
                log(f"[FAIL] Viewport handle is invalid: {self.shared_handle}", level="ERROR")
                return False
        log(f"[PASS] Viewport shared handle verified: {self.shared_handle}")

        # 3. GET_SCENE (we might have already received it if we used it to wait for VIEWPORT_READY)
        # But let's request it explicitly to be sure we have the latest.
        reply = await self.send_and_wait("GET_SCENE", expected_reply="SCENE_DATA")
        if not reply or "entities" not in reply or len(reply["entities"]) == 0:
            log("[FAIL] Scene data empty or invalid", level="ERROR")
            return False
        self.entities = reply["entities"]
        log(f"[PASS] Scene data verified: {len(self.entities)} entities found")

        # 4. GET_ENTITY_DETAILS
        target_id = self.entities[0]["id"]
        reply = await self.send_and_wait("GET_ENTITY_DETAILS", {"id": target_id}, expected_reply="ENTITY_DETAILS")
        if not reply or "details" not in reply:
            log(f"[FAIL] Failed to get details for entity {target_id}", level="ERROR")
            return False
        log(f"[PASS] Entity details verified for '{reply['details']['name']}'")

        # 5. UPDATE_ENTITY_PROPERTY
        update_payload = {
            "id": target_id,
            "component": "TransformComponent",
            "property": "pos",
            "value": [1.0, 2.0, 3.0]
        }
        await self.send_and_wait("UPDATE_ENTITY_PROPERTY", update_payload)
        log("[PASS] Property update message sent (smoke test)")

        # 6. SAVE_SCENE
        await self.send_and_wait("SAVE_SCENE")
        log("[PASS] Save scene message sent")

        return True

async def main():
    engine_path = os.path.abspath(ENGINE_EXE)
    if not os.path.exists(engine_path):
        log(f"Engine not found at {engine_path}", level="ERROR")
        sys.exit(1)

    client = EditorTestClient()
    
    log(f"Spawning Engine: {engine_path}")
    client.process = subprocess.Popen(
        [engine_path, "-mode", "2", "-renderer", "0", "-loglevel", "0", "-offscreen"], 
        cwd=os.path.abspath("Bin"),
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE
    )

    stdout_thread = threading.Thread(target=read_stream, args=(client.process.stdout, "STDOUT"))
    stderr_thread = threading.Thread(target=read_stream, args=(client.process.stderr, "STDERR"))
    stdout_thread.daemon = True
    stderr_thread.daemon = True
    stdout_thread.start()
    stderr_thread.start()

    success = False
    try:
        if await client.connect():
            success = await client.run_full_suite()
    except Exception as e:
        log(f"Test runner exception: {e}", level="ERROR")
    finally:
        log("Shutting down...")
        if client.process:
            client.process.terminate()
            try:
                client.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                client.process.kill()
        
    if success:
        log("=== ALL TESTS PASSED ===", level="SUCCESS")
        sys.exit(0)
    else:
        log("=== TESTS FAILED ===", level="ERROR")
        sys.exit(1)

if __name__ == "__main__":
    asyncio.run(main())
