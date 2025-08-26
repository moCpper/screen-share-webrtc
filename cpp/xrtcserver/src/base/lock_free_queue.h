#ifndef __BASE_LOCK_FREE_QUEUE_H__
#define __BASE_LOCK_FREE_QUEUE_H__

#include <atomic>

namespace xrtc{

// 一个生产者，一个消费者情况下的无锁队列
template<typename T>
class LockFreeQueue{
public:
    LockFreeQueue(){
        first = divider = last = new Node(T());  // 哨兵node
        size_ = 0;
    }

    ~LockFreeQueue(){
        while (first) {
            Node* temp = first;
            first = first->next;
            delete temp;
        }
        size_ = 0;
    }

    void produce(const T& t){
        last->next = new Node(t);
        last = last->next;
        size_ ++;
        
        while(divider != first){
            Node* temp = first;
            first = first->next;
            delete temp;
        }
    }

    bool consume(T* result){
        if(divider != last){
            *result = divider->next->data;
            divider = divider->next;
            size_ --;
            return true;
        }
        return false;
    }

    bool empty(){return size_ == 0; }
    int size(){ return size_.load();}

private:
    struct Node{
        T data;
        Node* next;
        Node(T value) : data(value), next(nullptr) {}
    };
    Node* first;
    Node* divider;      // 指向被消费节点
    Node* last;
    std::atomic<int> size_;
};

}

#endif 