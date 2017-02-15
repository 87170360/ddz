#!/bin/bash

kill_robot_nu=0


while true 
do 
	ps -Af  >lottery_client.txt
	cat lottery_client.txt | grep lottery_client > /dev/null 

	if [[ $? == "0" ]]
	then 
		echo `date` "Bet Client Ok"	
	else 
		echo `date` "Resteart Bet Client"
		for((i=10;i<19;i++)) 
		do 
			./lottery_client 127.0.0.1  9100 $i >/dev/null & 
		done 
	fi
	
	kill_robot_nu=$(( $kill_robot_nu + 1))
	sleep 30

	if [[ "$kill_robot_nu" -gt "120" ]] 
	then 
		echo `date` "Kill All Client"
		kill `cat lottery_client.txt |grep lottery_client |awk '{ print $2}'`
		kill_robot_nu=0
	fi

done 





