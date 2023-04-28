//
// Created by simon on 26.02.23.
//

#include <stdexcept>

#include <concurrentfw/stack.hpp>

namespace ConcurrentFW
{

void Stack::push(void* block)
{
	if (!block)
		throw std::runtime_error("nullptr not allowed as block");

	stack.modify(
		[=](void* const& stack_cached, void*& stack_modify) -> bool
		{
			*reinterpret_cast<void**>(block) = stack_cached;
			stack_modify = block;
			return true;
		}
	);	// will be completely inlined including lambda

	/*
	aarch64:
L0:	ldaxr	x2, [x0]
	str		x2, [x1]
	stlxr	w2, x1, [x0]
	tbnz	w2, #0, L0
*/
}

void* Stack::pop()
{
	void* top;	// will always be initialized in lambda
	stack.modify(
		[&](void* const& stack_cached, void*& stack_modify) -> bool
		{
			top = stack_cached;
			if (top == nullptr) [[unlikely]]
				return false;
			stack_modify = *reinterpret_cast<void**>(top);
			return true;
		}
	);
	return top;

	/*
	aarch64:
L0:	ldaxr	x0, [x1]
	cbz		x0, L1
	ldr		x3, [x0]
	stlxr	w2, x3, [x1]
	tbnz	w2, #0, L0
	ret
L1:	clrex
	ret
*/
}

}  // namespace ConcurrentFW
