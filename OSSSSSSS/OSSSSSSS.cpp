// OSSSSSSS.cpp: определяет точку входа для приложения.
//

#include "OSSSSSSS.h"

#include <iostream>
#include <Windows.h>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <array>
#include <condition_variable>

using namespace std;

// Глобальные переменные
mutex mtx; // Мьютекс для синхронизации доступа к общим ресурсам
condition_variable cv; // Условная переменная для управления потоками
vector<bool> isThreadExited; // Вектор для отслеживания завершения потоков
vector<bool> isThreadSleeping; // Вектор для отслеживания состояния сна потоков
vector<bool> activeThreads; // Вектор для хранения статуса активных потоков

// Функция для получения размера массива от пользователя
void GetArraySize(int& size) {
    cout << "Введите размер массива: ";
    while (true) {
        cin >> size;
        if (size > 0) break; // Проверка на корректность введенного размера
        cout << "Некорректный размер массива (должен быть больше 0). Попробуйте снова: ";
    }
}

// Функция для получения количества маркерных потоков от пользователя
void GetMarkerCount(int& count) {
    cout << "Введите количество маркерных потоков: ";
    while (true) {
        cin >> count;
        if (count > 0) break; // Проверка на корректность введенного количества потоков
        cout << "Некорректное количество потоков (должно быть больше 0). Попробуйте снова: ";
    }
}

// Функция для получения ID потока, который нужно остановить
void GetThreadToStop(int& stopId, int max) {
    cout << "\nВведите id потока для остановки (от 1 до " << max << "): ";
    while (true) {
        cin >> stopId;
        if (stopId > 0 && stopId <= max) break; // Проверка на корректность введенного ID
        cout << "Некорректно, диапазон (1; " << max << "). Попробуйте снова: ";
    }
}

// Функция, выполняемая каждым маркерным потоком
void MarkerThread(int id, vector<int>& array) {
    srand(id);
    int arraySize = array.size();
    int coloredCount = 0;

  
        while (true) {
            unique_lock<mutex> lock(mtx);

            // Упрощенное условие выхода
            if (isThreadExited[id]) {
                activeThreads[id] = false;
                break;
            }

            cv.wait(lock, [&] { return !isThreadSleeping[id] || isThreadExited[id]; });

            // Проверка на завершение, даже если поток пробужден
            if (isThreadExited[id]) {
                activeThreads[id] = false;
                break;
            }

            // Генерация и проверка индекса для окрашивания
            int randomIndex = rand() % arraySize;
            if (array[randomIndex] == 0) {
                this_thread::sleep_for(chrono::milliseconds(5));
                array[randomIndex] = id + 1;
                this_thread::sleep_for(chrono::milliseconds(5));
                coloredCount++;
            }
            else {
                isThreadSleeping[id] = true;
                cout << "порядковый номер: " << id << endl;
                cout << "количество помеченных элементов" << coloredCount << endl;
                cout << "индекс: " << randomIndex << endl;
                coloredCount = 0;
            }

            lock.unlock();
            cv.notify_all();
        }
}

// Главная функция
int main() {
    setlocale(LC_ALL, "rus"); // Установка локали для русского языка
    int arraySize;
    GetArraySize(arraySize); // Получение размера массива


        vector<int> array(arraySize, 0); // Инициализация массива нулями
    int markerCount;
    GetMarkerCount(markerCount); // Получение количества маркерных потоков


    vector<thread> threads(markerCount); // Вектор для хранения потоков
    isThreadExited.resize(markerCount, false); // Инициализация вектора завершения потоков
    isThreadSleeping.resize(markerCount, false); // Инициализация вектора сна потоков
    activeThreads.resize(markerCount, true); // Изначально все потоки активны

    // Запуск маркерных потоков
    for (int i = 0; i < markerCount; i++) {
        threads[i] = thread(MarkerThread, i, ref(array));
    }

    cv.notify_all(); // Запуск всех потоков

    int exitedThreadCount = 0; // Счетчик завершенных потоков
    while (exitedThreadCount < markerCount) {
        unique_lock<mutex> lock(mtx); // Блокировка мьютекса
        cv.wait(lock, [&] {
            return find(isThreadSleeping.begin(), isThreadSleeping.end(), 0) == isThreadSleeping.end();
            });


        // Проверка активных потоков
        cout << "\nАктивные потоки: ";
        for (size_t i = 0; i < activeThreads.size(); i++) {
            if (activeThreads[i]) {
                cout << i + 1 << " "; // Вывод активных потоков
            }
        }
        cout << endl;

        // Проверка, завершены ли все потоки
        if (exitedThreadCount == markerCount) {
            break; // Все потоки остановлены
        }

        cout << "Содержимое массива: ";
        for (const auto& value : array) {
            cout << value << " "; // Вывод содержимого массива
        }
        cout << endl;

        int stopId;
        GetThreadToStop(stopId, markerCount); // Получение ID потока для остановки

        // Проверка, активен ли выбранный поток
        if (!activeThreads[stopId - 1]) {
            cout << "Поток " << stopId << " уже остановлен." << endl;
            continue; // Пропустить итерацию, если поток уже неактивен
        }

        // Сброс окрашенного индекса
        for (size_t i = 0; i < arraySize; i++) {
            if (array[i] == stopId) {
                array[i] = 0; // Сброс окрашенного индекса
            }
        }

        isThreadExited[stopId - 1] = true; // Установка флага выхода
        isThreadSleeping = isThreadExited; // Обновление состояния сна
        activeThreads[stopId - 1] = false; // Обновление статуса потока
        exitedThreadCount++; // Увеличение счетчика завершенных потоков
        lock.unlock(); // Освобождение мьютекса
        cv.notify_all(); // Уведомление потоков о завершении
    }

    cv.notify_all();
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join(); // Ожидание завершения потока
        }
    }

    // Вывод итогового массива
    cout << "\n\nИтоговый массив:\n";
    for (const auto& value : array) {
        cout << value << " ";
    }

    return 0;
}
