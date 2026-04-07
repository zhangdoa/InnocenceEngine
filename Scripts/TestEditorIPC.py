import asyncio
import websockets
import json
import subprocess
import time
import os
import sys
import threading

def read_stream(stream, prefix):
    for line in iter(stream.readline, b''):
        print(f"{prefix}: {line.decode().strip()}")

async def run_test():
    # 1. Launch Engine in sidecar mode
    engine_path = os.path.abspath("Bin/RelWithDebInfo/Main.exe")
    if not os.path.exists(engine_path):
        print(f"Error: Engine not found at {engine_path}")
        return False

    print(f"Launching engine: {engine_path}")
    # Use actual production parameters
    process = subprocess.Popen([engine_path, "-mode", "2", "-renderer", "0", "-loglevel", "0"], 
                               cwd=os.path.abspath("Bin"),
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE)

    # Start threads to read output in real-time
    stdout_thread = threading.Thread(target=read_stream, args=(process.stdout, "ENGINE"))
    stderr_thread = threading.Thread(target=read_stream, args=(process.stderr, "ENGINE_ERR"))
    stdout_thread.daemon = True
    stderr_thread.daemon = True
    stdout_thread.start()
    stderr_thread.start()

    print("Waiting for engine to start...")
    
    uri = "ws://127.0.0.1:8081"
    success = False
    websocket = None
    
    # Retry connection for up to 60 seconds (Main.exe takes longer to load scenes)
    for i in range(60):
        try:
            websocket = await websockets.connect(uri)
            print(f"Connected to {uri} after {i} retries")
            break
        except Exception:
            await asyncio.sleep(1)
    
    if not websocket:
        print(f"Error: Could not connect to {uri} after 60 seconds")
        return False

    try:
        async with websocket:
            # Test HELO
            print("Sending HELO...")
            await websocket.send(json.dumps({"type": "HELO"}))
            
            # Use a loop to wait for the correct response if multiple are coming in
            while True:
                response = await websocket.recv()
                data = json.loads(response)
                print(f"Received: {data['type']}")
                
                if data["type"] == "HELLO_REPLY":
                    print("[PASS] Handshake received")
                    if data.get("sharedHandle") != 0:
                        print(f"[PASS] Valid handle in handshake: {data['sharedHandle']}")
                        break
                    else:
                        print("Handshake has 0 handle, waiting for VIEWPORT_READY...")
                
                if data["type"] == "VIEWPORT_READY":
                    if data["sharedHandle"] != 0:
                        print(f"[PASS] VIEWPORT_READY received with valid handle: {data['sharedHandle']}")
                        break
                    else:
                        print("[FAIL] VIEWPORT_READY has 0 handle")
                        return False

            # Test GET_SCENE
            print("Sending GET_SCENE...")
            await websocket.send(json.dumps({"type": "GET_SCENE"}))
            response = await websocket.recv()
            data = json.loads(response)

            if data["type"] == "SCENE_DATA" and len(data["entities"]) > 0:
                print(f"[PASS] GET_SCENE test passed, found {len(data['entities'])} entities")
                entity_id = data["entities"][0]["id"]
            else:
                print("[FAIL] GET_SCENE test failed")
                return False

            # Test GET_ENTITY_DETAILS
            print(f"Sending GET_ENTITY_DETAILS for ID {entity_id}...")
            await websocket.send(json.dumps({"type": "GET_ENTITY_DETAILS", "id": entity_id}))
            response = await websocket.recv()
            data = json.loads(response)

            if data["type"] == "ENTITY_DETAILS" and "details" in data:
                print("[PASS] GET_ENTITY_DETAILS test passed")
            else:
                print("[FAIL] GET_ENTITY_DETAILS test failed")
                return False

            success = True

    except Exception as e:
        print(f"Error during test: {e}")
    finally:
        print("Terminating engine...")
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()

    return success

if __name__ == "__main__":
    result = asyncio.run(run_test())
    sys.exit(0 if result else 1)
