
function  mueve(){
    move=$1
    # echo "curl -f  "http://localhost:$PORT/$sess/mueve/$game" --data-urlencode "m=$move"  --stderr /dev/null"
    output=`curl -f  "http://localhost:$PORT/$sess/mueve/$game" --data-urlencode "m=$move"  --stderr /dev/null`
    return $?
}
