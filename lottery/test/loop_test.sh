#!/bin/bash

while true 
do 
	echo "start loop"
	./run_onetime.sh 
	sleep 2 
	./kill_bet_client.sh 
done


