#ifndef CMANAGERBASEPTR_HPP
#define CMANAGERBASEPTR_HPP
#include <atomic>
#include <cstdint>


namespace BluBooster::Memory {

	template <typename T>
	struct ExportHandle {
		ExportHandle(T* ptr, uint64_t count) : ptr{ ptr }, useCount{ count } {}
		T* ptr;
		uint64_t useCount;
	};
	template <typename T>
	class use_ptr {
	public:
		use_ptr() : m_ptr{ nullptr }, m_useCount{ 0 }, isUsing{ false } {}
		use_ptr(T*&& ptr) : m_ptr{ ptr}, m_useCount{ 0 }, isUsing{ false } {}
		~use_ptr() {
			if (m_useCount.fetch_sub(1, std::memory_order_acq_rel) == 0 && m_ptr)
			{
				delete m_ptr;
				m_ptr = nullptr;
			}
		}

		use_ptr(const use_ptr<T>& other) = delete;
		use_ptr<T>& operator=(const use_ptr<T>& other) = delete;
		use_ptr(use_ptr<T>&& other) {
			claim(other.abandon());
		}
		use_ptr&& operator= (use_ptr<T>&& other) = delete;

		void claim(ExportHandle<T>& handle) {
			if (m_ptr != nullptr|| m_useCount.load(std::memory_order_acquire)) return;
			m_ptr = handle.ptr;
			m_useCount.store(handle.useCount, std::memory_order_release);
			isUsing = false;
		}
		ExportHandle<T> abandon() {
			T* raw = m_ptr;
			uint64_t useCount = m_useCount.load(std::memory_order_acquire);
			ExportHandle<T> handle{ raw,useCount };
			m_ptr = nullptr;
			return handle;
		}

		T* operator->() {
			if (!isUsing) {
				m_useCount.fetch_add(1, std::memory_order_acq_rel);
				isUsing = true;
			}
			return m_ptr;
		}
		T* use() {
			return operator->();
		}
		void operator--() {
			if (isUsing) {
				m_useCount.fetch_sub(1, std::memory_order_acq_rel);
				isUsing = false;
			}
		}
		void unuse() {
			operator--();
		}

		void raw_store(T* other) {
			m_ptr = other;
		}

		template <typename U>
		void store(use_ptr<U>& other) {
			if(std::is_base_of_v<U,T>){
				m_useCount.fetch_add(1,std::memory_order_acq_rel);
			}
			m_ptr = other.m_ptr;
		}
	private:
		T* m_ptr;
		std::atomic<uint64_t> m_useCount;
		bool isUsing;
	};

}

#endif // CMANAGERBASEPTR_HPP
