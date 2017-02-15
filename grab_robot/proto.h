#ifndef __PROTO_H__
#define __PROTO_H__

namespace GRAB 
{
	///---Э��ָ���
	enum system_command
	{
		SYS_ECHO					= 0001,       /* echo */
		SYS_ONLINE					= 0002,       /* get online */
	};

	///---�ͻ�������ָ��(�������ׯ��Э����+1000)
	enum client_command
	{
		CLIENT_LOGIN_REQ			= 2001,      /* join table */
		CLIENT_LOGOUT_REQ			= 2002,
		CLIENT_READY_REQ			= 2003,	     /* game ready */
		CLIENT_BET_REQ		 		= 2004,
		CLIENT_CHAT_REQ				= 2005,
		CLIENT_FACE_REQ				= 2006,
		CLIENT_CHANGE_REQ           = 2007,      /* change table */
		CLIENT_ROBOT_REQ            = 2008,
		CLIENT_INFO_REQ             = 2009,      /* update player info */
		CLIENT_TABLE_INFO_REQ       = 2010,      /* table info */
		CLIENT_EMOTION_REQ          = 2011,      /* interaction emotion */
		CLIENT_PROP_REQ             = 2012,      /* game prop */

		CLIENT_GRAB_BANKER_REQ      = 2018,      /*�����ׯ*/
		CLIENT_DOUBLE_BET_REQ       = 2019,      /*��Ҽӱ�*/
		CLIENT_GROUP_CARD_REQ       = 2020,      /*�������*/
	};

	///---���bet��action
	enum client_action
	{
		PLAYER_CALL 					= 2001,       /* call */
		PLAYER_RAISE	                = 2002,       /* raise */
		PLAYER_COMPARE		            = 2003,       /* compare */
		PLAYER_SEE						= 2004,		  /* see */
		PLAYER_FOLD	                    = 2005,       /* fold */
		PLAYER_ALLIN              = 2006,     /* all in */
		PLAYER_ALLIN_COMPARE      = 2007,     /* all in compare */
	};
	///---����ָ��
	enum prop_item
	{
		CHANGE_CARD = 3001,  /* change card */
		FORBIDDEN_CARD = 3002, /* forbidden compare card */
		DOUBLE_CARD_FOUR = 3003,    /* compare four multiple bet card */
		DOUBLE_CARD_SIX = 3004,    /* compare six multiple bet card */
		DOUBLE_CARD_EIGHT = 3005,    /* compare eight multiple bet card */
	};

	///---���������ָ��(�������ׯ��Э����+1000)
	enum server_command
	{
		SERVER_LOGIN_SUCC_UC       	 = 5000,
		SERVER_LOGIN_SUCC_BC       	 = 5001,
		SERVER_LOGIN_ERR_UC          = 5002,
		SERVER_REBIND_UC			 = 5003,
		SERVER_LOGOUT_SUCC_BC		 = 5004,
		SERVER_LOGOUT_ERR_UC		 = 5005,
		SERVER_TABLE_INFO_UC		 = 5006,
		SERVER_READY_SUCC_BC		 = 5007,
		SERVER_READY_ERR_UC			 = 5008,
		SERVER_GAME_START_BC		 = 5009,
		SERVER_NEXT_BET_BC			 = 5010,
		SERVER_BET_SUCC_BC			 = 5011,
		SERVER_BET_SUCC_UC			 = 5012,
		SERVER_BET_ERR_UC			 = 5013,
		SERVER_GAME_END_BC			 = 5014,
		SERVER_GAME_PREREADY_BC		 = 5015,
		SERVER_CHAT_BC				 = 5016,
		SERVER_FACE_BC				 = 5017,
		SERVER_ROBOT_SER_UC          = 5018,
		SERVER_ROBOT_NEED_UC         = 5019,
		SERVER_UPDATE_INFO_BC        = 5020,
		SERVER_EMOTION_BC            = 5021,
		SERVER_PROP_SUCC_UC          = 5022,
		SERVER_PROP_SUCC_BC          = 5023,
		SERVER_PROP_ERR_UC           = 5024,

		SERVER_GROUP_CARD_BC         = 5026,  // �㲥����
		SERVER_GROUP_CARD_UC         = 5027,  // ����Ӧ��
		SERVER_GAME_WAIT_BC          = 5028,  // �㲥��Ϸ�ȴ�  
		SERVER_GAME_READY_BC         = 5029,  // �㲥��Ϸ׼��
		SERVER_MAKE_BANKER_BC        = 5030,  // �㲥ׯ��

		SERVER_FIRST_CARD_UC         = 5032,  // �״η���
		SERVER_COMPARE_CARD_BC       = 5033,  // �㲥����

		SERVER_GRAB_BANKER_BC        = 5034,  // �㲥��ׯ
		SERVER_DOUBLE_UC             = 5035,  // �ӱ�Ӧ��
		SERVER_DOUBLE_BC             = 5036,  // �㲥�ӱ�
		SERVER_SECOND_CARD_UC        = 5037,  // ���η���
		SERVER_GRAB_BANKER_UC        = 5038,  // ��ׯӦ��

	};

}



#endif
