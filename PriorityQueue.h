#pragma once
#include <vector>
#include <map>
#include <optional>

template <typename K, typename T>
class PriorityQueue2
{

public:
    // Return the first element of the m_queue if it was not empty.
    std::optional<T> min()
    {
        if (len() == 0)
            return std::nullopt; 
        auto it = m_queue.begin();
        return it->second;
    }

    std::optional<T> pop()
    {
        if (len() == 0)
            return std::nullopt;

        auto it = m_queue.begin();
        T ret = it->second;
        m_queue.erase(m_queue.begin()); 
        return ret;
    } 

    size_t len() const noexcept { return m_queue.size(); }

    void append(const K& key, const T& item) { m_queue.insert({key, item}); }

private:
    std::map<K, T> m_queue; 
};


template <typename T>
class PriorityQueue
{
public:
    // Return the first element of the m_queue if it was not empty.
    std::optional<T> min() 
    { 
        if (len() == 0) 
            return std::nullopt; 
        FindMin();
        return m_queue[m_minIndex.value()];
    }

    std::optional<T> pop()
    {
        if (len() == 0)
            return std::nullopt;
        T ret = m_queue[m_minIndex.value()];
        m_queue.erase(m_queue.begin() + m_minIndex.value());
        m_minIndex = std::nullopt;
        return ret;
    }
    
    void FindMin()
    {
        if (m_minIndex.has_value())
            return;
        auto min = m_queue[0];
        m_minIndex = 0;
        for (int i = 1; i < m_queue.size(); ++i)
        {
            auto val = m_queue[i];
            if (val < min)
            {
                min = val;
                m_minIndex = i;
            }
        }
    }
    size_t len() const noexcept { return m_queue.size(); }

    // Create and insert the new item to the end of the m_queue and Nullified the m_minIndex.
    void append(const T& item) { m_queue.emplace_back(item); m_minIndex = std::nullopt; }
    
private:
    std::vector<T> m_queue;
    std::optional<int> m_minIndex{};
};

