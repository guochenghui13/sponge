#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // 这里要判断下是否有isn， 如果没有的话需要有syn的seg我们才接受
    // 这里说明是建立连接的第一个包
    if(!has_syn && seg.header().syn){
        _isn = seg.header().seqno;
        _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
        has_syn = true;
    } else if(has_syn && !seg.header().syn){
        uint64_t abs_seq = unwrap(seg.header().seqno, _isn, _reassembler.get_first_unassembled());
        _reassembler.push_substring(seg.payload().copy(), abs_seq - 1, seg.header().fin);
    }
    update_ack_no();
}

void TCPReceiver::update_ack_no(){
    // 如果到结尾需要+2 ， 因为fin 和syn 分别占据了一个字节
    // 平常就是相对字节流来说是 + 1；
    // TODO: 这里感觉有点问题， +2 的情况应该是期望收到最后的报文才对
    if(!has_syn) return;
    if(_reassembler.stream_out().input_ended()){
        _ackNo = wrap(_reassembler.get_first_unassembled()+2, _isn);
    } else{
        _ackNo = wrap(_reassembler.get_first_unassembled()+1, _isn);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(!has_syn) return nullopt;
    return _ackNo;
}

size_t TCPReceiver::window_size() const {
    return _reassembler.get_first_unacceptable() - _reassembler.get_first_unassembled();
}
