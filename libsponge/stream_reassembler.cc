#include "stream_reassembler.hh"
#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
    _output(capacity),
    _capacity(capacity),
    _buf(),
    _first_unread(0),
    _first_unassembled(0),
    _first_unacceptable(0),
    _eof(false),
    _eof_idx(-1) {}



//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 首先判断是否到达eof, 记录_eof_index, 之后如果组装到了这个的前一个， 那么关闭bytestream的写
    int len = data.size();
    if(eof) {
        _eof = true;
        _eof_idx = index + len;
    }
    // TODO: index < _first_unread
    // 1. 如果index + len - 1 < _first_unassembled: return
    // 2. 如果 >= , 截取后进行递归的调用

    // TODO: index + len - first_unread >= capacity: return
    // 这是无符号的整数， 要判断不能为负数否则会溢出
    if(index < get_first_unassembled() && index + len <= get_first_unassembled() ){
        return;
    }
    if(index < get_first_unassembled()  && index + len > get_first_unassembled() ){
        push_substring(data.substr(get_first_unassembled()  - index) ,get_first_unassembled() , eof);
        return;
    }
    // 如果index == first_unassembled
    // 直接放入bytestream
    // 然后进行reassemble操作， 看看能不能跟后面的拼接， 如果能的话， 截取最大的拼接在一起
    // 如果不等
    // 放入map中
    size_t old_first_unassembled = get_first_unassembled();
    size_t old_first_unacceptable =  get_first_unacceptable();

    if(index == get_first_unassembled() ){
        _output.write(data);
        reassemble(old_first_unassembled, old_first_unacceptable);
    } else{
        put_in_buff(data, index);
    }

    if(_eof && get_first_unassembled()  == _eof_idx){
        _buf.clear();
        _output.end_input();
    }
}

void StreamReassembler::reassemble(const size_t old_first_unassembled, const size_t old_first_unacceptable){
    string s = string();
    auto new_first_unassembled = get_first_unassembled();
    for(size_t i = old_first_unassembled; i < old_first_unacceptable; i++){
        if(_buf.count(i) == 0){
            if(i >= new_first_unassembled) break;
            continue;
        }
        if(i < new_first_unassembled){
            _buf.erase(i);
            continue ;
        }
        if(i == new_first_unassembled + s.length()){
            s.push_back(_buf[i]);
            _buf.erase(i);
        } else{
            break;
        }
    }
    _output.write(s);
}

void StreamReassembler::put_in_buff(const std::string &data, const size_t idx){
    int unacceptable = get_first_unacceptable();
    for(size_t i= 0; i < data.length(); i++){
        if(idx + i < get_first_unacceptable()){
            _buf[idx + i] = data[i];
        }
    }
}

size_t StreamReassembler::get_first_unassembled() {return _output.bytes_written(); }
size_t StreamReassembler::get_first_unacceptable() {return _output.bytes_read() + _capacity; }
size_t StreamReassembler::get_first_unread() {return _output.bytes_read(); }
size_t StreamReassembler::unassembled_bytes() const {return _buf.size(); }
bool StreamReassembler::empty() const { return _buf.empty(); }
