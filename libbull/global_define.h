#ifndef _GLOBAL_DEFINE_
#define _GLOBAL_DEFINE_


//�������Ͷ���
typedef          char        _tint8;     //�з��� 1 �ֽ�
typedef unsigned char        _uint8;     //�޷��� 1 �ֽ�
typedef short                _tint16;    //�з��� 2 �ֽ�
typedef unsigned short       _uint16;    //�޷��� 2 �ֽ�
typedef int                  _tint32;    //�з��� 4 �ֽ�
typedef unsigned int         _uint32;    //�޷��� 4 �ֽ�
typedef long long            _tint64;    //�з��� 8 �ֽ�
typedef unsigned long long   _uint64;    //�޷��� 8 �ֽ�
typedef unsigned int         socket_id;  //ͨ�ű�ʶ   4 �ֽ�
typedef _uint8               _svrtype;   //���������� 1 �ֽ�
typedef _uint64              _svrid;     //������ID   8 �ֽ�


#define	INVALID_BANKER			0xFF				//��Чׯ��ID
#define	INVALID_DOUBLE			0					//�޲����ӱ�
#define	INVALID_SECOND_DOUBLE	0xFF				//�޲������μӱ�
#define	INVALID_SWITCH			0xFF				//�޲�������
#define	INVALID_GER_BANK		0xFF				//�޲�����ׯ
#define	INVALID_USER_ID			0					//��Ч�û�ID

#define	MAX_CHAIR_COUNT			5					//���������
#define	GAME_PLAYER_COUNT		5					//��Ϸ����
#define	MAX_DOUBLE_CHOICE		4					//���ӱ�ѡ��
#define	MAX_CARD_TYPE			13					//���������
#define	MAX_CHOICE_CARD			3					//���ѡ����

#define	MAX_HAND_CARD_COUNT		5					//8�����������
#define	MAX_TABLE_CARD_COUNT	5					//�����������Ŀ
#define	MAX_SWITCH_CARD_COUNT	5					//�������
#define	MAX_CHOICE_CARD			3					//ѡ����Ŀ

#define	MAX_BAIREN_BET_NUM		4					//���������עѡ��
#define	MAX_GET_BANKER_NUM		10					//��ׯ�б�
#define	MAX_BAIREN_CARD_COUNT	5					//������
#define	MAX_RECORD_NUM			20					//��ע��¼



//��λ״̬
enum enChairStatus   
{
	STATUS_NULL    = 0,     //��λ	
	STATUS_LOOK_ON = 1,     //�Թ�(����)
	STATUS_IN_GAME = 2,     //��Ϸ��
	STATUS_STAND_UP= 3,     //վ���˳�
};

enum enRole
{
	ROLE_NULL=0,    //�м�
	ROLE_BANKER=1,  //ׯ��

};

//��������
enum enEndKind
{
	KIND_NULL = 0,  //��
	KIND_WIN  = 1,  //ʤ
	KING_LOSE = 2,  //��
};


#endif



