# AegisPtr (Temp) vs folly::hazptr
---
by EastPlek (Code First Created : 2025. 06. 08)

## AegisPtr란?
AegisPtr는 기존의 thread간의 공유되는 자원의 소멸 시점을 RAII (스코프 기반의 자동해제) 또는, 명시적 소멸 `try_delete()`로 해제할 수 있는 구조입니다.


