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

#ifndef _OBJECTMGR_H
#define _OBJECTMGR_H

/************************************************************************/
/* ��Ҫ���ڴ�����壨��Ʒ������ȣ���صľ�̬��                         */
/************************************************************************/

struct tagMonsterProto;

class ObjectMgr : public Singleton < ObjectMgr >
{
public:
	ObjectMgr();
	~ObjectMgr();

public:
	// Serialization
	int Init(void);
	int LoadBaseData(void);
	void LoadMonsterProto();
    void Reload();

public:
	uint32 GenerateLowGuid(uint32 guidhigh);

	void ResetDailies();						// �����ճ��¼�
	void ResetTwoHours();						// ����2Сʱ�¼�ѭ����������

	// �������
	tagMonsterProto* GetMonsterProto(uint32 entry);

protected:
	Mutex m_guidGenMutex;

	std::map<uint32, tagMonsterProto*> m_MonsterProtoList;

	std::map<uint32, uint32> m_findPathFrequency;
	uint32 m_crFindPathFrequencyCounter;									// ����Ѱ·Ƶ��ͳ��
	uint32 m_crFindPathTotalCostTime;										// ����Ѱ·������ʱ��
};

#define objmgr ObjectMgr::GetSingleton()

#endif
