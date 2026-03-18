// Google tests for List class (list.hpp)
#include <gtest/gtest.h>
#include "../../include/data_structures/list.hpp"

TEST(ListTest, InsertAndRemove) {
    List<int> list;
    EXPECT_TRUE(list.empty());

    list.insert(1);
    list.insert(2);
    list.insert(3);
    EXPECT_FALSE(list.empty());

    EXPECT_EQ(list.remove(), 1);
    EXPECT_EQ(list.remove(), 2);
    EXPECT_EQ(list.remove(), 3);
    EXPECT_TRUE(list.empty());
}

TEST(ListTest, RemoveFromEmpty) {
    List<int> list;
    EXPECT_THROW(list.remove(), std::runtime_error);
}

// Testing thread safety by inserting and removing elements from multiple threads
#include <thread>
#include <vector>
#include <atomic>
TEST(ListTest, ThreadSafety) {
    List<int> list;
    const int num_threads = 10;
    const int num_operations = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> insert_count(0);
    std::atomic<int> remove_count(0);

    // Insert elements in multiple threads
    for(int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&list, &insert_count, num_operations]() {
            for(int j = 0; j < num_operations; ++j) {
                list.insert(j);
                insert_count++;
            }
        });
    }

    // Remove elements in multiple threads
    for(int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&list, &remove_count, num_operations]() {
            for(int j = 0; j < num_operations; ++j) {
                try {
                    list.remove();
                    remove_count++;
                } catch (const std::runtime_error&) {
                    // List might be empty, ignore exceptions
                }
            }
        });
    }

    for(auto & thread : threads) {
        thread.join();
    }

    EXPECT_EQ(insert_count.load(), num_threads * num_operations);
    EXPECT_EQ(remove_count.load(), num_threads * num_operations);
}
