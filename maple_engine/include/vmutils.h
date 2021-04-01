/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLE_INCLUDE_VM_VMUTILS
#define MAPLE_INCLUDE_VM_VMUTILS

#ifndef NULL
#define NULL 0
#endif  // NULL

#include "mir_config.h"
#include "vmmemory.h"

template <class T>
struct NodeT {
  T data;
  NodeT<T> *next;
};

template <class T>
class StackT {
 private:
  NodeT<T> *top_;

 public:
  StackT();
  ~StackT();
  T Top();
  T Pop();
  void Push(T);
  bool IsEmpty() {
    return top_ == NULL;
  };
};

template <class T>
class QueueT {
 private:
  NodeT<T> *front_;
  NodeT<T> *rear_;

 public:
  ~QueueT();
  void Init();
  void EnQueue(T x);
  T DeQueue();
  T GetQueue();
  bool IsEmpty() {
    return front_ == rear_;
  }
};

template <class T>
StackT<T>::StackT() {
  top_ = NULL;
}

template <class T>
StackT<T>::~StackT() {
  while (top_) {
    NodeT<T> *top = top_;
    top_ = top_->next;
    VMFreeNOGC(top, sizeof(NodeT<T>));
  }
}

template <class T>
T StackT<T>::Top() {
  return top_->data;
}

template <class T>
T StackT<T>::Pop() {
  if (top_ == NULL) {
    MIR_FATAL("pop an empty stack");
  }
  NodeT<T> *predata = top_;
  T data = top_->data;
  top_ = top_->next;
  VMFreeNOGC(predata, sizeof(NodeT<T>));
  return data;
}

template <class T>
void StackT<T>::Push(T x) {
  NodeT<T> *node = (NodeT<T> *)VMMallocNOGC(sizeof(NodeT<T>));
  node->data = x;
  node->next = top_;
  top_ = node;
}

template <class T>
void QueueT<T>::Init() {
  NodeT<T> *node = (NodeT<T> *)VMMallocNOGC(sizeof(NodeT<T>));
  node->next = NULL;
  front_ = rear_ = node;
}

template <class T>
QueueT<T>::~QueueT() {
  while (front_ != NULL) {
    NodeT<T> *q = front_;
    front_ = front_->next;
    VMFreeNOGC(q, sizeof(NodeT<T>));
  }
}

template <class T>
void QueueT<T>::EnQueue(T x) {
  NodeT<T> *node = (NodeT<T> *)VMMallocNOGC(sizeof(NodeT<T>));
  node->data = x;
  node->next = NULL;
  rear_->next = node;
  rear_ = node;
}

template <class T>
T QueueT<T>::DeQueue() {
  NodeT<T> *p;
  T x;
  if (rear_ == front_) {
    MIR_FATAL("dequeue empty queue");
  }
  p = front_->next;
  x = p->data;
  front_->next = p->next;
  if (p->next == NULL) {
    rear_ = front_;
  }
  VMFreeNOGC(p, sizeof(NodeT<T>));
  return x;
}

template <class T>
T QueueT<T>::GetQueue() {
  return front_->next->data;
}

// define a list
template <class T>
class ListT {
 public:
  NodeT<T> *front_;
  NodeT<T> *rear_;

 public:
  // ListT();
  ~ListT();
  void Init();
  // void InsertFront(T x);
  void InsertBack(T x);
  void DeleteNode(T x);
  bool IsEmpty() {
    return front_ == rear_;
  }

  NodeT<T> *GetFirstNode() {
    return front_->next;
  }
};

template <class T>
void ListT<T>::Init()  // always define a front node
{
  NodeT<T> *node = (NodeT<T> *)VMMallocNOGC(sizeof(NodeT<T>));
  node->next = NULL;
  front_ = rear_ = node;
}

template <class T>
ListT<T>::~ListT() {
  while (front_ != NULL) {
    NodeT<T> *q = front_;
    front_ = front_->next;
    // delete q;
    VMFreeNOGC(q, sizeof(NodeT<T>));
  }
}

template <class T>
void ListT<T>::InsertBack(T x) {
  NodeT<T> *node = (NodeT<T> *)VMMallocNOGC(sizeof(NodeT<T>));
  node->data = x;
  node->next = NULL;
  rear_->next = node;
  rear_ = node;
}

template <class T>
void ListT<T>::DeleteNode(T x) {
  NodeT<T> *node = front_->next;
  NodeT<T> *prenode = front_;
  while (node && node->data != x) {
    prenode = node;
    node = node->next;
  }
  MIR_ASSERT(node && "trying to delete a node that doesn't exist on this list");
  prenode->next = node->next;
  if (rear_ == node) {
    rear_ = prenode;
  }
  VMFreeNOGC(node, sizeof(NodeT<T>));
}

// Operand stack. This stack is per function,
// should be allocated on stack only to make it fast
struct MvalStack {
  Mval stack_[OPERANDS_STACK_SIZE];
  int32 size_;  // current elements on stack

  inline void init() {
    size_ = 0;
  }

  inline void push(Mval val) {
#ifdef DEBUG
    if (size_ < 0 || size_ >= OPERANDS_STACK_SIZE - 1) {
      MIR_FATAL("push to wrong stack");
    }
#endif
    stack_[size_++] = val;
  }

  inline Mval pop() {
#ifdef DEBUG
    if (size_ <= 0) {
      MIR_FATAL("pop wrong");
    }
#endif
    MIR_ASSERT(size_ > 0);
    return stack_[--size_];
  }

  inline Mval top() {
#ifdef DEBUG
    if (size_ <= 0) {
      MIR_FATAL("top wrong");
    }
#endif
    MIR_ASSERT(size_ > 0);
    return stack_[size_ - 1];
  }

  Mval TopNth(int32 n)  // count from 0
  {
#ifdef DEBUG
    if (n > size_ || n < 0) {
      MIR_FATAL("top n wrong");
    }
#endif
    return stack_[size_ - n - 1];
  }
};

#endif  // MAPLE_INCLUDE_VM_VMUTILS
