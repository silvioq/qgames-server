#!/bin/bash
PATHQS=../src


# Primeramente, cargo una base de datos vacia
rm qgserver.db
$PATHQS/qgserver-tool -c qgserver.db
ret=$?
if [ $ret != 0 ]; then
    exit 1;
fi

# Voy a intentar ejecutar el servicio web
$PATHQS/qgserverd &
PID=$!

sleep 1 # le doy un segundo como para que levante

# Bien. 
output=`curl -f "http://localhost:8080/ping" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

if [ "blank$output" != "blankpong" ]; then
    echo  "No encontre pong"
    kill -2 $PID
    wait
    exit 1;
fi

# Bien. 
output=`curl -f "http://localhost:8080/ping.txt" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

if [ "blank$output" != "blankpong" ]; then
    echo  "No encontre pong"
    kill -2 $PID
    wait
    exit 1;
fi

output=`curl -f "http://localhost:8080/ping.yml" --stderr /dev/null`
resp=`echo "$output" | grep texto | cut -d " " -f 2`
if [ "blank$resp" == blank ]; then
    echo  "No encontre pong $output"
    kill -2 $PID
    wait
    exit 1;
fi

output=`curl -f "http://localhost:8080/ping.json" --stderr /dev/null`
resp=`echo "$output" | grep texto | cut -f 3 | sed s/^\"// | sed s/\"\,*$//`
if [ "blank$resp" == blank ]; then
    echo  "No encontre pong $output"
    kill -2 $PID
    wait
    exit 1;
fi

kill -2 $PID
wait
exit 0
