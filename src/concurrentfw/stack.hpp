//
// Created by simon on 14.10.22.
//

#pragma once
#ifndef CONCURRENTFW_STACK_HPP
#define CONCURRENTFW_STACK_HPP

#include <functional>

#include <concurrentfw/aba_wrapper.hpp>

namespace ConcurrentFW
{

template<typename T>
class Stack
{
	struct Node
	{
		explicit Node(const T& object_init)
		: object(object_init)
		{}

		template<typename... ARGS>
		Node(ARGS&&... args)
		: object {std::forward(args...)}
		{}

		Node* next {nullptr};
		T object;
	};

private:
	ABA_Wrapper<Node*> aba_top_ptr;

public:
	Stack() = default;
	Stack(Stack&&) = delete;
	Stack(const Stack&) = delete;
	Stack& operator=(Stack&&) = delete;
	Stack& operator=(const Stack&) = delete;

	~Stack();

	void push(T object_init);

	template<typename... ARGS>
	void enplace(ARGS&&... args);

	bool pop(T& object_top);

	template<typename RET>
	void bulk_pop_visit(std::function<RET(T&)> visit);

	template<typename RET>
	void bulk_pop_reverse_visit(std::function<RET(T&)> visit);

private:
	void node_push(Node* node);
	bool node_pop(Node*& node);
	Node* nodes_pop_all();

	static void nodes_cleanup(Node* nodes);
	static Node* nodes_reverse(Node* top);

	template<typename RET>
	static void nodes_visit_delete(Node* top, std::function<RET(T&)>& visit);
};

template<typename T>
Stack<T>::~Stack()
{
	Node* nodes = aba_top_ptr.get_nonatomic();
	nodes_cleanup(nodes);
}

template<typename T>
void Stack<T>::push(T object_init)
{
	Node* const new_node = new Node(object_init);
	node_push(new_node);
}

template<typename T>
template<typename... ARGS>
void Stack<T>::enplace(ARGS&&... args)
{
	Node* const new_node = new Node(std::forward(args...));
	node_push(new_node);
}

template<typename T>
bool Stack<T>::pop(T& object_top)
{
	Node* top;	// will be initialized in lambda

	bool success = node_pop(top);
	if (success)
	{
		object_top = top->object;
		delete top;
	}

	return success;
}

template<typename T>
template<typename RET>
void Stack<T>::bulk_pop_visit(std::function<RET(T&)> visit)
{
	Node* top = nodes_pop_all();
	nodes_visit_delete(top, visit);
}

template<typename T>
template<typename RET>
void Stack<T>::bulk_pop_reverse_visit(std::function<RET(T&)> visit)
{
	Node* top = nodes_pop_all();
	Node* reverse = nodes_reverse(top);
	nodes_visit_delete(reverse, visit);
}

template<typename T>
void Stack<T>::node_push(Node* node)
{
	// GCC: lamdas in function pointers can be inlined completely, but must not have captures
	bool (*inlined_modify_func)(Node* const&, Node*&, Node* const)	// implicit cast of lambda to function pointer
		= [](Node* const& ptr_cached, Node*& ptr_modify, Node* const node) -> bool
	{
		node->next = ptr_cached;
		ptr_modify = node;
		return true;
	};

	aba_top_ptr.modify(inlined_modify_func, node);	// will be completely inlined including lambda
}

template<typename T>
bool Stack<T>::node_pop(Node*& node)
{
	// GCC: lamdas in function pointers can be inlined completely, but must not have captures
	bool (*inlined_modify_func)(Node* const&, Node*&, Node*&)  // implicit cast of lambda to function pointer
		= [](Node* const& ptr_cached, Node*& ptr_modify, Node*& node) -> bool
	{
		node = ptr_cached;
		if (!node)	// if stack is already empty, no exchange is necessary
			return false;
		ptr_modify = node->next;
		return true;
	};

	return aba_top_ptr.modify(inlined_modify_func, node);
}

template<typename T>
typename Stack<T>::Node* Stack<T>::nodes_pop_all()
{
	Node* top;	// will always be initialized in lambda

	// GCC: lamdas in function pointers can be inlined completely, but must not have captures
	bool (*inlined_modify_func)(Node* const&, Node*&, Node*&)  // implicit cast of lambda to function pointer
		= [](Node* const& ptr_cached, Node*& ptr_modify, Node*& top) -> bool
	{
		top = ptr_cached;
		if (!top)  // if stack is already empty, no exchange is necessary
			return false;
		ptr_modify = nullptr;
		return true;
	};

	aba_top_ptr.modify(inlined_modify_func, top);
	return top;
}

template<typename T>
void Stack<T>::nodes_cleanup(Node* nodes)
{
	while (nodes)
	{
		Node* cleanup = nodes;
		nodes = nodes->next;
		delete cleanup;
	}
}

template<typename T>
typename Stack<T>::Node* Stack<T>::nodes_reverse(Node* top)
{
	Node* reverse = nullptr;
	while (top)
	{
		Node* next = top->next;
		top->next = reverse;
		reverse = top;
		top = next;
	}
	return reverse;
}

template<typename T>
template<typename RET>
void Stack<T>::nodes_visit_delete(Node* top, std::function<RET(T&)>& visit)
{
	while (top)
	{
		visit(top->object);
		Node* old = top;
		top = top->next;
		delete old;
	}
}

}  // namespace ConcurrentFW

#endif	//CONCURRENTFW_STACK_HPP
