#!/bin/bash

# TODO: Agregar getopts para distintos parametros (cantidad de movidas, tipos de juego, procesos)
#

function   usage(){
    cat  <<EOF

usage: $0  options

Options:
  -c      Iniciar la base (no iniciar por defecto)
  -g n    Cantidad de juegos en paralelo (default $GAMES)
  -h s    Hostname (default: localhost)
  -H home Home para environment de DBBerkeley
  -k n    Workers (experimental)
  -m n    Cantidad de movidas maxima por partido (default $MOVIDAS)
  -n      Usar valgrind (no usar por defecto)
  -p n    Puerto (default: $PORT)
  -s      Iniciar el servidor qgserverd (no iniciar por defecto)
  -v      Verbose
  -w      Watch!

EOF
}

PATHQS=./src
DBFILE=./stress/qgserver.db
VERBOSE=
WORKERS=
HOST=localhost
PORT=8015
GAMES=30
MOVIDAS=200
DBHOME=

if [ x$1 == x ]; then
    usage; exit 0;
fi






while getopts "vg:k:wh:p:scm:nH:" OPTION; do
    case $OPTION in
      c) INITDB=1;;
      g) GAMES=$OPTARG;;
      h) HOST=$OPTARG;;
      H) DBHOME="-H $OPTARG"; DBFILE=qgserver.db;;
      k) WORKERS="-w $OPTARG";;
      m) MOVIDAS=$OPTARG;;
      n) VALGRIND=1;;
      p) PORT=$OPTARG;;
      s) STARTQG=1;;
      v) VERBOSE=-v;;
      w) WATCH=1;;
      *) usage; exit 0;
    esac
done



. ./stress/functions.sh


# Primeramente, cargo una base de datos vacia
if [ x$INITDB == x1 ]; then
    rm $DBFILE
    echo "Creando archivo"
    $PATHQS/qgserver-tool -c $DBFILE  $DBHOME
    ret=$?
    if [ $ret != 0 ]; then
        exit 1;
    fi
fi

# el directorio de archivos temporales.
if [ ! -e ./stress/tmp ]; then
    mkdir  ./stress/tmp
fi

# Voy a intentar ejecutar el servicio web
if [ x$STARTQG == x1 ]; then
    if [ x$VALGRIND == x1 ]; then
        echo "Iniciando servidor valgrind"
        valgrind  --leak-check=full --show-reachable=yes \
               $PATHQS/qgserverd -d $DBFILE   $DBHOME  \
               -p $PORT $VERBOSE $WORKERS &> ./stress/tmp/salida.txt &
        PID=$!
        sleep 5 # le doy un segundo como para que levante
    else
        echo "Iniciando servidor"
        $PATHQS/qgserverd -d $DBFILE -p $PORT $VERBOSE $WORKERS $DBHOME > ./stress/tmp/salida.txt &
        PID=$!
        sleep 1 # le doy un segundo como para que levante
    fi
fi


sess=$(sesslogin)

function   juega_hasta_200(){
    tipojuego=`selecttipojuego`
    num=$1
    echo $tipojuego
    game=`creajuego $tipojuego`
    for i in `seq 1 $MOVIDAS`; do
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
            echo "Termina => Estado $stat (num: $num mov:$i)"
            exit
        fi
        printf " $i"
    done
    echo  "Termina $tipojuego => movidas $movidas "
    exit
}



function  watch_registraciones(){
    sleep 1
    while [ true ]; do 
        clear
        registraciones 
        sleep 2
    done
}



waiting=""
for i in `seq 1 $GAMES`; do
    juega_hasta_200 $i > ./stress/tmp/$i &
    waiting="$waiting $!"
done

if [ x$WATCH == x1 ]; then
    watch_registraciones &
fi

wait  $waiting
wait
exit 0
