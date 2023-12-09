PORT_TO_FREE=55555

PID=$(lsof -t -i :$PORT_TO_FREE)

if [ -z "$PID" ]; then
	echo "No process found using port $PORT_TO_FREE"
else
	echo "Terminating process $PID using $PORT_TO_FREE"
	kill -9 $PID
fi
