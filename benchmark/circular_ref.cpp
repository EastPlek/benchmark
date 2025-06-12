#include "BluTransEntry/use_ptr.hpp"
#include <iostream>
using namespace UsePtr;

struct C; // 먼저 선언
struct B; // 먼저 선언

struct A {
	A(int val) : val{val} {}
	virtual ~A() { }
	virtual void say() { std::cout << "A" << '\n'; }
	int val;
	use_ptr<B> ptr{};
};

struct B {
	B(int val) : val{ val } {}
	void say() { std::cout << "B" << '\n'; }
	int val;
	use_ptr<C> ptr{};
};

struct C : A{
	C(int val) : A{ val }, val{ val } {}
	~C() {}
	void say() override { std::cout << "C" << '\n'; }
	int val;
	use_ptr<A> ptr{};
};

void test() {
	use_ptr<A> a(new A(1));
	use_ptr<B> b(new B(2));
	use_ptr<C> c(new C(3));

	use_guard<A> guard1(a);
	use_guard<B> guard2(b);
	use_guard<C> guard3(c);

	guard1->ptr.store(b);     // A → B
	guard2->ptr.store(c);     // B → C
	guard3->ptr.store(a);     // C → A (업캐스팅 OK)
	{
		use_guard<B> guard1_1(guard1->ptr);
		use_guard<C> guard2_1(guard2->ptr);
		use_guard<A> guard3_1(guard3->ptr);
		{
			use_guard<C> guard1_2(guard1_1->ptr);
			use_guard<A> guard2_2(guard2_1->ptr);
			use_guard<B> guard3_2(guard3_1->ptr);
			{
				use_guard<A> guard1_3(guard1_2->ptr);
				use_guard<B> guard2_3(guard2_2->ptr);
				use_guard<C> guard3_3(guard3_2->ptr);

				guard1_3->say();
				guard2_3->say();
				guard3_3->say();
			}

			guard1_2->say();
			guard2_2->say();
			guard3_2->say();
		}

		guard1_1->say();
		guard2_1->say();
		guard3_1->say();
	}

	guard1->say();  
	guard2->say();
	guard3->say(); 
}
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	test();
	_CrtDumpMemoryLeaks();
}