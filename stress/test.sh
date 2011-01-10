#!/bin/bash
PATHQS=./src
DBFILE=./stress/qgserver.db
VERBOSE=
PORT=8015


. ./stress/functions.sh


# Primeramente, cargo una base de datos vacia
rm $DBFILE

echo "Creando archivo"
$PATHQS/qgserver-tool -c $DBFILE
ret=$?
if [ $ret != 0 ]; then
    exit 1;
fi

# Voy a intentar ejecutar el servicio web
echo "Iniciando servidor"
$PATHQS/qgserverd -d $DBFILE -p $PORT $VERBOSE &
PID=$!

sleep 1 # le doy un segundo como para que levante

sess=$(sesslogin)

function   juega_hasta_200(){
    tipojuego=`selecttipojuego`
    echo $tipojuego
    movidas=200
    game=`creajuego $tipojuego`
    for i in `seq 1 $movidas`; do
        move=`selectposibles $game`
        mueve $move

        ret=$?
        if [ $ret != 0 ]; then
          curl -f "http://localhost:$PORT/$sess/tablero/$game" --stderr /dev/null
          echo "Error: $ret movida $move"
          exit 1;
        fi

        stat=`statusgame $game`
        if [ "x$stat" != "xJugando" ]; then
            echo "Termina => Estado $stat ($i)"
            exit
        fi
        printf " $i"
    done
    echo  "Termina $tipojuego => movidas $movidas "
}

waiting=""
for i in `seq 1 30`; do
    juega_hasta_200 $i &
    waiting="$waiting $!"
done

wait  $waiting

echo "Fin de todos los procesos, cancelo"
kill -2 $PID
wait
exit 0
