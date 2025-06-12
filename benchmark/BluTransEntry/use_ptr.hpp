#ifndef USEPTR_HPP
#define USEPTR_HPP
#include <atomic>
#include <cstdint>


namespace UsePtr {

	template <typename T>
	struct ExportHandle {
		ExportHandle(T* ptr, uint64_t count) : ptr{ ptr }, useCount{ count } {}
		T* ptr;
		uint64_t useCount;
	};
	template <typename T>
	class use_ptr {
	public:
		use_ptr() : m_ptr{ nullptr }, m_useCount{ 0 } {}
		use_ptr(T*&& ptr) : m_ptr{ ptr}, m_useCount{ 0 } {}
		~use_ptr() {
			if (m_useCount.load(std::memory_order_acquire) == 0 && m_ptr)
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
		}
		ExportHandle<T> abandon() {
			T* raw = m_ptr;
			uint64_t useCount = m_useCount.load(std::memory_order_acquire);
			ExportHandle<T> handle{ raw,useCount };
			m_ptr = nullptr;
			return handle;
		}

		T* get() {
			return m_ptr;
		}

		void use() {
			m_useCount.fetch_add(1, std::memory_order_acq_rel);
		}
		void unuse() {
			m_useCount.fetch_sub(1, std::memory_order_acq_rel);
		}
		void raw_store(T* other) {
			m_ptr = other;
		}
		template <typename U>
		void store(use_ptr<U>& other) {
			// temporary
			if(std::is_base_of_v<T,U>){
				m_useCount.fetch_add(1,std::memory_order_acq_rel);
			}
			m_ptr = other.m_ptr;
		}
	private:
		T* m_ptr;
		std::atomic<uint64_t> m_useCount;
	};

	template <typename T>
	class use_guard {
	public:
		use_guard() = default;
		use_guard(use_ptr<T>& ptr) : m_ptr{&ptr} {
			if (m_ptr) {
				m_ptr->use();
				isUsing = true;
			}
		}
		~use_guard() {
			operator--();
		}
		void attach(use_ptr<T>& ptr) {
			if (m_ptr) {
				m_ptr->unuse();
				isUsing = false;
			}
			m_ptr = &ptr;
			if (m_ptr) {
				m_ptr->use();
				isUsing = true;
			}
		}
		T* operator-> () const {
			return m_ptr->get();
		}
		T* get() const {
			return operator->();
		}
		T& operator* () const {
			return *(m_ptr->get());
		}
		void operator-- ()  {
			if (isUsing && m_ptr) {
				m_ptr->unuse();
				isUsing = false;
			}
		}
		void clear() const {
			operator--();
		}
		ExportHandle<T> abandon() {
			return m_ptr->abandon();
		}
	private:
		use_ptr<T>* m_ptr{nullptr};
		bool isUsing{ false };
	};

}

#endif // USEPTR_HPP
