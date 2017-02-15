
build:
	make -C libxtnet
	make -C libxtgame
	make -C libbull  
	make -C normal_bull 
	make -C normal_robot
	make -C grab_bull
	make -C grab_robot

clean :
	make -C libbull clean
	make -C normal_bull  clean 
	make -C normal_robot clean
	make -C grab_bull  clean 
	make -C grab_robot clean
