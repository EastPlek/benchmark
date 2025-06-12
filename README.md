# use_ptr — A Usage-Based Smart Pointer
---
by EastPlek 
(Code First Created : 2025. 06. 08 | README First Created : 2025. 06. 10)
(Renamed AegisPtr -> use_ptr at 2025. 06. 12.)

## Use-ref GC 란?
대부분의 메모리 관련 버그는 **UAF (Use-After-Free)**에서 옵니다.
UAF를 방지하는 방법에 뭐가 있을까 고민하던 와중...
> UAF가 사용을 전제로 한다면, 그냥 사용하고 있으면 지우지 않으면 되지 않을까?
라는 생각에서 착안을 해서,
**Use-ref GC**라는 새로운 메모리 모델을 제안합니다.

Use-ref GC는 shared_ptr와 같이 std::atomic<uint64_t>를 사용합니다.
다만, **소유권**을 추적하는 대신, **사용권**을 추적합니다.

이는 전환점이 될 수 있습니다.
| 메모리 모델 | 세는 것 |
|--------------|-----------|
| shared_ptr   | 누가 *소유*하는가 |
| use_ptr      | 누가 *사용*하는가 |

즉,
> *Use-ref GC는 아무도 사용하지 않으면 메모리를 수거합니다.*

## 순환참조?
예를 들어 다음과 같은 코드가 있다고 가정합시다.
```
#include <iostream>
#include <memory>

struct C; // 먼저 선언
struct B; // 먼저 선언

struct A {
	A(int val) : val{val} {}
	virtual ~A() { }
	virtual void say() { std::cout << "A" << '\n'; }
	int val;
	std::shared_ptr<B> ptr{};
};

struct B {
	B(int val) : val{ val } {}
	void say() { std::cout << "B" << '\n'; }
	int val;
	std::shared_ptr<C> ptr{};
};

struct C : A{
	C(int val) : A{ val }, val{ val } {}
	~C() {}
	void say() override { std::cout << "C" << '\n'; }
	int val;
	std::shared_ptr<A> ptr{};
};

```
이런 상황에서, std::shared_ptr는 A <- B <- C <- A와 같은 형태로 참조가 된다고 가정할 수 있습니다.
```
std::shared_ptr<A> a_shared = std::make_shared(A(1));
std::shared_ptr<B> b_shared = std::make_shared(B(2));
std::shared_ptr<C> c_shared = std::make_shared(C(3));

a_shared->ptr = b_shared; // ++ref;
b_shared->ptr = c_shared; // ++ref;
c_shared->ptr = a_shared; // ++ref;

// 현재 ref
a_shared: use_count == 1
b_shared: use_count == 1
c_shared: use_count == 1

// a_shared, b_shared, c_shared 모두 소멸
// a_shared는 c_shared가 참조
// b_shared는 a_shared가 참조
// c_shared는 b_shared가 참조
...
```
위와 같이 순환 참조가 일어나서 메모리를 해제 할 수 없고, 결국에는 누수가 발생합니다.

## use_ptr는 어떠한 방식으로 순환참조를 해결하는가?
use_ptr는 사용 카운트를 기준으로 메모리 소멸을 판단합니다. use_ptr도 shared_ptr와 비슷하게 순환 사용이 있지만,
사용자가 명시적으로 해제할 수 있고, 또한 RAII로 사용과 해제를 하는 use_guard를 사용하면 안전하고 편리하게
순환참조를 해결할 수 있습니다.
```
use_ptr<A> a(new A(1));
use_ptr<B> b(new B(2));
use_ptr<C> c(new C(3));

use_guard<A> guard1(a);
use_guard<B> guard2(b);
use_guard<C> guard3(c);

guard1->ptr.store(b);     // A → B
guard2->ptr.store(c);     // B → C
guard3->ptr.store(a);     // C → A (업캐스팅 OK)
```
guard가 use_ptr를 받는 생성자를 호출하면 a->use()를 호출하고 ~use_guard에서 a->unuse()를 해서 자동으로 관리합니다.
use_ptr의 소멸자에서는 use_count가 0인지 확인하고 GC를 실행합니다.

소멸자 호출 순서 :
use_guard<C> // use_count 1 -> 0
use_guard<B> // use_count 1 -> 0
use_guard<A> // use_count 1 -> 0
use_ptr<C> // c 삭제
use_ptr<B> // b 삭제
use_ptr<A> // a 삭제

또한, 다음과 같이 자기가 자기 자신을 참조하는 코드도 **안전하게** 실행할 수 있습니다.
```
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
```

## What is Use-ref GC?
Most of Memory bugs in C++ comes from **UAF (Use-After-Free).**
So, I thought :
> if "UAF requires *use*, what if we _just don't free_ when something is still **using** it?

And, I propose :
**Use-ref GC**.

It uses `std::atomic<uint64_t>` like shared_ptr,
but instead of **tracking ownership**, it tracks **usage**.

This could be paradigm shift :
| Memory Model | Counts... |
|--------------|-----------|
| shared_ptr   | who *owns* it |
| use_ptr      | who *uses* it |

Shortly,
> *Use-ref GC frees memory only when nobody using it.*
