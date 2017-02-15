
#下注流水
DROP TABLE IF EXISTS bet_flow;
CREATE TABLE `bet_flow` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `vid` int(11) DEFAULT NULL COMMENT '场管vid',
  `uid` int(11) DEFAULT NULL COMMENT '玩家uid',
  `bet_num` int(11) DEFAULT NULL COMMENT '下注额度',
  `bet_time` timestamp DEFAULT CURRENT_TIMESTAMP COMMENT '下注时间',
  `reward_money` int(11) DEFAULT NULL COMMENT '输赢的钱',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

#下注宝箱
DROP TABLE IF EXISTS bet_lottery_daily;
CREATE TABLE `bet_lottery_daily` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `vid` int(11) DEFAULT NULL COMMENT '场管vid',
  `uid` int(11) DEFAULT NULL COMMENT '玩家uid',
  `lottery_time` timestamp DEFAULT CURRENT_TIMESTAMP COMMENT '彩票时间',
  `lottery_money` int(11) DEFAULT NULL COMMENT '彩金值',
  `box_type` int(11) DEFAULT NULL COMMENT '宝箱类型',  
  `get_flag` int(11) DEFAULT 0 COMMENT '领取标记',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

#宝箱配置
DROP TABLE IF EXISTS bet_box_cfg;
CREATE TABLE `bet_box_cfg` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `vid` int(11) DEFAULT NULL COMMENT '场管的id',
  `box_type` int(11) DEFAULT NULL COMMENT '宝箱类型',  
  `box_name` VARCHAR(32) DEFAULT "宝箱名" COMMENT '宝箱名称',
  `bet_need` int(11) DEFAULT NULL COMMENT '所需下注额度',  
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;


#下注流水触发器，生成下注宝箱bet_lottery_daily
DROP TRIGGER IF EXISTS t_afterinsert_on_bet_flow;
CREATE TRIGGER t_afterinsert_on_bet_flow 
AFTER INSERT ON bet_flow
FOR EACH ROW
BEGIN
     #错误定义，标记循环结束 
     DECLARE _done int default 0; 
   
     #今天下注总额
     DECLARE totalBet int(11) DEFAULT 0;

     #今天输赢的钱
     DECLARE rewardBet int(11) DEFAULT 0;
     
     #奖厉的钱
     DECLARE lotteryMoney int(11) DEFAULT 0;

     #前一个宝箱所需的下注额度
     DECLARE preBetNeed int(11) DEFAULT 0;
     
     #系统抽水比例
     DECLARE sys_fee FLOAT DEFAULT 0.04;
 
	 #下注宝箱个数 
     DECLARE boxCount INT DEFAULT 0;

     #宝箱配置值
     DECLARE boxType INT DEFAULT 0;
	 DECLARE betNeed INT DEFAULT 0;

	 #宝箱配置游标
     DECLARE cur_box CURSOR FOR 
          SELECT box_type, bet_need FROM bet_box_cfg ORDER BY bet_need ASC;

     DECLARE CONTINUE HANDLER FOR SQLSTATE '02000' SET _done = 1;

     #累计总的下注  #累计输赢
     SELECT SUM(bet_num), SUM(reward_money) INTO totalBet, rewardBet FROM bet_flow 
         WHERE vid=new.vid AND uid=new.uid AND to_days(bet_time) = to_days(now());  

		 OPEN cur_box;

         #循环处理
		 REPEAT  

			FETCH cur_box INTO boxType, betNeed; 

			IF NOT _done THEN  
                
                #满足宝箱生成条件
				IF totalBet>=betNeed THEN  

					#判断有没生成此宝箱
					SELECT COUNT(*) INTO boxCount FROM bet_lottery_daily 
                    WHERE box_type=boxType AND vid=new.vid AND uid=new.uid AND to_days(lottery_time)=to_days(now()); 

                    #插入下注宝箱
					IF boxCount=0 THEN  

					   #赢钱
					   IF rewardBet > 0 THEN
							SET lotteryMoney = rewardBet*sys_fee*0.2;
					   END IF;

					   #输钱
					   IF rewardBet <= 0 THEN
							SET lotteryMoney = -rewardBet*sys_fee*0.2 + 0.001*(betNeed-preBetNeed);
					   END IF;


					   INSERT INTO bet_lottery_daily(vid,uid,lottery_money,box_type) 
                             VALUES (new.vid, new.uid, lotteryMoney, boxType); 
					END IF;  

                    #记录前一个值
                    SET preBetNeed=betNeed;
								
				END IF;  
			END IF;  
		 UNTIL _done END REPEAT; #当_done=1时退出被循  

		 CLOSE cur_box;  
 
END;