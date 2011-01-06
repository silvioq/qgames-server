#!/bin/bash
PATHQS=../src


# Primeramente, cargo una base de datos vacia
rm qgserver.db
$PATHQS/qgserver-tool -c qgserver.db
ret=$?
if [ $ret != 0 ]; then
    exit 1;
fi

$PATHQS/qgserver-tool -d qgserver.db -u test -n test << EOF
clave
EOF
ret=$?
if [ $ret != 0 ]; then
    echo "Error al intentar crear usuario test"
    wait
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
    wait
    exit 1;
fi

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
    exit 1;
fi

# Intento crear un juego de Ajedrez
output=`curl -f "http://localhost:8080/$sess/crea/Ajedrez" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

game=`echo "$output" | grep game_id | cut -d " " -f 2`
if [ "blank$game" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    wait
    exit 1;
fi

# echo "Juego creado $game"

# Vamos a intentar leer la informacion de ese partido que creamos
output=`curl -f "http://localhost:8080/$sess/tablero/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

ret=`echo "$output" | grep "total:" | cut -f 2 -d " "`
if [ x$ret != x32 ]; then
    echo  "Estoy esperando 32 piezas y tengo $ret"
    kill -2 $PID
    wait
    exit 1;
fi
    
output=`curl -f "http://localhost:8080/$sess/posibles/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  kill -2 $PID
    wait
  exit 1;
fi



# Bien, ahora hago lo mismo, pero con el usuario test
sess_root=$sess
game_root=$game

# Primer intento, me logoneo mal
curl "http://localhost:8080/login" -f -d "user=test&pass=mala"
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

# Me logoneo bien
output=`curl -f "http://localhost:8080/login" -d "user=test&pass=clave" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret en login"
    kill -2 $PID
    wait
    exit 1;
fi

# Tomo la sesion
sess=`echo "$output" | grep sesion | cut -d " " -f 2`
if [ "blank$sess" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    wait
    exit 1;
fi

# Vamos a intentar leer la informacion de ese partido de root
output=`curl -f "http://localhost:8080/$sess/tablero/$game_root" --stderr /dev/null`
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret leyendo tablero"
    kill -2 $PID
    wait
    exit 1;
fi

# Vamos a intentar leer la informacion de ese partido de root
output=`curl -f "http://localhost:8080/$sess/posibles/$game_root" --stderr /dev/null`
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

# Vamos a intentar mover
output=`curl -f "http://localhost:8080/$sess/mueve/$game_root" -d "m=4" --stderr /dev/null`
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

# Vamos a intentar leer la informacion de ese partido de root
output=`curl -f "http://localhost:8080/$sess/desregistra/$game_root" --stderr /dev/null`
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

# Primero metemos la data de la partida en un archivo temporal
tmpfile=/tmp/$$
# echo "curl -f "http://localhost:8080/$sess_root/partida/$game_root" --stderr /dev/null > $tmpfile"
curl -f "http://localhost:8080/$sess_root/partida/$game_root" --stderr /dev/null > $tmpfile
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret al leer partida"
    kill -2 $PID
    wait
    exit 1;
fi

# Vamos a intentar registrarle algo encima
output=`curl -f "http://localhost:8080/$sess/registra/$game_root" --data-binary @$tmpfile --stderr /dev/null`
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    rm $tmpfile
    wait
    exit 1;
fi

rm $tmpfile

# Intento crear un juego de Ajedrez
output=`curl -f "http://localhost:8080/$sess/crea/Ajedrez" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

game=`echo "$output" | grep game_id | cut -d " " -f 2`
if [ "blank$game" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    wait
    exit 1;
fi

# echo "Juego creado $game"

# Vamos a intentar leer la informacion de ese partido que creamos
output=`curl -f "http://localhost:8080/$sess/tablero/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

# Vamos a intentar mover
output=`curl -f "http://localhost:8080/$sess/mueve/$game" -d "m=4" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret moviendo con test"
    kill -2 $PID
    wait
    exit 1;
fi
# Final del tema

kill -2 $PID
wait
exit 0
