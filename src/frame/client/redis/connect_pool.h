#pragma once
#include <memory>
#include <list>
#include <string>
#include <functional>
namespace redis_client {

/*
一个大小为pool_size的连接池，连接资源紧张时会自动扩张到max_size，但最大不能超过max_size，
资源紧张情况消失时连接池又会恢复到pool_size
*/
template <typename Conn>
class ConnectPool {
public:
    typedef std::function<std::shared_ptr<Conn>(void)> BuildFunc;
    BuildFunc _build_connection;
public:
    ConnectPool(int pool_size, int max_size, BuildFunc build_connection) noexcept :
    _pool_size(pool_size), _max_size(max_size), _build_connection(build_connection) {
        if (_pool_size > max_size) {
            throw std::runtime_error("pool_size should not be greater than max_size, pool_size: " + std::to_string(_pool_size)
            + " max_size: " + std::to_string(_max_size));
        }
        for (int i = 0; i < _pool_size; i++){
            auto conn = build_connection();
            if (conn == nullptr) {
                std::cout << "build_connection return nullptr, error!";
            } else {
                conn_list.push_back(conn);
            }
        }
    }
    std::shared_ptr<Conn> borrow(int* borrow_size = nullptr) {
        std::lock_guard<std::mutex> guard(mut);
        std::shared_ptr<Conn> conn = nullptr;
        if (conn_list.size() > 0) {
            conn = conn_list.front();
            conn_list.pop_front();
            ++_borrow_size;
        } else {
			std::cout << "conn_list.size is 0, build one" << std::endl;
            conn = _build_connection();
            ++_borrow_size;
        }
        if (_borrow_size > _pool_size) {
            ++_busy;
        } else {
            _busy = 0;
        }
        if (borrow_size != nullptr) {
            *borrow_size = _borrow_size;
        }
        return conn;
    }

    bool release(std::shared_ptr<Conn> conn) {
        if (!conn) {
            std::cout << "conn is nullptr, when conn";
            return false;
        }
        std::lock_guard<std::mutex> guard(mut);
        if (_borrow_size <= 0) {
			std::cout << "_borrow_size: " << _borrow_size << " is illegal, a code error ";
        }
        --_borrow_size;
        if (conn_list.size() >= _max_size) {
            // should not happen
            return false;
        }
        if (conn_list.size() < _pool_size) {
            conn_list.push_back(conn);
            return true;
        }
        if (_busy > 0) {
            // 处于client资源紧张状态,conn复用
            conn_list.push_back(conn);
            return true;
        }
        //这种情况资源紧张情况消失, 删掉多余connection
        return true;
    }
    bool size() {
        std::lock_guard<std::mutex> guard(mut);
        return conn_list.size();
    }
private:
    uint32_t _pool_size = 0;
    uint32_t _max_size = 0;
    uint32_t _borrow_size = 0;
    uint32_t _busy = 0;
    std::list<std::shared_ptr<Conn>> conn_list;
    std::mutex mut;
};
} // end of namespace
