# use_ptr — A Usage-Based Smart Pointer
---
by EastPlek 
(Code First Created : 2025. 06. 08 | README First Created : 2025. 06. 10)
(Renamed AegisPtr -> use_ptr at 2025. 06. 12.)

Korean Version
(To read english version, scroll down further.)
---
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
이런 상황에서, std::shared_ptr는 A -> B -> C -> A와 같은 형태로 참조가 된다고 가정할 수 있습니다.
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

또한 C가 A를 상속하는 상속 구조가 되면 일반적인 use_ptr<T>로는 C 삭제 A 삭제 B 삭제 A double free가 됩니다.
따라서 std::is_base_of_v<T,U>를 통해 double free가 되지 않도록 ref_count를 1 증가시켜 임시적으로 해결했습니다.
(어디까지나 임시적인 방책이고, 사실, 이러한 다형성 순환 구조는 **절대로, 앞으로도 설계되선 안되며**, UB를 일으킬 수 있습니다.)

English Version
---

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

## Cyclic Reference?
What if we write code like the following?
```
#include <iostream>
#include <memory>

struct C; // forward decl
struct B; 

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
In this situation, std::shared_ptr forms a cyclic reference like A -> B -> C -> A.
```
std::shared_ptr<A> a_shared = std::make_shared(A(1));
std::shared_ptr<B> b_shared = std::make_shared(B(2));
std::shared_ptr<C> c_shared = std::make_shared(C(3));

a_shared->ptr = b_shared; // ++ref;
b_shared->ptr = c_shared; // ++ref;
c_shared->ptr = a_shared; // ++ref;

// currnent use count
a_shared: use_count == 1
b_shared: use_count == 1
c_shared: use_count == 1

// a_shared, b_shared, c_shared destructs.
// c_shared holds a reference to a_shared.
// a_shared holds a reference to b_shared.
// b_shared holds a reference to c_shared.
...
```
cyclic reference occurs, resulting in a memory leak.

## How use_ptr Resolves Cyclic References
use_ptr determines garbage collection timing based on its usage count, not ownership.
While `use_ptr` can still participate in **cyclic usage graphs**, the user can explicitly release usage.
With `use_guard`, which automatically increments and decrements the usage count via RAII,  
**handling cyclic references becomes trivial**.

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
When a `use_guard` is constructed with a `use_ptr<T>&`,  
it calls `a->use()` to indicate that the object is in use.

When the `use_guard` is destructed,  
it automatically calls `a->unuse()` to mark that the object is no longer in use.

Finally, in `use_ptr`’s destructor,  
it checks whether `use_count == 0`, and if so, **frees the object** (garbage collection).

Destructor order :
use_guard<C> // use_count 1 -> 0
use_guard<B> // use_count 1 -> 0
use_guard<A> // use_count 1 -> 0
use_ptr<C> // frees c 
use_ptr<B> // frees b
use_ptr<A> // frees a

Additionally, use_ptr can safely execute code that references itself.
```
use_ptr<A> a(new A(1));
use_ptr<B> b(new B(2));
use_ptr<C> c(new C(3));

use_guard<A> guard1(a);
use_guard<B> guard2(b);
use_guard<C> guard3(c);

guard1->ptr.store(b);     // A → B
guard2->ptr.store(c);     // B → C
guard3->ptr.store(a);     // C → A (Upcasting)
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

Additionally, if C inherits from A, using a plain use_ptr<T> may cause a double free in certain circular structures.
For example, if C points to itself through a base class pointer (A*), it can result in:

    C is deleted

    A (base part of C) is deleted again

    B is deleted

    then A is deleted once more → double free

To avoid this, we temporarily increase the reference count by 1 when std::is_base_of_v<T, U> is true,
detecting an upcast during store() and adjusting usage accordingly.

However, this is just a workaround, not a solution.
Such polymorphic circular references should never be designed this way,
and may lead to undefined behavior (UB).
