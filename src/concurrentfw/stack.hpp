//
// Created by simon on 14.10.22.
//

#pragma once
#include <type_traits>
#ifndef CONCURRENTFW_STACK_HPP
#define CONCURRENTFW_STACK_HPP

#include <functional>  // std::function<>
#include <new>		   // std::hardware_destructive_interference_size
#include <cstddef>	   // offsetof()
#include <optional>	   // std::optional

#include <concurrentfw/aba_wrapper.hpp>

namespace ConcurrentFW
{

class alignas(64) Stack	 // align to cache line
{
	using UnspecifiedBlock = void*;
private:
	ABA_Wrapper<UnspecifiedBlock> stack {nullptr};

public:
	void push [[ATTRIBUTE_ABA_LOOP_OPTIMIZE]] (UnspecifiedBlock block);
	void* pop [[ATTRIBUTE_ABA_LOOP_OPTIMIZE]] ();
};

}  // namespace ConcurrentFW

#endif	//CONCURRENTFW_STACK_HPP
