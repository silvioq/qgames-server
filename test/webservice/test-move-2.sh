#!/bin/bash
PATHQS=../src
PORT=8080


# Primeramente, cargo una base de datos vacia
rm qgserver.db
$PATHQS/qgserver-tool -c qgserver.db
ret=$?
if [ $ret != 0 ]; then
    exit 1;
fi

# Voy a intentar ejecutar el servicio web
$PATHQS/qgserverd -p $PORT &
PID=$!

sleep 1 # le doy un segundo como para que levante

# Me logoneo bien
output=`curl -f "http://localhost:$PORT/login" -d "user=root&pass=root" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Error 1. Esperado 0. Encontrado $ret"
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

# Intento crear un juego de Ajedrez
output=`curl -f "http://localhost:$PORT/$sess/crea/Gomoku" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Error 2. Esperado 0. Encontrado $ret"
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

echo "Juego creado $game"

output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" -d "move=f3"  --stderr /dev/null`
ret=$?
if [ $ret == 0 ]; then
  echo "$output"
  echo "Debe dar error ya que no se pasaron correctamente los parametros por post"
  kill -2 $PID
  wait
  exit 1;
fi


output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" -d "m=f3"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error 1: $output"
  kill -2 $PID
  wait
  exit 1;
fi


output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" -d "m=f4"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error 2: $output"
  kill -2 $PID
  wait
  exit 1;
fi

# Aca viene la joda. Cierro el servidor y lo vuelvo a abrir
kill -2 $PID
wait

$PATHQS/qgserverd &
PID=$!
sleep 1 # le doy un segundo como para que levante

output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" -d "m=f5"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error 3: $output"
  kill -2 $PID
  wait
  exit 1;
fi


# Esta prueba es para la instruccion historial.
output=`curl -f "http://localhost:$PORT/$sess/crea/Jubilado" --stderr /dev/null`
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
    wait
    exit 1;
fi

echo "Juego creado $game"
for move in b4 b6 c4 g6 g4 g5 a3 b5 c5 Kd8 d3 h6 h4 Ke8 hxg5 a6 gxh6 c6 h7 f6 f3 Kf8 a4 d6 "h8=R" \
            Kf7 Ra8 Kg6 Rxa6 Kh6 a5 Kg6 Rb6 f5 d4 e6 Kf1 f4 Kf2 Kf6 Ke1 Kf7 e4 Kf8 Rb8 Kg7 cxd6 \
            e5 Kf1 Kh7 Ke1 exd4 Rb7 Kg6 Kd2 d3 d7 Kg5 Rxb5 Kg6 "d8=R" Kh6 Rh5 Kg6 Rh5-h8 c5 g5 c4 \
            Rd8-e8 Kxg5 Rh4 c3 Kd1 Kg6 "Rh4-h8" d2 Re8-f8 c2 Kxc2 "d1=R" Kb2 Re1 Rf5 Rd1 Rd8 Ra1 Rf7 \
            Kxf7 Rd1 Rc1 Rd6 Re1 e5 Re4 Kb1 Rxe5 Rd3 Ke7 Rd6 Kf8 Rd8 Re8 Kb2 Kg8 Ka1 Kf8 Rb8 Kg8 \
            Kb1 Kg7 Rb7 Re7 Kc2 Kg6 Rc7 Rxc7 Kd1 Rc1 Kxc1 Kf5 Kc2 Kf6 Kd2 Kg6 Kd1 Kg5 Ke2 Kg6 Kd3 \
            Kg5 Ke4 Kh5 Kd4 Kg5 Ke5 Kg6 Kd6 Kh5 Ke7 Kh4 Kf6 Kh5 Kf5 Kh6 Ke5 Kh5 Ke4 Kg5 Kd4 Kf5 \
            a6 Kg6 Ke4 Kh5 b5 Kh6 Kd4 Kg6 b6 Kg5 Ke5 Kg6 Kd6 Kf5 Kd7 Kg6 Kc7 Kf7 Kb7 Ke8 a7 Kf7 \
            Ka8 Kg6 Kb8 Kh6 Ka8 Kg6 b7 Kh6 "b8=R" Kh7 Rg8 Kh6 Rd8 Kh5 Kb8 Kg5 Ka8 Kh4 Rg8 Kh5 Kb8 \
            Kh6 Kc7 Kh5 Rg5 Kxg5 Kd8 Kf6 ; do
    output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" -d "m=$move"  --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
      echo "Error: $ret $output"
      kill -2 $PID
      wait
      exit 1;
    fi
    output=`curl -f "http://localhost:$PORT/$sess/historial/$game" --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
        echo "Esperado 0. Encontrado $ret - historial $output 2"
        kill -2 $PID
        wait
        exit 1;
    fi
done


# FIn del test, paro el servidor

kill -2 $PID
wait
exit 0
