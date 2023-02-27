#ifndef  __OBJECT_POOL__H_
#define  __OBJECT_POOL__H_

#define SECURE_MODE					1

#include <new.h>

namespace Jay
{
	/**
	* @file		ObjectPool.h
	* @brief	������Ʈ �޸� Ǯ Ŭ����(������Ʈ Ǯ / ��������Ʈ)
	* @details	Ư�� �����͸�(����ü, Ŭ����, ����) ������ �Ҵ� �� ��������.
	* @usage	Jay::ObjectPool<T> MemPool(300, false);
				T *pData = MemPool.Alloc();
				pData ���
				MemPool.Free(pData);
	* @author   ������
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
		* @brief	������, �Ҹ���
		* @details
		* @param	int(�ʱ� �� ����), bool(Alloc �� ������ / Free �� �ı��� ȣ�� ����)
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
		* @brief	�� �ϳ��� �Ҵ�޴´�.
		* @details
		* @param	void
		* @return	T*(������ �� ������)
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
		* @brief	������̴� ���� �����Ѵ�.
		* @details
		* @param	T*(������ �� ������)
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
		* @brief	���� Ȯ�� �� �� ������ ��´�.(�޸� Ǯ ������ ��ü ����)
		* @details
		* @param	void
		* @return	int(�޸� Ǯ ���� ��ü �� ����)
		**/
		inline int GetCapacityCount(void)
		{
			return _capacity;
		}

		/**
		* @brief	���� ������� �� ������ ��´�.
		* @details
		* @param	void
		* @return	int(������� �� ����)
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
