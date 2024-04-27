#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <optional>
#include <vector>
#include <deque>
#include <mutex>
#include <span>
//--------------------------------------------------------------
namespace CircularBuffer {
    //--------------------------------------------------------------
    template <typename T>
    class CircularBufferDynamic {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            explicit CircularBufferDynamic(const size_t& size) : m_max_size(size) {
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            CircularBufferDynamic(void) = delete;
            //--------------------------
            ~CircularBufferDynamic(void) = default;
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
            typename std::deque<T>::iterator begin(void) {
                return m_buffer.begin();
            }//end typename std::deque<T>::iterator begin(void)
            //--------------------------
            typename std::deque<T>::iterator end(void) {
                return m_buffer.end();
            }// end typename std::deque<T>::iterator
            //--------------------------
            typename std::deque<T>::const_iterator begin(void) const {
                return m_buffer.begin();
            }// end typename std::deque<T>::const_iterator begin(void) const
            //--------------------------
            typename std::deque<T>::const_iterator end(void) const {
                return m_buffer.end();
            }//end typename std::deque<T>::const_iterator end(void) const
            //--------------------------
            typename std::deque<T>::const_iterator cbegin(void) const {
                return m_buffer.cbegin();
            }//end typename std::deque<T>::const_iterator cbegin(void) const
            //--------------------------
            typename std::deque<T>::const_iterator cend(void) const {
                return m_buffer.cend();
            }//end typename std::deque<T>::const_iterator cend(void) const
            //--------------------------
            typename std::deque<T>::reverse_iterator rbegin(void) {
                return m_buffer.rbegin();
            }//end typename std::deque<T>::reverse_iterator rbegin(void)
            //--------------------------
            typename std::deque<T>::reverse_iterator rend(void) {
                return m_buffer.rend();
            }//end typename std::deque<T>::reverse_iterator rend(void)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            void push_back(const T& item) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.size() == m_max_size) {
                    static_cast<void>(m_buffer.pop_front());
                }// end if (m_buffer.size() == m_max_size)
                //--------------------------
                m_buffer.push_back(item);
                //--------------------------
            }// end void push_back(const T& item)
            //--------------------------
            void push_back(T&& item) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.size() == m_max_size) {
                    static_cast<void>(m_buffer.pop_front());
                }// end if (m_buffer.size() == m_max_size)
                //--------------------------
                m_buffer.push_back(std::move(item));
                //--------------------------
            }// end void push_back(T&& item)
            //--------------------------
            template <typename... Args>
            void emplace_back(Args&&... args) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.size() == m_max_size) {
                    static_cast<void>(m_buffer.pop_front());
                }//end if (m_buffer.size() == m_max_size)
                //--------------------------
                m_buffer.emplace_back(std::forward<Args>(args)...);
                //--------------------------
            }// end void emplace_back(Args&&... args)
            //--------------------------
            std::optional<T> get_top_pop(void) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                T value = m_buffer.front();
                static_cast<void>(m_buffer.pop_front());
                //--------------------------
                return value;
                //--------------------------
            }// end std::optional<T> get_top_pop(void)
            //--------------------------
            std::optional<T> get_top(void) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return m_buffer.front();
                //--------------------------
            }// end T get_top(void)
            //--------------------------
            bool pop_front(void) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return false;
                }// end if (m_buffer.empty())
                //--------------------------
                m_buffer.pop_front();
                //--------------------------
                return true;
                //--------------------------
            }// end bool pop_front(void)
            //--------------------------
            bool is_empty(void) const override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return m_buffer.empty();
                //--------------------------
            }// end bool is_empty(void) const
            //--------------------------
            size_t get_size(void) const override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return m_buffer.size();
                //--------------------------
            }// end size_t get_size(void) const
            //--------------------------
            void clear(void) override {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                m_buffer.clear();
                //--------------------------
            }// end void clear(void)
            //--------------------------
            std::optional<std::span<T>> get_span(void) override {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                // Create a vector that copies all elements from the deque
                //--------------------------
                std::vector<T> contiguous_copy(m_buffer.begin(), m_buffer.end());
                //--------------------------
                // Return a span constructed from this contiguous vector
                //--------------------------
                return std::span<T>(contiguous_copy.data(), contiguous_copy.size());
                //--------------------------
            }// end std::optional<std::span<T>> get_span(void)
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            const size_t m_max_size;
            std::deque<T> m_buffer;
            std::mutex m_mutex;
            //--------------------------------------------------------------
    };// end class CircularBufferDynamic
    //--------------------------------------------------------------
}// end namespace CircularBuffers
