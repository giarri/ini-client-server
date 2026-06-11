# the server needs to be running
./build/ini_client --load example.ini | grep -q "0" && echo "Load tests passed" || echo "Load tests failed"
./build/ini_client --set section.color.red "roses are red" | grep -q "0" && echo "Set tests passed" || echo "Set tests failed"
./build/ini_client --get debug | grep -q "0 true$" && echo "Get tests passed" || echo "Get tests failed"
