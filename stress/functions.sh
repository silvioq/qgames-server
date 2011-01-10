
function  mueve(){
    move=$1
    # echo "curl -f  "http://localhost:$PORT/$sess/mueve/$game" --data-urlencode "m=$move"  --stderr /dev/null"
    output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" --data-urlencode "m=$move"  --stderr /dev/null`
    return $?
}


function   sesslogin(){
    output=`curl -f "http://localhost:$PORT/login" -d "user=root&pass=root" --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
        echo "Error en login: Esperado 0. Encontrado $ret"
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
    echo $sess
    exit 0;
}


function creajuego(){
    tipojuego=$1
    output=`curl -f "http://localhost:$PORT/$sess/crea/$tipojuego" --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
        echo "Esperado 0. Encontrado $ret"
        kill -2 $PID
        exit 1;
    fi
    
    game=`echo "$output" | grep game_id | cut -d " " -f 2`
    if [ "blank$game" == blank ]; then
        echo  "No encontre game $output"
        kill -2 $PID
        wait
        exit 1;
    fi
    echo  $game
}

function   listaposibles(){
    game=$1
    output=`curl -f "http://localhost:$PORT/$sess/posibles/$game" --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
        echo "Esperado 0. Encontrado $ret"
        kill -2 $PID
        exit 1;
    fi

    echo "$output" | grep notacion | cut -d " " -f 4
}

function   statusgame(){
    game=$1
    output=`curl -f "http://localhost:$PORT/$sess/tablero/$game" --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
        echo "statusgame: Esperado 0. Encontrado $ret"
        kill -2 $PID
        exit 1;
    fi
    echo "$output" | grep descripcion_estado | cut -d " " -f 2,3
}
    


function   selectposibles(){
    game=$1
    posibles=`listaposibles $game`
    movidas=`echo "$posibles" | wc -l `
    elegida=$(($RANDOM * $movidas / 32768))
    elegida=$(($elegida + 1))
    echo "$posibles" | head -n $elegida | tail -n 1
}


function   listatipos(){
    output=`curl -f "http://localhost:$PORT/$sess/lista" --stderr /dev/null`
    ret=$?
    if [ $ret != 0 ]; then
        echo "Esperado 0. Encontrado $ret"
        kill -2 $PID
        exit 1;
    fi

    echo "$output" | grep -v "^ " | cut -d ":" -f 1

}


function  selecttipojuego(){
    posibles=`listatipos`
    movidas=`echo "$posibles" | wc -l `
    elegida=$(($RANDOM * $movidas / 32768))
    elegida=$(($elegida + 1))
    echo "$posibles" | head -n $elegida | tail -n 1
}


