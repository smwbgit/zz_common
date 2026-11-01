#pragma once

#pragma region 能在类里使用的消息映射，不需要DECLARE_MESSAGE_MAP宏
#define utils_begin_message_map(ThisClass_T) \
	protected: \
		PTM_WARNING_DISABLE \
		virtual const AFX_MSGMAP* GetMessageMap() const override \
		{ \
			return GetThisMessageMap(); \
		} \
		static const AFX_MSGMAP* PASCAL GetThisMessageMap() \
		{ \
			using ThisClass = ThisClass_T;				   \
			static const AFX_MSGMAP_ENTRY _messageEntries[] =  \
			{



#define utils_end_message_map() \
				{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 } \
			}; \
			static const AFX_MSGMAP messageMap = \
			{ &__super::GetThisMessageMap, &_messageEntries[0] }; \
			return &messageMap; \
		}								  \
		PTM_WARNING_RESTORE


#pragma endregion