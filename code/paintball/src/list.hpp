#ifndef __LIST_H
#define __LIST_H

#include <array>

template<typename T, std::size_t S>
class List : public std::array<T, S> {
    private:
        std::size_t count = 0;
    public:
        // TODO: return success/failure
        void add(T &&item) {
            if (count >= this->size()) return;
            auto &&newItem = std::array<T,S>::operator[](count);
            // TODO: this still does allocation
            newItem = std::move(item);
            count++;
        }

        void remove(typename std::array<T,S>::iterator it) {
            if (count == 0) return;
            count--;
            *it = std::move(std::array<T,S>::operator[](count));
        }

        typename std::array<T,S>::iterator end() noexcept {
            return this->begin() + count;
        }
};

#endif // __LIST_H
