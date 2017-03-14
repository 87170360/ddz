
build:
	make -C libxtnet
	make -C libxtgame
	make -C libbull  
	make -C libzjh  
	make -C normal_bull 
	make -C normal_robot
	make -C grab_bull
	make -C grab_robot
	make -C zjhsvr  
	make -C hlddz  
	make -C hlddz_robot

clean :
	make -C libbull clean
	make -C normal_bull  clean 
	make -C normal_robot clean
	make -C grab_bull  clean 
	make -C grab_robot clean
	make -C libzjh  clean
	make -C zjhsvr clean 
	make -C hlddz clean 
	make -C hlddz_robot clean 

cp: build
	cp -f /data/src/bull/normal_bull/normal_bull  /data/game/bin 
	cp -f /data/src/bull/normal_robot/normal_robot /data/game/bin
	cp -f /data/src/bull/grab_bull/grab_bull  /data/game/bin 
	cp -f /data/src/bull/grab_robot/grab_robot /data/game/bin
	cp -f /data/src/bull/hlddz/hlddz /data/game/bin
	cp -f /data/src/bull/hlddz_robot/hlddz_robot /data/game/bin
