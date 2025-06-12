# AegisPtr (Temp) vs folly::hazptr
---
by EastPlek (Code First Created : 2025. 06. 08 | README First Created : 2025. 06. 10)

## AegisPtr란?
AegisPtr는 기존의 thread간의 공유되는 자원의 소멸 시점을 Use-ref GC를 기반으로 수거합니다.

## Use-ref GC (Codename : 허접❤️ GC)란?
UAF가 발생하기 위해서는 사용이라는 조건이 따라옵니다.
그러면 누가 사용중이면 GC를 안하면 되는 것 아닌가? 라는 결론에 도달했습니다.
Use-ref GC도 std::shared_ptr와 같이 std::atomic<정수> ref-count라는 것을 사용합니다.
다만, shared_ptr는 누가 소유하고 있는가를 기준으로 카운트하고,
Use-ref GC는 누가 사용하고 있는가를 기준으로 카운트한다는 결정적인 차이가 존재합니다.
