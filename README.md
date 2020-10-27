## Simplified std::vector implementation

This is a simplifiend std::vector implementation created just for practice.
Currently, it doesn't use allocators and there is no vector\<bool\> specialization.

Strong exception guarantee is ensured where possible, but ultimately depends on T's move constructor's guarantee and the existence of T's copy constructor.
