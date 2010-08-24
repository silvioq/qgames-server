#!/bin/bash
PATHQS=../src
JUEGO=Gomoku


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
    exit 1;
fi

sess=`echo "$output" | grep sesion | cut -d " " -f 2`
if [ "blank$sess" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    exit 1;
fi

# Intento crear un juego de lo que sea
output=`curl -f "http://localhost:8080/$sess/crea/$JUEGO" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    exit 1;
fi

game=`echo "$output" | grep game_id | cut -d " " -f 2`
if [ "blank$game" == blank ]; then
    echo  "No encontre juego $output"
    kill -2 $PID
    exit 1;
fi

echo "Juego creado $game"

# Vamos a mover n veces
for i in {0..5}; do
    output=`curl -f "http://localhost:8080/$sess/posibles/$game" --stderr /dev/null`
    movidas=`echo "$output" | grep "total: " | cut -f 2 -d " "`
    (( move = RANDOM % movidas ))
    curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=$move"  --stderr /dev/null
    ret=$?
    if [ $ret != 0 ]; then
        echo  "Error en interacion $i m=$move"
    fi
done

    

kill -2 $PID
exit 0
