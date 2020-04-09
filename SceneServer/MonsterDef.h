#ifndef _MONSTER_DEF_H_
#define _MONSTER_DEF_H_

// �ƶ�����
enum MonsterMoveType
{
	Monster_move_none			= 0,
	Monster_move_random			= 1,	// ����ƶ�
	Monster_move_waypoint		= 2,	// ·���ƶ�
};

enum MonsterAttackType
{
	Monster_ack_passive		= 0,	// �������ܻ��󻹻���
	Monster_ack_active		= 1,	// ����չ������
	Monster_ack_peace		= 2,	// ��ƽ���ܻ��󲻻�����
};

enum MonsterAIState
{
	AI_IDLE            = 0,    // ����״̬
	AI_ATTACKING       = 1,    // ����״̬
	AI_FOLLOW          = 2,    // ׷��
	AI_EVADE           = 3,    // �ӱܣ��ص�ԭλ��

	AI_STATE_COUNT,
};

// ����ԭ�Ͷ���
struct tagMonsterProto
{
	uint32 m_ProtoID;						//ԭ��id
	string m_MonsterName;					//������
	string m_CreatureTitle;					//����ͷ��
	uint32 m_ResModelId;					//ģ����Դid
	uint32 m_RespawnTime;					//ˢ��ʱ��
	uint32 m_Level;							//�ȼ�
	uint32 m_Exp;							//�������þ���
	uint32 m_HP;							//����ֵ
	float  m_BaseMoveSpeed;					//�����ƶ��ٶ�(��λ:��)
	uint32 m_CreatureModelSize;				//�����ģ�ʹ�С
};

struct tagMonsterBornConfig
{
	inline uint32 GetSpawnGridPosX() { return (uint32)born_center_pos.m_fX; }
	inline uint32 GetSpawnGridPosY() { return (uint32)born_center_pos.m_fY; }

	uint32	conf_id;				// ����id
	uint32	proto_id;				// ����ԭ��id
	FloatPoint born_center_pos;		// ����λ�����ĵ�
	uint16	born_count;				// ��������
	uint16	born_direction;			// ��������
	float born_radius;
	uint8	rand_move_percentage;	// ����ƶ��ĸ���
	uint8	active_ack_percentage;	// ���������ĸ���
};

// �����������
struct tagMonsterSpwanData
{
	tagMonsterSpwanData() : config_id( 0 ), proto_id( 0 ), born_dir(0.0f), move_type( Monster_move_none ),
		spawn_count( 0 ), attack_type( Monster_ack_passive )
	{

	}
	inline void InitData(uint32 sqlID, uint32 spawnID, uint32 protoID)
	{
		config_id = sqlID;
		spawn_id = spawnID;
		proto_id = protoID;
	}
	inline uint32 GetSpawnGridPosX() { return uint32(born_pos.m_fX); }
	inline uint32 GetSpawnGridPosY() { return uint32(born_pos.m_fY); }

	uint32	config_id;			// ��Ӧ��sqlid
	uint32	spawn_id;			// ��������id��ÿ����Ψһ��
	uint32	proto_id;			// ԭ��id
	FloatPoint born_pos;		// ʵ�ʳ���λ��
	float	born_dir;			// ��λ
	uint8	move_type;			// �ƶ�����
	uint8	attack_type;		// ��������
	uint8	spawn_count;		// ��������
};

#endif
