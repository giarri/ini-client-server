nc -w 1 localhost 12345 && echo "OK - CLIENT CONNECTS"

echo LOAD $(pwd)/example.ini | nc -w 1 localhost 12345 |\
	grep -q "0" && echo "OK - LOAD WORKS"

echo UNKNOWN COMMAND | nc -w 1 localhost 12345 |\
	grep -q "127" && echo "OK - UNKNOWN COMMAND CODE 127"

printf "LOAD $(pwd)/example.ini\nGET host\n" | nc -w 2 localhost 12345 |\
	grep -q "localhost" && echo "OK - GET WORKS"

printf "LOAD $(pwd)/example.ini\nGET section.key\n" | nc -w 2 localhost 12345 |\
	grep -q "value" && echo "OK - GET IN SECTION-ELEMENT WORKS"

printf "LOAD $(pwd)/example.ini\nSET section.color.red roses are red\n" | nc -w 2 localhost 12345 |\
	grep -q "0" && echo "OK - SET WORKS"
