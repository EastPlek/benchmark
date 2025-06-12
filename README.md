# use_ptr (Temp)
---
by EastPlek (Code First Created : 2025. 06. 08 | README First Created : 2025. 06. 10)
(AegisPtr has changed its name to use_ptr at 2025. 06. 12.)

## Use-ref GC 란?
UAF가 발생하기 위해서는 사용이라는 조건이 따라옵니다.
그러면 누가 사용중이면 GC를 안하면 되는 것 아닌가? 라는 결론에 도달했습니다.
Use-ref GC도 std::shared_ptr와 같이 std::atomic<정수> ref-count라는 것을 사용합니다.
다만, shared_ptr는 누가 소유하고 있는가를 기준으로 카운트하고,
Use-ref GC는 누가 사용하고 있는가를 기준으로 카운트한다는 결정적인 차이가 존재합니다.

## 순환참조?
기존 std::shared_ptr는 A -> B | B -> A 모델에서 순환 참조하여 메모리 leak이 발생하는 일이 생깁니다.
Use-ref GC는 사용 기반 카운트 참조로 사용자가 Use()하고 Unuse()만 잘 설정하면 순환참조가 발생하지 않습니다.
