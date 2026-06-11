#!/bin/bash
SERVER=./build/ini_server/ini_server
CLIENT=./build/ini_client/ini_client
PORT=12345
PASS=0
FAIL=0

RESET="\033[0m";
GREEN="\033[32m";
RED="\033[31m";
ok()   { printf "$GREEN OK----$RESET $1\n"; PASS=$((PASS+1)); }
fail() { printf   "$RED FAIL--$RESET $1\n"; FAIL=$((FAIL+1)); }

# ---------------------------------------------------------------------------
# TEST 1: server lifecycle
# ---------------------------------------------------------------------------
echo "=== TEST 1: Server Lifecycle ==="

# 1.i — launch the server
$SERVER >/dev/null &
SERVER_PID=$!
sleep 0.5

# 1.ii — verify server is up: check PID and port
if kill -0 "$SERVER_PID" 2>/dev/null; then
   ok "server process is alive (PID=$SERVER_PID)"
else
   fail "server process not found"
fi

if nc -z -w1 localhost $PORT 2>/dev/null; then
    ok "server is listening on port $PORT"
else
    fail "server is not listening on port $PORT"
fi

# 1.iii — stop server with SIGINT
kill -SIGINT $SERVER_PID
sleep 1

# 1.iv — verify server is no longer running
if kill -0 "$SERVER_PID" 2>/dev/null; then
    fail "server process still alive after SIGINT"
else
    ok "server process has terminated"
    sleep 0.5
fi

if nc -z -w1 localhost $PORT 2>/dev/null; then
    fail "port $PORT still bound after shutdown"
else
    ok "port $PORT is free after shutdown"
fi

# ---------------------------------------------------------------------------
# TEST 2: INI load / get / set roundtrip
# ---------------------------------------------------------------------------
echo ""
echo "=== TEST 2: INI Roundtrip ==="

# 2.i — write a test INI file to /tmp
INI_FILE=/tmp/example.ini
cat > "$INI_FILE" <<'INI'
host = localhost
port = 8080

[section]
key = value
[[color]]
red = ferrari

INI
ok "test INI file written to $INI_FILE"

# 2.ii — launch the server
$SERVER >/dev/null &
SERVER_PID=$!
sleep 0.5

# 2.iii launch the client and verigy the load the test INI file
if $CLIENT --load $INI_FILE 1>/tmp/client_out; then
    ok "client started successfully"
else
    fail "client not started"
fi

# 2.iv verify the load success
if grep -q "0" /tmp/client_out; then
    ok "server response 0: file loaded"
else
    fail "server response $(cat /tmp/client_out): file error"
fi

# 2.v — get an existing top-level key
$CLIENT --get host >/tmp/client_out;

# 2.vi — verify the correct output an existing top-level key
if grep -q "localhost" /tmp/client_out; then
    ok "GET host returned expected value 'localhost'"
else
    fail "GET host failed (got: '$(cat /tmp/client_out)')"
fi

# 2.vii GET a non existing value
$CLIENT --get nonexisting >/tmp/client_out;

# 2.viii check GET a non existing value output
if grep -q "3" /tmp/client_out; then
    ok "GET nonexisting returned error 3 (missing key)"
else
    fail "GET nonexisting did not return 3 (got: '$(cat tmp/client_out)')"
fi

# get a key inside a section
$CLIENT --get section.key >/tmp/client_out;
if grep -q "value" /tmp/client_out; then
    ok "GET host returned expected value 'value'"
else
    fail "GET host failed (got: '$(cat tmp/client_out)')"
fi

# 2.ix — launch the client and set a new key/value pair
$CLIENT --set section.color.red "roses are red"

# 2.x verify operation success
if grep -q "value" /tmp/client_out; then
    ok "SET section.color.red returned 0 (success)"
else
    fail "SET section.color.red failed (got: '$(cat /tmp/client_out)')"
fi
if grep -q "roses" $INI_FILE; then
    ok "set value is persisted"
else
    fail "I have no roses in memory"
fi

# 2.xi — stop server with SIGINT
kill -SIGINT "$SERVER_PID" 2>/dev/null
sleep 0.5
if kill -0 "$SERVER_PID" 2>/dev/null; then
    fail "server still alive after SIGINT"
else
    ok "server terminated cleanly"
fi
#
# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo "Results: $PASS passed, $FAIL failed."
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
