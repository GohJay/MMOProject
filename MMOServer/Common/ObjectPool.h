#ifndef  __OBJECT_POOL__H_
#define  __OBJECT_POOL__H_

#define SECURE_MODE					1

#include <new.h>

namespace Jay
{
	/**
	* @file		ObjectPool.h
	* @brief	오브젝트 메모리 풀 클래스(오브젝트 풀 / 프리리스트)
	* @details	특정 데이터를(구조체, 클래스, 변수) 일정량 할당 후 나눠쓴다.
	* @usage	Jay::ObjectPool<T> MemPool(300, false);
				T *pData = MemPool.Alloc();
				pData 사용
				MemPool.Free(pData);
	* @author   고재현
	* @date		2022-02-26
	* @version  1.0.1
	**/
	template <typename T>
	class ObjectPool
	{
	private:
		struct NODE
		{
			T data;
#if SECURE_MODE
			size_t signature;
#endif
			NODE* prev;
		};
	public:
		/**
		* @brief	생성자, 소멸자
		* @details
		* @param	int(초기 블럭 개수), bool(Alloc 시 생성자 / Free 시 파괴자 호출 여부)
		* @return
		**/
		ObjectPool(int blockNum, bool placementNew = false)
			: _top(nullptr), _placementNew(placementNew), _capacity(0), _useCount(0)
		{
			while (blockNum > 0)
			{
				NODE* node = (NODE*)malloc(sizeof(NODE));
				node->prev = _top;
#if SECURE_MODE
				node->signature = (size_t)this;
				_capacity++;
#endif
				if (!_placementNew)
					new(&node->data) T();

				_top = node;

				blockNum--;
			}
		}
		~ObjectPool()
		{
			NODE* prev;
			while (_top)
			{
				if (!_placementNew)
					_top->data.~T();

				prev = _top->prev;
				free(_top);
				_top = prev;
#if SECURE_MODE
				_capacity--;
#endif
			}
		}
	public:
		/**
		* @brief	블럭 하나를 할당받는다.
		* @details
		* @param	void
		* @return	T*(데이터 블럭 포인터)
		**/
		T* Alloc(void)
		{
			NODE* node;

			if (_top == nullptr)
			{
				node = (NODE*)malloc(sizeof(NODE));
				new(&node->data) T();
#if SECURE_MODE
				node->signature = (size_t)this;
				_capacity++;
				_useCount++;
#endif
				return &node->data;
			}

			node = _top;
			_top = node->prev;

			if (_placementNew)
				new(&node->data) T();

#if SECURE_MODE
			_useCount++;
#endif
			return &node->data;
		}

		/**
		* @brief	사용중이던 블럭을 해제한다.
		* @details
		* @param	T*(데이터 블럭 포인터)
		* @return	void
		**/
		void Free(T* data) throw()
		{
			NODE* node = (NODE*)data;
#if SECURE_MODE
			if (node->signature != (size_t)this)
				throw;
#endif
			if (_placementNew)
				node->data.~T();

			node->prev = _top;
			_top = node;

#if SECURE_MODE
			_useCount--;
#endif
		}

		/**
		* @brief	현재 확보 된 블럭 개수를 얻는다.(메모리 풀 내부의 전체 개수)
		* @details
		* @param	void
		* @return	int(메모리 풀 내부 전체 블럭 개수)
		**/
		inline int GetCapacityCount(void)
		{
			return _capacity;
		}

		/**
		* @brief	현재 사용중인 블럭 개수를 얻는다.
		* @details
		* @param	void
		* @return	int(사용중인 블럭 개수)
		**/
		inline int GetUseCount(void)
		{
			return _useCount;
		}
	private:
		NODE* _top;
		bool _placementNew;
		int _capacity;
		int _useCount;
	};
}
#endif
