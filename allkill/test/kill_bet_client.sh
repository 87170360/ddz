ps -Af > aaa.txt
kill `cat aaa.txt | grep bet_client |awk '{ print $2}'`
