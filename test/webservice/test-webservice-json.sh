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
curl "http://localhost:8080/login.json" -f -d "user=root&pass=mala"
ret=$?
if [ $ret != 22 ]; then
    echo "Esperado 22. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

# Me logoneo bien
output=`curl -f "http://localhost:8080/login.json" -d "user=root&pass=root" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret"
    kill -2 $PID
    wait
    exit 1;
fi

sess=`echo "$output" | grep sesion | cut -f 3 | sed s/^\"// | sed s/\"\,*$//`
if [ "blank$sess" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    wait
    exit 1;
fi

# Intento crear un juego de Ajedrez
output=`curl -f "http://localhost:8080/$sess/crea/Ajedrez.json" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - crea"
    kill -2 $PID
    wait
    exit 1;
fi

game=`echo "$output" | grep game_id | cut -f 3 | sed s/^\"// | sed s/\"\,*$//`
if [ "blank$game" == blank ]; then
    echo  "No encontre sesion $output"
    kill -2 $PID
    wait
    exit 1;
fi

# echo "Juego creado $game"

# Vamos a intentar leer la informacion de ese partido que creamos
output=`curl -f "http://localhost:8080/$sess/tablero/$game.json" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - tablero"
    kill -2 $PID
    wait
    exit 1;
fi

ret=`echo "$output" | grep "total" | cut -f 3 | sed s/,*$//`
if [ x$ret != x32 ]; then
    echo  "Estoy esperando 32 piezas y tengo $ret"
    kill -2 $PID
    wait
    exit 1;
fi
    

for i in a1 b1 c1 d1 e1 f1 g1 h1; do
  echo "$output" | grep  "casillero\":" | grep "\"$i\"" > /dev/null
  ret=$?
  if [ $ret != 0 ]; then
    echo "No se encuentra casillero $i"
    echo  "$output"
    kill -2 $PID
    wait
    exit 1;
  fi
done

output=`curl -f "http://localhost:8080/$sess/posibles/$game.json" --stderr /dev/null`
echo "$output" | grep "notacion\":" | grep "\"Nf3" > /dev/null
ret=$?
if [ $ret != 0 ]; then
  echo "No se encuentra Nf3"
  kill -2 $PID
    wait
  exit 1;
fi

echo "$output" | grep "notacion\":" | grep "\"Nf4" > /dev/null
ret=$?
if [ $ret == 0 ]; then
  echo "Se encuentra Nf4"
  kill -2 $PID
  wait
  exit 1;
fi

# Controlo json de juegos
output=`curl -f "http://localhost:8080/$sess/lista/Ajedrez.json" --stderr /dev/null | grep \"imagen\" | wc -l`
ret=$?
if [ $ret != 0 ]; then
  kill -2 $PID
  wait
  exit 1;
fi
if [ x$output != x26 ]; then
    echo  "Estoy esperando 26 lineas y tengo $output"
    kill -2 $PID
    wait
    exit 1;
fi

# Muevo!
output=`curl -f "http://localhost:8080/$sess/mueve/$game.json" --stderr /dev/null -d "m=d4" `
ret=$?
if [ $ret != 0 ]; then
  echo "Error moviendo : $ret   $output"
  kill -2 $PID
  wait
  exit 1;
fi

movs=`echo "$output" | grep cantidad_movidas\"\: | cut -f 3 | sed s/,*$//`
if [ x$movs != x1 ]; then
    echo  "Estoy esperando 1 movida y tengo $movs"
    kill -2 $PID
    wait
    exit 1;
fi

# Saliendo
output=`curl -f "http://localhost:8080/$sess/logout.json" --stderr /dev/null`
ret=$?
if [ $ret != 0 ]; then
    echo "Esperado 0. Encontrado $ret - crea"
    kill -2 $PID
    wait
    exit 1;
fi


echo "$output" | grep "respuesta" | grep "OK" > /dev/null
ret=$?
if [ $ret != 0 ]; then
  echo "No se encuentra respuesta"
  kill -2 $PID
    wait
  exit 1;
fi

kill -2 $PID
wait
exit 0
