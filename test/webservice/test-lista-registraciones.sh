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

# Intento crear varios juegos
for i in Ajedrez Ajedrez Jubilado Gomoku; do
  output=`curl -f "http://localhost:8080/$sess/crea/$i" --stderr /dev/null`
  ret=$?
  if [ $ret != 0 ]; then
      echo "Esperado 0. Encontrado $ret - crea"
      kill -2 $PID
      exit 1;
  fi
done

# Leo informacion de las partidas que tengo dando vuelta
output=`curl -f "http://localhost:8080/$sess/registraciones" --stderr /dev/null`
ret=`echo "$output" | grep "cantidad:" | cut -f 2 -d " "`
if [ x$ret != x4 ]; then
    echo "Estao esperando 4 partidas y tengo $ret"
    kill -2 $PID
    exit 1;
fi

echo "$output"



kill -2 $PID
exit 0
