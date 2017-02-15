#!/bin/bash

kill_robot_nu=0


while true 
do 
	ps -Af  >beauty_client.txt
	cat beauty_client.txt | grep bet_client > /dev/null 

	if [[ $? == "0" ]]
	then 
		echo `date` "Bet Client Ok"	
	else 
		echo `date` "Resteart Bet Client"
		for((i=10;i<19;i++)) 
		do 
			./beauty_client 127.0.0.1  11000 $i >/dev/null & 
		done 
	fi
	
	kill_robot_nu=$(( $kill_robot_nu + 1))
	sleep 30

	if [[ "$kill_robot_nu" -gt "120" ]] 
	then 
		echo `date` "Kill All Client"
		kill `cat beauty_client.txt |grep bet_client |awk '{ print $2}'`
		kill_robot_nu=0
	fi

done 





