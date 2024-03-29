/*
 * stack.cpp
 *
 * (C) 2023 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the MIT license, see file LICENSE.
 */

#include <stdexcept>
#include <bit>

#include <concurrentfw/stack.hpp>

namespace ConcurrentFW
{

void Stack::push(UnspecifiedBlock block)
{
    if (!block)
        throw std::invalid_argument("nullptr not allowed as block");

    stack.modify(
        [block](const UnspecifiedBlock& stack_cached, UnspecifiedBlock& stack_modify)
        {
            *std::bit_cast<UnspecifiedBlock*>(block) = stack_cached;
            stack_modify = block;
            return true;
        }
    );  // will be completely inlined including lambda

    /*
	aarch64:
L0:	ldaxr	x2, [x0]
	str		x2, [x1]
	stlxr	w2, x1, [x0]
	tbnz	w2, #0, L0
*/
}

Stack::UnspecifiedBlock Stack::pop()
{
    UnspecifiedBlock top;  // will always be initialized in lambda
    stack.modify(
        [&top](const UnspecifiedBlock& stack_cached, UnspecifiedBlock& stack_modify)
        {
            top = stack_cached;
            if (top == nullptr) [[unlikely]]
                return false;
            stack_modify = *std::bit_cast<UnspecifiedBlock*>(top);
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
