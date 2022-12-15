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
    _unassemble_strs(),
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
    // index < _first_unread
    // 1. 如果index + len - 1 < _first_unassembled: return
    // 2. 如果 >= , 截取后进行递归的调用
    if(index < get_first_unassembled() && index + len <= get_first_unassembled() ){
        return;
    }

    cout << "first_unassembled: " << get_first_unassembled() << endl;

    if(index < get_first_unassembled()  && index + len > get_first_unassembled() ){
        push_substring(data.substr(get_first_unassembled()  - index) ,get_first_unassembled() , eof);
        return;
    }
    // 如果index + len - first_unread >= capacity: return
    // TODO: 这个地方不太确定是否要抛弃， 还是放在reassemble的buffer里
    // 否则会溢出
    if(index + len > _first_unread && index + len - _first_unread - 1 > _capacity){
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
    // _eof标识位是true， 并且_eof_idx 之前已经被装配了
    if(_eof && get_first_unassembled()  == _eof_idx){
        _unassemble_strs.clear();
        _output.end_input();
    }
}

void StreamReassembler::reassemble(const size_t _f_ua, const size_t _f_unacc){
    // TODO: 完成加入字符串后， 看看map中还有什么能合并
    for(size_t i = _f_ua; i < _f_unacc; i++){
        // 如果i 已经超过unassmeble 跳过
        if(i > get_first_unassembled()) break;

        // 不存在map中直接跳过
        if(_unassemble_strs.count(i) == 0) continue;

        // 已经读到bytestream就不要了
        if(i + _unassemble_strs[i].size() - 1 < get_first_unassembled()){
            _unassemble_strs.erase(i);
            continue;
        }
        string tmp = _unassemble_strs[i].substr(get_first_unassembled() - i);
        _unassemble_strs.erase(i);
        _output.write(tmp);
    }
}

void StreamReassembler::put_in_buff(const std::string &data, const size_t idx){
    if(_unassemble_strs.size() == 0){
        _unassemble_strs[idx] = data;
        return;
    }

    // TODO: 主要要看看map中是否有重合的string， 如果有的话要合并
    auto higher = _unassemble_strs.upper_bound(idx + data.size());
    auto lower = _unassemble_strs.upper_bound(idx);

    // 判断是否到头了， 可能存在没有比idx还小
    if(lower != _unassemble_strs.begin()) lower--;
    if(higher != _unassemble_strs.begin()) higher--;

    // 如果完全被包含就直接返回
    if(lower->first <= idx && lower->first + lower->second.size() - 1 >= idx +data.size()) return;
    if(higher->first <= idx && higher->first + higher->second.size() - 1 >= idx +data.size()) return;

    // 看看是否和前面合并
    // 前面的结尾大于等于idx， 说明出现了交集
    if(lower->first < idx && lower-> first + lower->second.size() > idx){
        auto tmp = lower->second;
        tmp = tmp.substr(0, idx - lower->first);
        _unassemble_strs[lower->first] = tmp;
    }

    // 看看是否和后面合并
    // 如果后面的结尾小于index + data.size() 不用处理了， 后面直接删掉
    // 否则把后面的开头截取下重新放入
    if(idx + data.size() > higher->first) {
        if (idx + data.size() < higher->first + _unassemble_strs[higher->first].size()) {
            auto tmp = higher->second;
            tmp = tmp.substr(idx + data.size() - higher->first + higher->second.size());
            _unassemble_strs[idx + data.size()] = tmp;
        }
    }
    // 在index -> index + data.size()中的所有区间都被删去
    _unassemble_strs.erase(_unassemble_strs.lower_bound(idx), _unassemble_strs.upper_bound(idx + data.size() - 1));
    _unassemble_strs[idx] = data;
}

size_t StreamReassembler::get_first_unassembled() {return _output.bytes_written(); }
size_t StreamReassembler::get_first_unacceptable() {return _output.bytes_read() + _capacity; }

size_t StreamReassembler::unassembled_bytes() const {
    int sum = 0;
    for(auto i : _unassemble_strs){
        sum += i.second.size();
    }
    return sum;
}

bool StreamReassembler::empty() const { return _unassemble_strs.size() == 0; }
