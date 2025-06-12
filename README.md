# AegisPtr (Temp) vs folly::hazptr
---
by EastPlek (Code First Created : 2025. 06. 08 | README First Created : 2025. 06. 10)
(AegisPtr has changed its name to use_ptr at 2025. 06. 12.)

## AegisPtr란?
AegisPtr는 기존의 thread간의 공유되는 자원의 소멸 시점을 Use-ref GC를 기반으로 수거합니다.

## Use-ref GC (Codename : 허접❤️ GC)란?
UAF가 발생하기 위해서는 사용이라는 조건이 따라옵니다.
그러면 누가 사용중이면 GC를 안하면 되는 것 아닌가? 라는 결론에 도달했습니다.
Use-ref GC도 std::shared_ptr와 같이 std::atomic<정수> ref-count라는 것을 사용합니다.
다만, shared_ptr는 누가 소유하고 있는가를 기준으로 카운트하고,
Use-ref GC는 누가 사용하고 있는가를 기준으로 카운트한다는 결정적인 차이가 존재합니다.

Use-ref GC는 다음과 같이 동작합니다.
AegisPtr<T> ptr를 생성 : AegisPtr<T> ptr내부에는 사용 카운트를 세는 std::atomic<uint64_t> 변수가 있습니다.
AegisPtr<T>의 참조(나중에는 재사용성을 위해 포인터로 바꿀 예정입니다.)를 AegisGuard<T> guard(ptr)가 받습니다.
guard-> 또는 guard.use()를 호출해서 카운트를 AegisPtr<T>에 fetch_add(1)를 합니다. (만약 use()가 두 번 이상 호출할 시 fetch_add는 무시합니다.)
!guard 또는 guard.unuse()를 호출해서 카운트를 AegisPtr<T>에 단 한 번만 fetch_sub(1)를 합니다.
~guard 또는 guard.try_delete()로 사용 카운트가 0이라면 early GC를 시도할 수 있습니다.
-guard 또는 guard.clear()로 unuse와 try_delete()를 동시에 할 수 있습니다.

guard는 unuse()가 호출되지 않았다면 RAII로 자동으로 Unuse()를 합니다.

AegisPtr<T>는 사용 카운트가 0이고 earlyGC가 되지 않았다면 자동으로 삭제합니다.

## 순환참조?
기존 std::shared_ptr는 A -> B | B -> A 모델에서 순환 참조하여 메모리 leak이 발생하는 일이 생깁니다.
Use-ref GC는 사용 기반 카운트 참조로 사용자가 Use()하고 Unuse()만 잘 설정하면 순환참조가 발생하지 않습니다.
