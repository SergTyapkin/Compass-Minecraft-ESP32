#pragma once
#include <Arduino.h>

// ============================================================
//  Медианный фильтр для удаления выбросов из данных
//  Хранит последние SIZE измерений и возвращает их медиану
// ============================================================
template <typename T, int SIZE>
class MedianFilter {
private:
    T buffer[SIZE];
    int index = 0;
    int count = 0;
    
public:
    MedianFilter() {
        for (int i = 0; i < SIZE; i++) {
            buffer[i] = 0;
        }
    }
    
    // Добавить новое значение
    void add(T value) {
        buffer[index] = value;
        index = (index + 1) % SIZE;
        if (count < SIZE) count++;
    }
    
    // Получить медиану текущих значений
    T getMedian() {
        if (count == 0) return 0;
        
        T temp[SIZE];
        for (int i = 0; i < count; i++) {
            temp[i] = buffer[i];
        }
        
        // Сортировка пузырьком для маленьких массивов
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - i - 1; j++) {
                if (temp[j] > temp[j + 1]) {
                    T swap = temp[j];
                    temp[j] = temp[j + 1];
                    temp[j + 1] = swap;
                }
            }
        }
        
        return temp[count / 2];
    }
    
    // Получить последнее значение
    T getLast() {
        if (count == 0) return 0;
        int lastIndex = (index - 1 + SIZE) % SIZE;
        return buffer[lastIndex];
    }
    
    // Получить количество накопленных значений
    int getCount() {
        return count;
    }
    
    // Проверить, заполнен ли буфер
    bool isFull() {
        return count == SIZE;
    }
    
    // Сбросить фильтр
    void reset() {
        index = 0;
        count = 0;
        for (int i = 0; i < SIZE; i++) {
            buffer[i] = 0;
        }
    }
};
