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

# Bien. Primer intento, me logoneo mal
curl "http://localhost:8080/login" -f -d "user=root&pass=mala"
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    exit 1;
fi

# Me logoneo bien
output=`curl -f "http://localhost:8080/login" -d "user=root&pass=root" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    exit 1;
fi

sess=`echo "$output" | grep sesion | cut -d " " -f 2`
if [ "blank$sess" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    exit 1;
fi

# Intento crear un juego de Ajedrez
output=`curl -f "http://localhost:8080/$sess/crea/Ajedrez" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    exit 1;
fi

game=`echo "$output" | grep game_id | cut -d " " -f 2`
if [ "blank$game" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    exit 1;
fi

echo "Juego creado $game"

# Vamos a intentar leer la informacion de ese partido que creamos
output=`curl -f "http://localhost:8080/$sess/tablero/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    exit 1;
fi

echo $output



kill -2 $PID
exit 0
