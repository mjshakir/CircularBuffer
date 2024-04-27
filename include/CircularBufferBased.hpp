#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <optional>
#include <span>
#include <utility>
//--------------------------------------------------------------
namespace CircularBuffer{
    //--------------------------------------------------------------
    template <typename T>
    class CircularBufferBased {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            CircularBufferBased(void)           = default;
            //--------------------------
            virtual ~CircularBufferBased(void)  = default;
            //--------------------------
            void push(const T& item) {
                push_back(item);
            }// end void push(const T& item) 
            //--------------------------
            void push(T&& item) {
                push_back(std::move(item));
            }// end void push(T&& item)
            //--------------------------
            template <typename... Args>
            void emplace(Args&&... args) {
                emplace_back(std::forward<Args>(args)...);
            }// end void emplace(Args&&... args)
            //--------------------------
            void pop(void) {
                pop_front();
            }// end void pop(void)
            //--------------------------
            void top(void){
                return get_top();
            }//end void top(void)
            //--------------------------
            std::optional<T> top_pop(void){
                return get_top_pop();
            }// end std::optional<T> pop(void)
            //--------------------------
            bool empty(void) const {   
                return is_empty();
            }// end bool empty(void) const
            //--------------------------
            size_t size(void) const {   
                return get_size();
            }// end size_t size(void) const
            //--------------------------
            void reset(void){
                clear();
            }// end void reset(void)
            //--------------------------
            std::optional<std::span<T>> span(void){
                return get_span();
            }// end std::optional<std::span<T>> span(void)
            //--------------------------
        protected:
            //--------------------------
            virtual void push_back(const T& item)               = 0;
            //--------------------------
            virtual void push_back(T&& item)                    = 0;
            //--------------------------
            template <typename... Args>
            virtual void emplace_back(Args&&... args)           = 0;
            //--------------------------
            virtual std::optional<T> get_top_pop(void)          = 0;
            //--------------------------
            virtual std::optional<T> get_top(void)              = 0;
            //--------------------------
            virtual bool pop_front(void)                        = 0;
            //--------------------------
            virtual bool is_empty(void) const                   = 0;
            //--------------------------
            virtual size_t get_size(void) const                 = 0;
            //--------------------------
            virtual void clear(void)                            = 0;
            //--------------------------
            virtual std::optional<std::span<T>> get_span(void)  = 0;
            //--------------------------
    };//end class CircularBufferBased
    //--------------------------------------------------------------
}// end namespace CircularBuffer
//--------------------------------------------------------------