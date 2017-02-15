ps -Af > aaa.txt
kill `cat aaa.txt | grep beauty_client |awk '{ print $2}'`
