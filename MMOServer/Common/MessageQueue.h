#ifndef __MESSAGE_QUEUE__H_
#define __MESSAGE_QUEUE__H_
#include "Lock.h"
#include <queue>

namespace Jay
{
	/**
	* @file		MessageQueue.h
	* @brief	Message Queue Template Class
	* @details	멀티스레드 환경에서의 데이터 통신을 위한 메시지 큐 클래스
	* @author	고재현
	* @date		2023-03-01
	* @version	1.0.0
	**/
	template<typename T>
	class MessageQueue
	{
	public:
		MessageQueue()
		{
			_mainMessageQ = new std::queue<T>();
			_subMessageQ = new std::queue<T>();
		}
		~MessageQueue()
		{
			delete _mainMessageQ;
			delete _subMessageQ;
		}
	public:
		void Enqueue(T data)
		{
			_messageLock.Lock();
			_subMessageQ->push(data);
			_messageLock.UnLock();
		}
		void Dequeue(T& data)
		{
			data = _mainMessageQ->front();
			_mainMessageQ->pop();
		}
		void Flip()
		{
			_messageLock.Lock();
			std::swap(_mainMessageQ, _subMessageQ);
			_messageLock.UnLock();
		}
		bool empty()
		{
			return _mainMessageQ->empty();
		}
		int size()
		{
			return _mainMessageQ->size();
		}
	private:
		std::queue<T>* _mainMessageQ;
		std::queue<T>* _subMessageQ;
		SpinLock _messageLock;
	};
}

#endif
