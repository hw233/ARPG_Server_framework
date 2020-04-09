/****************************************************************************
 *
 * General Object Type File
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

#ifndef _PLAYER_H
#define _PLAYER_H

#include "Object.h"

class GameSession;
class Partner;
typedef map<uint64, Partner*> PlayerPartnerMap;

class Player: public Object
{
public:
	Player ();
	~Player ();

public:
    /**
    * Init Player�����ʼ�������������ݳ�Ա���������Ӧ�ĳ�ʼ������
    * @param guid: ��ҽ�ɫid
    */
	void Init(GameSession *session, uint64 guid, const uint32 fieldCount);

	// ���ֽ����ж�ȡ�����������
	// bool LoadFromByteBuffer(uint32 platformServerID, uint64 playerGuid, ByteBuffer &buffer);

    /**
    * Term ���������Ա�������ã����������ݳ�Ա���������Ӧ���������
    */
    void Term();

    /** 
    * Update ��ʱ���²���
    * @param time: ���θ��¼�����ϴβ�����ʱ����
    */
    void Update(uint32 time);

	void SyncUpdateDataMasterServer();							// ͬ���������ݵ������
	void ResetSyncObjectDatas();							    // ����ͬ����������

public:
    /************************************************************************/
    /* Player�����Ӧ�Ŀͻ��˻Ự��������ü���ȡ�ӿ�                       */
    /************************************************************************/
    inline GameSession *GetSession() const { return m_session; }
    /************************************************************************/
    /* ��ɫid�����ֵȻ�ȡ�ӿ�                                               */
    /************************************************************************/
    inline const std::string &strGetUtf8NameString() const { return m_NickName; }
    virtual const char* szGetUtf8Name() const { return m_NickName.c_str(); }
    inline void SetPlayerTransferStatus(uint8 pStatus) { m_transfer_status = pStatus; }
    inline uint8 GetPlayerTransferStatus() const { return m_transfer_status; }
	inline uint32 GetPlatformID() { return GetUInt32Value(PLAYER_FIELD_PLATFORM); }
	
	inline pbc_wmessage* GetObjCreationBuffer() { return m_pbcCreationDataBuffer; }
	inline pbc_wmessage* GetObjUpdateBuffer() { return m_pbcPropUpdateDataBuffer; }
	inline tagLastMapData* GetLastMapData() { return &m_lastMapData; }
	inline uint32 GetLevel() { return GetUInt32Value(UNIT_FIELD_LEVEL); }
	inline uint32 GetCareer() { return GetUInt32Value(PLAYER_FIELD_CAREER); }

public:
	string strGetGbkNameString();
    /************************************************************************/
    /* ��ɫ��Ϸ״̬��ؽӿ�                                                 */
    /************************************************************************/
    uint32 GetGameStates();                                 // ��ȡ��ҵ�ǰ��Ϸ״̬
   
    /************************************************************************/
    /* ��ͼ��ؽӿ�                                                         */
    /************************************************************************/
    //override
    virtual void AddToWorld();                                              // ���뵽Ԥ��ĵ�ͼ��
	virtual MapMgr* OnObjectGetInstanceFailed();							// ������ͼ��ȡ֮ǰ��ȡ������mapmgr
    virtual void OnPushToWorld();                                           // �����ͼʱ�Ĵ���
    virtual void RemoveFromWorld();                                         // �뿪��ǰ��ͼ
    
    /**
    * SafeTeleport ���͵�ĳ��ͼ��ĳ������λ��
    * @param MapID: ָ���ĵ�ͼid
    * @param InstanceID: ��ͼʵ����id����Ϊ0��Ѱ�Һ��ʵĵ�ͼ���м���
    * @param X: ����Ŀ�ĵ�x������
    * @param Y: ����Ŀ�ĵ�y������
    * Return: �Ƿ��ͳɹ�
    */
    bool SafeTeleport(uint32 MapID, uint32 InstanceID, float X, float Y);
	
	/*
	*/
	bool CanEnterThisScene(uint32 mapId);

	/*
	* ��������������Ľӿڣ�һ����ⲿ����
	*/
	bool HandleSceneTeleportRequest(uint32 destServerIndex, uint32 destMapID, FloatPoint destPos);

    /**
    * _Relocate ���͵�ĳ��ͼ��ĳ������λ��
    * @param mapid: ָ���ĵ�ͼid
    * @param v: �������Ϣ
    * @param force_new_world: �Ƿ�ǿ�ƽ����µ�ͼ���������н���ͬһ����ͼ����ʵ��
    * @param instance_id: ��ͼʵ��id
    * Return: �Ƿ��ͳɹ�
    */
    bool _Relocate(uint32 mapid, const FloatPoint &v, bool force_new_world, uint32 instance_id);

    void EjectFromInstance();											// �ӵ�ǰ�����б�����

	// ������
	Partner* GetPartnerByProto(uint32 protoID);
	Partner* GetPartnerByGuid(uint64 partnerGuid);

public:
	static UpdateMask m_visibleUpdateMask;                  // ���˿ɼ�����������Ϣ

	// lua�������
	static int newCppPlayerObject(lua_State *l);
	static int delCppPlayerObject(lua_State *l);

	int initData(lua_State *l);
	int getPlayerUtf8Name(lua_State *l);
	int setPlayerUtf8Name(lua_State *l);
	int getPlayerGbkName(lua_State *l);
	int removeFromWorld(lua_State *l);
	int sendPacket(lua_State *l);
	int sendServerPacket(lua_State *l);
	int setPlayerDataSyncFlag(lua_State *l);

	int releasePartner(lua_State *l);		// �ų�һ�����
	int revokePartner(lua_State *l);		// �ջ�ĳ�����

	LUA_EXPORT(Player)

private:
	virtual void OnValueFieldChange(const uint32 index);
    //���������Ҫͬ��������
	virtual void _SetCreateBits(UpdateMask *updateMask, Player* target) const;
	virtual	void _SetUpdateBits(UpdateMask *updateMask, Player* target) const;

private:
	GameSession *m_session;                                 // �ͻ���ӳ��Ựָ��
    uint32 m_gamestates;                                    // ��ӦSessionStatus�ĻỰ״̬
    std::string m_NickName;                                 // ��ɫ��
    uint8 m_transfer_status;                                // ��ǰ����״̬

	tagLastMapData m_lastMapData;							// ���һ��������ͨ����������
	PlayerPartnerMap m_Partners;							// ��ҵĻ�� ���ж��,map<���guid,ʵ��>

    //��Χobject������Ϣ������͵���ؽӿ�
    bool bProcessPending;                                   // �Ƿ����ڴ����������
    bool bMoveProcessPending;                               // �Ƿ������ƶ���Ϣ�����������
	bool bRemoveProcessPending;								// �Ƿ������Ƴ���Ϣ�����������
	bool bCreateProcessPending;								// �Ƿ����ڴ�����Ϣ�����������

    Mutex _bufferS;
	pbc_wmessage *m_pbcMovmentDataBuffer;				// �����ƶ�Э�����ݻ�����
	pbc_wmessage *m_pbcCreationDataBuffer;				// ���󴴽�Э�����ݻ�����
	pbc_wmessage *m_pbcRemoveDataBuffer;				// �����Ƴ�Э�����ݻ�����
	uint32 m_pbcRemoveDataCounter;						// �����Ƴ�����
	pbc_wmessage *m_pbcPropUpdateDataBuffer;			// �������Ը���Э�����ݻ�����

public:
	// ��������ͬ��
	uint32 m_nSyncDataLoopCounter;			// ѭ��������������ÿ��update��ͬ�����ݣ�
	bool m_bNeedSyncPlayerData;				// ��Ҫͬ���������
	
	virtual void BuildCreationDataBlockForPlayer( Player* target );
	virtual bool BuildValuesUpdateBlockForPlayer( Player *target );
	virtual void BuildRemoveDataBlockForPlayer( Player *target );
	virtual void ClearUpdateMask();

    // ��ɫ����ͬ�����ƶ���ͬ����ؽӿ�
	void PushSingleObjectPropUpdateData();
	void PushCreationData();
	void PushOutOfRange(const uint64 &guid, uint32 objtype);
	void PushMovementData(uint64 objguid, float srcX, float srcY, float srcHeight, float destX, float destY, float destHeight, uint8 mode);
	
	void ProcessObjectPropUpdates();			// �������Ը��°�
	void ProcessObjectRemoveUpdates();			// ���Ͷ����Ƴ���
	void ProcessObjectMovmentUpdates();			// ���Ͷ����ƶ���
	void ProcessObjectCreationUpdates();		// ���Ͷ��󴴽���
	
	void ClearAllPendingUpdates();
};

#endif

