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

## 순환참조?
기존 std::shared_ptr는 A -> B | B -> A 모델에서 순환 참조하여 메모리 leak이 발생하는 일이 생깁니다.
Use-ref GC는 사용 기반 카운트 참조로 사용자가 Use()하고 Unuse()만 잘 설정하면 순환참조가 발생하지 않습니다.
