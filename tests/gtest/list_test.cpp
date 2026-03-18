// Google tests for List class (list.hpp)
#include <gtest/gtest.h>
#include "../../include/data_structures/list.hpp"
#include <thread>
#include <vector>
#include <iostream>

TEST(ListTest, InsertAndRemove) {
    List<int> list;
    EXPECT_TRUE(list.empty());

    int * a = new int(1);
    int * b = new int(2);
    int * c = new int(3);
    list.insert(a);
    list.insert(b);
    list.insert(c);
    EXPECT_FALSE(list.empty());

    EXPECT_EQ(list.remove(), a);
    EXPECT_EQ(list.remove(), b);
    EXPECT_EQ(list.remove(), c);
    EXPECT_TRUE(list.empty());
    delete a;
    delete b;
    delete c;
}

TEST(ListTest, RemoveFromEmptyList) {
    List<int> list;
    EXPECT_TRUE(list.empty());
    EXPECT_THROW(list.remove(), std::runtime_error);
}

TEST(ListTest, ConcurrentAccess) {
    List<int> list;
    const int num_threads = 10;
    const int num_iterations = 1000;

    auto insert_func = [&list](int thread_id) {
        for (int i = 0; i < num_iterations; i++) {
            int * value = new int(thread_id * num_iterations + i);
            list.insert(value);
        }
    };


    auto remove_func = [&list]() {
        int removed_count = 0;
        while (removed_count < num_iterations) {
            try {
                int * value = list.remove();
                delete value;
                removed_count++; // Só conta se removeu com sucesso
            } catch (const std::runtime_error&) {
                // A lista está vazia no momento, mas ainda há 
                // inserções pendentes. Vamos dar um "respiro" para a CPU.
                std::this_thread::yield(); 
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(insert_func, i);
        threads.emplace_back(remove_func);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Current size: " << list.size() << std::endl;
    EXPECT_TRUE(list.empty());
}
