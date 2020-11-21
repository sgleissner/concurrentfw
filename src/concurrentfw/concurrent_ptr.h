/*
 * concurrentfw/concurrent_ptr.h
 *
 * (C) 2017-2020 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */


#ifndef CONCURRENTFW_CONCURRENT_PTR_H_
#define CONCURRENTFW_CONCURRENT_PTR_H_

#include <cstdint>

#include "aba_wrapper.h"

namespace ConcurrentFW
{

template<typename T>
class Concurrent_Ptr
{
public:
	Concurrent_Ptr(T* init = nullptr)
	: aba_ptr(init)
	{}

	void set(T* const ptr);
	T* get();

	// TODO: [[deprecated("usage of get_counter() only for testing")]]
	typename ABA_Wrapper<T*>::Counter get_counter();

	ABA_Wrapper<T*> aba_ptr;
};

template<typename T>
void Concurrent_Ptr<T>::set(T* const ptr)
{
	// lamdas in function pointers can be inlined completely, but must not have captures
	bool (*inlined_modify_func)(T* const &, T*&, T* const) =	// implicit conversion of lambda to function pointer
			[](T* const & /* ptr_cached */, T*& ptr_modify, T* const ptr_init) -> bool
			{
				ptr_modify = ptr_init;
				return true;
			};

	aba_ptr.modify(inlined_modify_func, ptr);	// will be completely inlined including lambda

/*
 *	compiles to (GCC 10.x x86_64):
 *
 *	0000000000000000 <_ZN12ConcurrentFW14Concurrent_PtrItE3setEPt>:
 *	   0:   53                      push   %rbx					rcx already contains unknown value for counter
 *	   1:   48 89 f3                mov    %rsi,%rbx			rbx contains desired pointer
 *	   4:   48 89 d8                mov    %rbx,%rax			rbx:rcx contains desired data
 *	   7:   48 89 ca                mov    %rcx,%rdx			rax:rdx now contains expected data = copy of rbx:rcx
 *	   a:   f0 48 0f c7 0f          lock cmpxchg16b (%rdi)		rax:rdx loads atomic value, rdx contains now known counter
 *	   f:   48 8d 4a 01             lea    0x1(%rdx),%rcx		increment counter (from .modify())
 *	  13:   f0 48 0f c7 0f          lock cmpxchg16b (%rdi)		try to write
 *	  18:   75 f5                   jne    f <_ZN12ConcurrentFW14Concurrent_PtrItE3setEPt+0xf>		loop if failed
 *	  1a:   5b                      pop    %rbx
 *	  1b:   c3                      retq
 */
}

template<typename T>
T* Concurrent_Ptr<T>::get()
{
	return aba_ptr.get();
}

template<typename T>
typename ABA_Wrapper<T*>::Counter Concurrent_Ptr<T>::get_counter()
{
	return aba_ptr.get_counter();
}

} // namespace ConcurrentFW

#endif /* CONCURRENTFW_CONCURRENT_PTR_H_ */

