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
    exit 1;
fi

sess=`echo "$output" | grep sesion | cut -d " " -f 2`
if [ "blank$sess" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    exit 1;
fi

# Intento crear un juego de Ajedrez
output=`curl -f "http://localhost:8080/$sess/crea/Gomoku" --stderr /dev/null`
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

output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "move=f3"  --stderr /dev/null`
ret=$?
if [ $ret == 0 ]; then
  echo "$output"
  echo "Debe dar error ya que no se pasaron correctamente los parametros por post"
  kill -2 $PID
  exit 1;
fi


output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=f3"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error 1: $output"
  kill -2 $PID
  exit 1;
fi


output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=f4"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error 2: $output"
  kill -2 $PID
  exit 1;
fi

# Aca viene la joda. Cierro el servidor y lo vuelvo a abrir
kill -2 $PID
sleep 1 # le doy un segundo como para que cierre

$PATHQS/qgserverd &
PID=$!
sleep 1 # le doy un segundo como para que levante

output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=f5"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error 3: $output"
  kill -2 $PID
  exit 1;
fi



# FIn del test, paro el servidor

kill -2 $PID
exit 0
