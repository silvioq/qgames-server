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

for i in a1 b1 c1 d1 e1 f1 g1 h1 a2 b2 c2 d2 e2 f2 g2 h2; do
  ret=`echo "$output" | grep "casillero: $i"`
  ret=$?
  if [ $ret != 0 ]; then
    echo "Error: $output"
    echo "No se encuentra casillero $i"
    kill -2 $PID
    exit 1;
  fi
done

ret=`echo "$output" | grep "total:" | cut -f 2 -d " "`
if [ x$ret != x32 ]; then
    echo  "Estoy esperando 32 piezas y tengo $ret"
    kill -2 $PID
    exit 1;
fi
    
output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "move=Nf3"  --stderr /dev/null`
ret=$?
if [ $ret == 0 ]; then
  echo "$output"
  echo "Debe dar error ya que no se pasaron correctamente los parametros por post"
  kill -2 $PID
  exit 1;
fi


output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=Nf3"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error: $output"
  kill -2 $PID
  exit 1;
fi

# Vamos a intentar leer la informacion de ese partido que creamos
output=`curl -f "http://localhost:8080/$sess/tablero/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    exit 1;
fi


ret=`echo "$output" | grep "total:" | cut -f 2 -d " "`
if [ x$ret != x32 ]; then
    echo  "Estoy esperando 32 piezas y tengo $ret"
    kill -2 $PID
    exit 1;
fi

for i in a1 b1 c1 d1 e1 f1 f3 h1 a2 b2 c2 d2 e2 f2 g2 h2; do
  ret=`echo "$output" | grep "casillero: $i"`
  ret=$?
  if [ $ret != 0 ]; then
    echo "Error: $output"
    echo "No se encuentra casillero $i"
    kill -2 $PID
    exit 1;
  fi
done

# El caballo no esta mas en g1
ret=`echo "$output" | grep "casillero: g1"` 
ret=$?
if [ $ret == 0 ]; then
  echo "algo quedo en g1"; echo $output
  kill -2 $PID
  exit 1;
fi

# Vamos a intentar leer la informacion de ese partido que creamos
output=`curl -f "http://localhost:8080/$sess/historial/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - historial $output"
    kill -2 $PID
    exit 1;
fi

output=`curl -f  "http://localhost:8080/$sess/mueve/$game" -d "m=d5"  --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
  echo "Error: $output"
  kill -2 $PID
  exit 1;
fi


output=`curl -f "http://localhost:8080/$sess/historial/$game" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - historial $output 2"
    kill -2 $PID
    exit 1;
fi

kill -2 $PID
exit 0
