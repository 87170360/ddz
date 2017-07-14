#!/bin/bash

while true 
do 
	echo "start loop"
	./run_onetime.sh 
	sleep 20
	./kill_bet_client.sh 
done


