#!/bin/bash

echo "Dropbox Lite Demo"
echo "================="
echo ""
echo "This will start:"
echo "  1. Server on port 50051"
echo "  2. Two clients syncing to ./demo_client1 and ./demo_client2"
echo ""
echo "Try creating/editing files in either folder to see sync in action!"
echo ""

# Cleanup previous demo
rm -rf demo_storage demo_client1 demo_client2

# Create directories
mkdir -p demo_storage demo_client1 demo_client2

# Start server in background
echo "Starting server..."
./build/dropbox_server_app demo_storage 50051 &
SERVER_PID=$!
sleep 2

# Start clients in background
echo "Starting client 1..."
./build/dropbox_client_app demo_client1 localhost:50051 client1 &
CLIENT1_PID=$!
sleep 1

echo "Starting client 2..."
./build/dropbox_client_app demo_client2 localhost:50051 client2 &
CLIENT2_PID=$!

echo ""
echo "Demo running!"
echo "  Server PID: $SERVER_PID"
echo "  Client 1 PID: $CLIENT1_PID"
echo "  Client 2 PID: $CLIENT2_PID"
echo ""
echo "Press Ctrl+C to stop all processes"

# Cleanup on exit
trap "kill $SERVER_PID $CLIENT1_PID $CLIENT2_PID 2>/dev/null; exit" INT TERM

# Wait
wait
