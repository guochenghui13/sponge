#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _ackno_absolute(0)
    , _ackno(0)
    , retx_nums(0)
    , _retransmission_timeout(retx_timeout)
    , _ticks(0)
    , _window_size(1)
    , zero_window(false)
    , syn_send(false)
    , syn_received(false)
    , fin_send(false)
    , fin_received(false){}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ackno_absolute;}

void TCPSender::fill_window() {
    // 结束连接直接返回
    if(fin_send || fin_received) return;

    while(bytes_in_flight() < _window_size){
        // TODO: 如果bytestream里面没有数据, 并且还有发送syn？
        if(!syn_send && _stream.eof()){
            cout << "undefined behaviour" << endl;
            return;
        }

        TCPSegment seg;
        // 处理头部
        if(!syn_send){
            // syn 包处理
            seg.header().syn = true;
            seg.header().seqno = _isn;
            syn_send = true;
        } else if(!fin_send && _stream.eof()){
            // fin包处理
            seg.header().fin = true;
        } else{
            // 普通包处理
            seg.header().seqno = wrap(_next_seqno, _isn);
        }

        // 处理body
        size_t len = min(_window_size - bytes_in_flight() - seg.length_in_sequence_space(), TCPConfig::MAX_PAYLOAD_SIZE);
        seg.payload() = _stream.read(len);

        if (seg.length_in_sequence_space() == 0) break;

        // 发送数据包
        _segments_out.push(seg);

        // 记录还没收到确认的数据包
        fly_segment.push_back(seg);

        // 更新next_seq
        _next_seqno += seg.length_in_sequence_space();
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 更新状态
    // 首先要获取ack， 然后判断这个ack是否是比我们记录的ack还小， 如果还小的话可能是历史记录， 然后可以不用管
    uint64_t tmp_ackno_absolute = unwrap(ackno, _isn, _ackno_absolute);
    if(tmp_ackno_absolute < _ackno_absolute) return;
    else{
        _ackno_absolute = tmp_ackno_absolute;
        _ackno = ackno;
    }

    if(syn_send && !syn_received && _ackno_absolute > 0){
        syn_received = true;
    } else if(fin_send && !fin_received && _ackno_absolute >= _stream.bytes_read() + 2){
        fin_received = true;
        fly_segment.clear();
    }

    retx_nums = 0;
    _retransmission_timeout = _initial_retransmission_timeout;
    _ticks = 0;

    // 更新窗口
    if(window_size > 0){
        _window_size = window_size;
        zero_window = false;
    } else{
        // HINT 如果窗口大小是0 就将其看成1
        _window_size = 1;
        zero_window = true;
    }

    // 更新确认
    vector<TCPSegment> tmp_vec;
    for(auto seg : fly_segment){
        uint64_t seg_seqno = unwrap(seg.header().seqno, _isn, _ackno_absolute);
        // TODO: 这里放入的思路看看能不能调整下
        if(seg_seqno + seg.length_in_sequence_space() > _ackno_absolute){
            tmp_vec.push_back(seg);
        }
    }
    fly_segment = tmp_vec;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(fly_segment.empty()){
        cout << "NO fly segment, but tick expires" << endl;
        _ticks = 0;
    }

    _ticks += ms_since_last_tick;

    if(_ticks >= _retransmission_timeout){
        _ticks = 0;
        _segments_out.push(fly_segment[0]);

        if(!zero_window){
            _retransmission_timeout *= 2;
        }
        retx_nums++;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return retx_nums; }

void TCPSender::send_empty_segment() {
    TCPSegment tcpSegment;
    tcpSegment.header().seqno = next_seqno();
    _next_seqno += tcpSegment.length_in_sequence_space();
    _segments_out.push(tcpSegment);
}
