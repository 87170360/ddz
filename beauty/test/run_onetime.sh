#!/bin/bash

kill_robot_nu=0


ps -Af  >bet_client.txt
cat bet_client.txt | grep bet_client > /dev/null 

if [[ $? == "0" ]]
then 
	echo `date` "Bet Client Ok"	
else 
	echo `date` "Resteart Bet Client"
	for((i=10;i<19;i++)) 
	do 
		./bet_client 127.0.0.1  11000 $i >/dev/null & 
	done 
fi






