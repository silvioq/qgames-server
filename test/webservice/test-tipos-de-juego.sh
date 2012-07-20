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

# Me logoneo bien
output=`curl -f "http://localhost:8080/login" -d "user=root&pass=root" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

sess=`echo "$output" | grep sesion | cut -d " " -f 2`
if [ "blank$sess" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    wait
    exit 1;
fi

# Listo los tipos de juego
output=`curl -f "http://localhost:8080/$sess/lista" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret (list)"
    kill -2 $PID
    exit 1;
fi


echo "$output" | grep Ajedrez > /dev/null
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret buscando ajedrez"
    kill -2 $PID
    wait
    exit 1;
fi

if [ "x$1" == "x-v" ]; then
  echo "$output"
fi

# Una prueba
output=`curl -f "http://localhost:8080/$sess/lista/Ajedrez" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret (list)"
    kill -2 $PID
    exit 1;
fi

kill -2 $PID
wait
exit 0
