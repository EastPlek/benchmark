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

	use_guard<A> guard(a);
	use_guard<B> guard2(b);
	use_guard<C> guard3(c);

	guard->ptr.store(b);     // A → B
	guard2->ptr.store(c);     // B → C
	guard3->ptr.store(a);     // C → A (업캐스팅 OK)

	// 다형성도 가능
	guard->say();           // "C"
	guard2->say();
	guard3->say(); // "C" (A → B → C)

	// 해제
}
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	test();
	_CrtDumpMemoryLeaks();
}