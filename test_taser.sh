#!/bin/bash
set -e

echo "Starting game..."
# Run the game in the background. The parent script will use xvfb-run.
./build/StealthActionGame > game_output.log 2>&1 &
GAME_PID=$!
echo "Game running with PID $GAME_PID"

# Wait for the game to initialize and get the window
sleep 3
WINDOW_ID=$(xdotool search --name "Stealth Action" | head -n1)
if [ -z "$WINDOW_ID" ]; then
    echo "Error: Could not find game window!"
    kill $GAME_PID
    exit 1
fi
echo "Found window ID: $WINDOW_ID"

# Focus the window and send commands
echo "Selecting taser (Numpad 3)..."
xdotool windowfocus $WINDOW_ID
xdotool key --window $WINDOW_ID "KP_3"
sleep 0.5

echo "Firing taser (Space)..."
xdotool key --window $WINDOW_ID "space"
sleep 1

echo "Test complete. Killing game."
kill $GAME_PID
# Wait for the process to be fully terminated
wait $GAME_PID || true

echo "Test finished."
