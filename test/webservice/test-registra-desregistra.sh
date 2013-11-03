#!/bin/bash
PATHQS=../src
JUEGO=Ajedrez


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

# Intento crear un juego de lo que sea
output=`curl -f "http://localhost:8080/$sess/crea/$JUEGO" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

game=`echo "$output" | grep game_id | cut -d " " -f 2`
if [ "blank$game" == blank ]; then
    echo  "No encontre juego $output"
    kill -2 $PID
    wait
    exit 1;
fi

echo "Juego creado $game"

# Vamos a mover n veces
for i in {0..5}; do
    output=`curl -f "http://localhost:8080/$sess/posibles/$game" --stderr /dev/null`
    movidas=`echo "$output" | grep "total: " | cut -f 2 -d " "`
    (( move = RANDOM % movidas ))
    curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=$move"  --stderr /dev/null > /dev/null
    ret=$?
    if [ $ret != 0 ]; then
        echo  "Error en interacion $i m=$move"
        curl  "http://localhost:8080/$sess/mueve/$game" -d "m=$move" 
        kill -2 $PID
        wait
        exit 1;
    fi
done

# Salvamos primeramente todo el tablero y las movidas posibles
output=`curl -f "http://localhost:8080/$sess/posibles/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - posibles"
    kill -2 $PID
    wait
    exit 1;
fi

info_mov_save="$output"
info_tab_save=`curl -f "http://localhost:8080/$sess/tablero/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - tablero"
    kill -2 $PID
    wait
    exit 1;
fi

# contamos registraciones
cantidad=`curl -f "http://localhost:8080/$sess/registraciones"  --stderr /dev/null | grep cantidad | cut -d " " -f 2`
if [ x$cantidad != x1 ]; then
    echo "controlando registraciones cantidad $cantidad != 1"
    kill -2 $PID
    wait
    exit 1;
fi


# Primero metemos la data de la partida en un archivo temporal
tmpfile=/tmp/$$
curl -f "http://localhost:8080/$sess/partida/$game" --stderr /dev/null > $tmpfile
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret al leer partida"
    kill -2 $PID
    wait
    exit 1;
fi




# Ahora desregistraremos el juego
output=`curl -f "http://localhost:8080/$sess/desregistra/$game" -d "" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - deregistra $game"
    rm $tmpfile
    kill -2 $PID
    wait
    exit 1;
fi

if [ "$output" != "Partida desregistrada" ]; then
    echo "Esperado Partida desregistrada. Encontrado $output"
    rm $tmpfile
    kill -2 $PID
    wait
    exit 1;
fi

# El juego no esta mas
output=`curl -f "http://localhost:8080/$sess/posibles/$game" --stderr /dev/null`
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    rm $tmpfile
    kill -2 $PID
    wait
    exit 1;
fi

# contamos registraciones
cantidad=`curl -f "http://localhost:8080/$sess/registraciones"  --stderr /dev/null | grep cantidad | cut -d " " -f 2`
if [ x$cantidad != x0 ]; then
    echo "controlando registraciones cantidad $cantidad != 0"
    kill -2 $PID
    wait
    exit 1;
fi


# Volvemos a registrar el juego, con otro nombre, porque no.
output=`curl -f "http://localhost:8080/$sess/registra/r$game" --stderr /dev/null --data-binary @$tmpfile`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret al volver a registrar partida"
    kill -2 $PID
    rm $tmpfile
    wait
    exit 1;
fi
rm $tmpfile

# Leemos la info de nuestro nuevo juego
info_mov_new=`curl -f "http://localhost:8080/$sess/posibles/r$game" --stderr /dev/null | sed "s/game_id: r/game_id: /"`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

info_tab_new=`curl -f "http://localhost:8080/$sess/tablero/r$game" --stderr /dev/null | sed "s/game_id: r/game_id: /"`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

if [ "$info_mov_new" != "$info_mov_save" ]; then
    echo "No son lo mismo --- (Movidas) "
    echo "$info_mov_new"
    echo "$info_mov_save"
    kill -2 $PID
    wait
    exit 1;
fi
    
if [ "$info_tab_new" != "$info_tab_save" ]; then
    echo "No son lo mismo --- (Tablero) "
    echo "$info_tab_new"
    echo "$info_tab_save"
    kill -2 $PID
    wait
    exit 1;
fi

# contamos registraciones
cantidad=`curl -f "http://localhost:8080/$sess/registraciones"  --stderr /dev/null | grep cantidad | cut -d " " -f 2`
if [ x$cantidad != x1 ]; then
    echo "controlando registraciones cantidad $cantidad != 1"
    kill -2 $PID
    wait
    exit 1;
fi

kill -2 $PID
wait
exit 0
